#define MINIAUDIO_IMPLEMENTATION
#include "../include/miniaudio.h"
#include "audio_capture.h"
#include "yin.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include "../basicpitch.cpp/src/basicpitch.hpp"

#define SAMPLE_RATE 22050
#define YIN_BUFFER_SIZE 2048
#define CIRCULAR_BUFFER_SIZE 88200

static ma_context context;
static bool context_initialized = false;
static ma_device_info* capture_infos = NULL;
static ma_uint32 capture_count = 0;

static ma_device device;
static float circular_buffer[CIRCULAR_BUFFER_SIZE];
static int write_index = 0;
static bool is_device_started = false;
static Yin yin;

static std::thread inference_thread;
static std::atomic<bool> stop_inference(false);
static std::mutex active_notes_mutex;
static std::vector<int> current_active_notes;

static std::mutex config_mutex;
static DetectionConfig current_config = {0.001f, 0.2f, 15}; // defaults

static std::mutex rms_mutex;
static float current_rms = 0.0f;

static void InferenceLoop() {
    std::vector<float> local_buffer(43008, 0.0f);
    while (!stop_inference.load()) {
        if (!is_device_started) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // Lockless reading is mostly okay here since we just copy the circular buffer
        // using the existing AudioCapture_GetWaveform helper
        AudioCapture_GetWaveform(local_buffer.data(), 43008);
        
        // Calculate RMS of the buffer
        float sum_sq = 0.0f;
        for (float sample : local_buffer) {
            sum_sq += sample * sample;
        }
        float rms = sqrtf(sum_sq / local_buffer.size());
        {
            std::lock_guard<std::mutex> lock(rms_mutex);
            current_rms = rms;
        }

        // Grab current config
        DetectionConfig cfg;
        {
            std::lock_guard<std::mutex> lock(config_mutex);
            cfg = current_config;
        }
        
        try {
            std::vector<int> new_active;
            
            // Noise gate
            if (rms >= cfg.noise_gate_rms) {
                auto result = basic_pitch::ort_inference(local_buffer);
                auto& notes = result.notes;
                int num_frames = notes.dimension(0);
                
                if (num_frames > 0) {
                    // Look at the last N frames to avoid edge artifacts and prevent blinking
                    int start_frame = num_frames - cfg.smoothing_window_frames;
                    if (start_frame < 0) start_frame = 0;
                    
                    struct NoteConf { int midi; float conf; };
                    std::vector<NoteConf> candidates;
                    
                    for (int i = 0; i < 88; i++) {
                        float max_conf = 0.0f;
                        for (int f = start_frame; f < num_frames; f++) {
                            float conf = notes(f, i);
                            if (conf > max_conf) max_conf = conf;
                        }
                        
                        if (max_conf > cfg.nn_confidence_threshold) {
                            candidates.push_back({i + basic_pitch::constants::MIDI_OFFSET, max_conf});
                        }
                    }
                    
                    // Sort descending by confidence to get the strongest fundamentals
                    std::sort(candidates.begin(), candidates.end(), [](const NoteConf& a, const NoteConf& b) {
                        return a.conf > b.conf;
                    });
                    
                    if (!candidates.empty()) {
                        float strongest_conf = candidates[0].conf;
                        for (size_t k = 0; k < candidates.size() && k < 6; k++) { // Max 6 strings!
                            // Ignore weak overtones that are less than 40% as confident as the main note
                            if (candidates[k].conf >= strongest_conf * 0.4f) {
                                new_active.push_back(candidates[k].midi);
                            }
                        }
                    }
                }
            }
            
            {
                std::lock_guard<std::mutex> lock(active_notes_mutex);
                current_active_notes = new_active;
            }
        } catch (...) {
            // Inference failed (model not found etc.)
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10 fps updates
    }
}

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pDevice;
    (void)pOutput;
    if (pInput == NULL) return;

    const float* input_samples = (const float*)pInput;
    for (ma_uint32 i = 0; i < frameCount; i++) {
        circular_buffer[write_index] = input_samples[i];
        write_index = (write_index + 1) % CIRCULAR_BUFFER_SIZE;
    }
}

