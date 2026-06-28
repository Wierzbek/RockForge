#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <stdbool.h>

struct DetectionConfig {
    float noise_gate_rms;          // Minimum volume to trigger inference (e.g. 0.001)
    float nn_confidence_threshold; // Threshold to accept note (e.g. 0.2)
    int smoothing_window_frames;   // Number of frames to look back for debounce (e.g. 15)
};

// Initialize miniaudio context and enumerate devices
bool AudioCapture_InitContext(void);
void AudioCapture_CloseContext(void);

int AudioCapture_GetDeviceCount(void);
const char* AudioCapture_GetDeviceName(int index);

// If index is -1, uses the default capture device
bool AudioCapture_StartDevice(int index);
void AudioCapture_StopDevice(void);

float AudioCapture_GetLatestPitch(void);

// Configuration Setters and Getters
void AudioCapture_SetDetectionConfig(DetectionConfig config);
DetectionConfig AudioCapture_GetDetectionConfig(void);

// Get the current audio volume (RMS)
float AudioCapture_GetCurrentRMS(void);

// Fetch a snapshot of the raw audio buffer for visual rendering (e.g. oscilloscope)
// out_buffer must be pre-allocated to hold num_samples.
void AudioCapture_GetWaveform(float* out_buffer, int num_samples);

// Get the active polyphonic notes
int AudioCapture_GetActiveNotes(int* out_notes, int max_notes);

#endif // AUDIO_CAPTURE_H