bool AudioCapture_InitContext(void) {
    stop_inference = false;
    inference_thread = std::thread(InferenceLoop);

    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        printf("Failed to initialize context.\n");
        return false;
    }
    
    ma_device_info* playback_infos;
    ma_uint32 playback_count;
    
    if (ma_context_get_devices(&context, &playback_infos, &playback_count, &capture_infos, &capture_count) != MA_SUCCESS) {
        printf("Failed to enumerate devices.\n");
        ma_context_uninit(&context);
        return false;
    }
    
    context_initialized = true;
    return true;
}

void AudioCapture_CloseContext(void) {
    stop_inference = true;
    if (inference_thread.joinable()) {
        inference_thread.join();
    }

    if (is_device_started) {
        AudioCapture_StopDevice();
    }
    if (context_initialized) {
        ma_context_uninit(&context);
        context_initialized = false;
    }
}

int AudioCapture_GetDeviceCount(void) {
    return capture_count;
}

const char* AudioCapture_GetDeviceName(int index) {
    if (index < 0 || (ma_uint32)index >= capture_count) return NULL;
    return capture_infos[index].name;
}

bool AudioCapture_StartDevice(int index) {
    if (!context_initialized) return false;
    if (is_device_started) AudioCapture_StopDevice();

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format   = ma_format_f32;
    deviceConfig.capture.channels = 1;
    deviceConfig.sampleRate       = 22050;
    deviceConfig.dataCallback     = data_callback;

    if (index >= 0 && (ma_uint32)index < capture_count) {
        deviceConfig.capture.pDeviceID = &capture_infos[index].id;
    } else {
        deviceConfig.capture.pDeviceID = NULL; // Default device
    }

    if (ma_device_init(&context, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to initialize capture device.\n");
        return false;
    }

    Yin_Init(&yin, YIN_BUFFER_SIZE, 0.15f); 

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start capture device.\n");
        ma_device_uninit(&device);
        Yin_Free(&yin);
        return false;
    }

    is_device_started = true;
    return true;
}

void AudioCapture_StopDevice(void) {
    if (!is_device_started) return;
    ma_device_uninit(&device);
    Yin_Free(&yin);
    is_device_started = false;
}

float AudioCapture_GetLatestPitch(void) {
    if (!is_device_started) return -1.0f;

    float processing_buffer[YIN_BUFFER_SIZE];
    int start_index = write_index - YIN_BUFFER_SIZE;
    if (start_index < 0) start_index += CIRCULAR_BUFFER_SIZE;

    for (int i = 0; i < YIN_BUFFER_SIZE; i++) {
        processing_buffer[i] = circular_buffer[(start_index + i) % CIRCULAR_BUFFER_SIZE];
    }

    float sum_squares = 0;
    for (int i = 0; i < YIN_BUFFER_SIZE; i++) {
        sum_squares += processing_buffer[i] * processing_buffer[i];
    }
    float rms = sqrtf(sum_squares / YIN_BUFFER_SIZE);
    
    if (rms < 0.01f) {
        return -1.0f; 
    }

    return Yin_GetPitch(&yin, processing_buffer, SAMPLE_RATE);
}

void AudioCapture_GetWaveform(float* out_buffer, int num_samples) {
    if (!is_device_started || num_samples <= 0) {
        if (num_samples > 0) memset(out_buffer, 0, sizeof(float) * num_samples);
        return;
    }
    
    if (num_samples > CIRCULAR_BUFFER_SIZE) num_samples = CIRCULAR_BUFFER_SIZE;
    
    int start_index = write_index - num_samples;
    if (start_index < 0) start_index += CIRCULAR_BUFFER_SIZE;
    
    for (int i = 0; i < num_samples; i++) {
        out_buffer[i] = circular_buffer[(start_index + i) % CIRCULAR_BUFFER_SIZE];
    }
}

int AudioCapture_GetActiveNotes(int* out_notes, int max_notes) {
    std::lock_guard<std::mutex> lock(active_notes_mutex);
    int count = 0;
    for (int note : current_active_notes) {
        if (count >= max_notes) break;
        out_notes[count++] = note;
    }
    return count;
}

void AudioCapture_SetDetectionConfig(DetectionConfig config) {
    std::lock_guard<std::mutex> lock(config_mutex);
    current_config = config;
}

DetectionConfig AudioCapture_GetDetectionConfig(void) {
    std::lock_guard<std::mutex> lock(config_mutex);
    return current_config;
}

float AudioCapture_GetCurrentRMS(void) {
    std::lock_guard<std::mutex> lock(rms_mutex);
    return current_rms;
}
