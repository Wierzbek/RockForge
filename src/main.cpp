#include "../include/raylib.h"
#include "audio_capture.h"
#include "note_math.h"
#include <stdio.h>
#include <math.h>
#include <stdarg.h>

#define RAYGUI_IMPLEMENTATION
#include "../include/raygui.h"
#include "config_manager.h"
#include "song_data.h"
#include "tab_renderer.h"
#include "fretboard_view.h"
#include "file_manager.h"

enum AppMode {
    MODE_FREEPLAY = 0,
    MODE_PRACTICE,
    MODE_FRETBOARD
};

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

static float last_stable_freq = -1.0f;
static int stable_frames = 0;
static int active_midi[10];
static int active_count = 0;
static float snapshot_waveform[2048];
static bool has_snapshot = false;

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Guitar Tab Audio Analyzer");
    SetTargetFPS(60);

    if (!AudioCapture_InitContext()) {
        printf("Failed to init audio context.\n");
        CloseWindow();
        return 1;
    }

    DetectionConfig app_config = ConfigManager::LoadConfig("rockforge_config.ini");
    AudioCapture_SetDetectionConfig(app_config);

    // Setup GUI styles
    GuiLoadStyleDefault();

    Font font = GetFontDefault();
    bool device_selected = false;
    int device_count = AudioCapture_GetDeviceCount();
    
    AppMode current_mode = MODE_FREEPLAY;
    Song current_song = GenerateDynamicScale(9, SCALES[2].intervals, SCALES[2].name, 0.0f); // Default A Minor Pentatonic
    float current_beat = -4.0f; // 4-beat count-in
    bool playback_active = false;
    
    // UI State for Practice Mode
    bool is_circle_of_fifths = false;
    bool loop_playback = true;
    int root_selection = 9; // A
    int scale_selection = 2; // Minor Pentatonic
    bool root_dropdown_active = false;
    bool scale_dropdown_active = false;
    
    char root_names_str[256] = "C;C#;D;Eb;E;F;F#;G;G#;A;Bb;B";
    char scale_names_str[1024] = "";
    for (size_t i = 0; i < SCALES.size(); i++) {
        strcat(scale_names_str, SCALES[i].name);
        if (i < SCALES.size() - 1) strcat(scale_names_str, ";");
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){ 24, 24, 27, 255 }); // Dark gray/zinc background

        if (!device_selected) {
            DrawTextEx(font, "Select Audio Input Device", (Vector2){ 20, 20 }, 30, 2, LIGHTGRAY);
            
            int y_offset = 80;
            DrawText("Press the number key corresponding to your guitar interface:", 20, y_offset, 20, GRAY);
            y_offset += 40;

            DrawText("[0] Default System Capture Device", 40, y_offset, 20, WHITE);
            y_offset += 30;

            for (int i = 0; i < device_count; i++) {
                if (i >= 9) break; // Only show up to 9 for simple key mapping
                char dev_text[256];
                sprintf(dev_text, "[%d] %s", i + 1, AudioCapture_GetDeviceName(i));
                DrawText(dev_text, 40, y_offset, 20, WHITE);
                y_offset += 30;
            }

            // Check for key presses
            if (IsKeyPressed(KEY_ZERO)) {
                if (AudioCapture_StartDevice(-1)) device_selected = true;
            }
            for (int i = 0; i < device_count; i++) {
                if (i >= 9) break;
                if (IsKeyPressed(KEY_ONE + i)) {
                    if (AudioCapture_StartDevice(i)) device_selected = true;
                }
            }

        } else {
            // Main tracking loop
            float freq = AudioCapture_GetLatestPitch();
            
            if (freq > 40.0f && freq < 1000.0f) {
                if (stable_frames == 0) { // Just detected a new note onset!
                    AudioCapture_GetWaveform(snapshot_waveform, 2048);
                    has_snapshot = true;
                }
                last_stable_freq = freq;
                stable_frames = 10; // debouncing
            } else {
                if (stable_frames > 0) stable_frames--;
            }

            if (stable_frames > 0 && last_stable_freq > 0) {
                // Tuner maintains last_stable_freq
            }

            active_count = AudioCapture_GetActiveNotes(active_midi, 10);

            // Top Bar UI
            DrawRectangle(0, 0, GetScreenWidth(), 50, GetColor(0x27272aff)); // Zinc 800
            DrawTextEx(font, "RockForge", (Vector2){ 20, 10 }, 30, 2, LIGHTGRAY);
            
            // Mode Switcher Buttons
            if (GuiButton((Rectangle){ 200, 10, 120, 30 }, "Free Play")) {
                current_mode = MODE_FREEPLAY;
            }
            if (GuiButton((Rectangle){ 330, 10, 120, 30 }, "Practice Mode")) {
                current_mode = MODE_PRACTICE;
            }
            if (GuiButton((Rectangle){ 460, 10, 140, 30 }, "Fretboard Explorer")) {
                current_mode = MODE_FRETBOARD;
            }
            
            if (current_mode == MODE_FREEPLAY) {
                // Draw Freeplay Tablature Area
            int tab_x = 50;
            int tab_y = 150;
            int tab_width = GetScreenWidth() - 100;
            int string_spacing = 30;
            
            const char* string_names[] = {"e", "B", "G", "D", "A", "E"};

            if (active_count > 0) {
                GuitarNote optimized_notes[10]; // Max 10 notes, realistically 6
                OptimizeChordVoicing(active_midi, active_count, optimized_notes);

                for (int i = 0; i < 6; i++) {
                    int y = tab_y + i * string_spacing;
                    DrawLine(tab_x, y, tab_x + tab_width, y, (Color){ 82, 82, 91, 255 }); // Zinc 600
                    DrawText(string_names[i], tab_x - 30, y - 10, 20, LIGHTGRAY);
                    
                    // string_idx 5 is High E ("e"), which maps to i=0 (top line)
                    // string_idx 0 is Low E ("E"), which maps to i=5 (bottom line)
                    for (int n = 0; n < active_count; n++) {
                        GuitarNote gn = optimized_notes[n];
                        if (gn.string_idx == 5 - i && gn.fret != -1) {
                            DrawCircle(tab_x + tab_width / 2, y, 16, (Color){ 239, 68, 68, 255 }); // Red 500
                            char fret_text[4];
                            sprintf(fret_text, "%d", gn.fret);
                            int text_width = MeasureText(fret_text, 20);
                            DrawText(fret_text, tab_x + tab_width / 2 - text_width / 2, y - 10, 20, WHITE);
                        }
                    }
                }
            } else {
                for (int i = 0; i < 6; i++) {
                    int y = tab_y + i * string_spacing;
                    DrawLine(tab_x, y, tab_x + tab_width, y, (Color){ 82, 82, 91, 255 });
                    DrawText(string_names[i], tab_x - 30, y - 10, 20, LIGHTGRAY);
                }
            }

            // Display current note name and freq
            if (active_count > 0) {
                if (active_count == 1) {
                    char note_name[10];
                    MidiToName(active_midi[0], note_name);
                    char info_text[64];
                    sprintf(info_text, "Note: %s (%.1f Hz)", note_name, last_stable_freq);
                    DrawTextEx(font, info_text, (Vector2){ 20, 70 }, 24, 2, (Color){ 52, 211, 153, 255 }); // Emerald 400
                    
                    // Tuner UI
                    float cents = GetCentsOffset(last_stable_freq, active_midi[0]);
                    
                    int tuner_x = 350;
                    int tuner_y = 80;
                    int tuner_w = 400;
                    
                    // Draw tuner background line
                    DrawLine(tuner_x, tuner_y, tuner_x + tuner_w, tuner_y, GRAY);
                    
                    // Draw center mark and bounds
                    DrawLine(tuner_x + tuner_w / 2, tuner_y - 15, tuner_x + tuner_w / 2, tuner_y + 15, WHITE);
                    DrawLine(tuner_x, tuner_y - 10, tuner_x, tuner_y + 10, GRAY);
                    DrawLine(tuner_x + tuner_w, tuner_y - 10, tuner_x + tuner_w, tuner_y + 10, GRAY);
                    
                    // Draw cents text
                    char cents_text[32];
                    if (fabs(cents) < 5.0f) {
                        sprintf(cents_text, "In Tune");
                        DrawText(cents_text, tuner_x + tuner_w / 2 - MeasureText(cents_text, 20) / 2, tuner_y + 20, 20, GREEN);
                    } else if (cents < 0) {
                        sprintf(cents_text, "%.1f cents flat", -cents);
                        DrawText(cents_text, tuner_x + tuner_w / 2 - MeasureText(cents_text, 20) / 2, tuner_y + 20, 20, RED);
                    } else {
                        sprintf(cents_text, "%.1f cents sharp", cents);
                        DrawText(cents_text, tuner_x + tuner_w / 2 - MeasureText(cents_text, 20) / 2, tuner_y + 20, 20, RED);
                    }
                    
                    // Draw needle (cents ranges roughly -50 to +50)
                    float clamped_cents = cents;
                    if (clamped_cents < -50.0f) clamped_cents = -50.0f;
                    if (clamped_cents > 50.0f) clamped_cents = 50.0f;
                    
                    // -50 cents = tuner_x, +50 cents = tuner_x + tuner_w
                    int needle_x = tuner_x + (int)((clamped_cents + 50.0f) / 100.0f * tuner_w);
                    Color needle_color = (fabs(cents) < 5.0f) ? GREEN : RED;
                    
                    DrawCircle(needle_x, tuner_y, 8, needle_color);
                } else {
                    DrawTextEx(font, "Chords detected (Polyphonic Mode)", (Vector2){ 20, 70 }, 24, 2, (Color){ 52, 211, 153, 255 }); // Emerald 400
                }
            } else {
                DrawTextEx(font, "Waiting for signal...", (Vector2){ 20, 70 }, 24, 2, GRAY);
            }

            // Draw Oscilloscope
            int panel_w = 320;
            int osc_x = 50;
            int osc_y = 320; // below the tab
            int osc_w = GetScreenWidth() - panel_w - 100; // Leave room for panel
            int remaining_h = GetScreenHeight() - osc_y - 30; // 30px bottom padding
            if (remaining_h < 200) remaining_h = 200; // minimum fallback
            int osc_h = remaining_h / 2;

            DrawRectangle(osc_x, osc_y, osc_w, osc_h, (Color){ 39, 39, 42, 255 }); // Zinc 800 background
            DrawRectangleLines(osc_x, osc_y, osc_w, osc_h, (Color){ 82, 82, 91, 255 }); // Zinc 600 border

            float waveform[2048];
            AudioCapture_GetWaveform(waveform, 2048);

            for (int i = 0; i < 2047; i++) {
                float visual_gain = 2.5f; // Lowered to avoid flattening/clipping the peaks
                float val1 = waveform[i] * visual_gain;
                float val2 = waveform[i+1] * visual_gain;
                
                if (val1 > 1.0f) val1 = 1.0f; else if (val1 < -1.0f) val1 = -1.0f;
                if (val2 > 1.0f) val2 = 1.0f; else if (val2 < -1.0f) val2 = -1.0f;
                
                int x1 = osc_x + (int)(((float)i / 2048.0f) * osc_w);
                int x2 = osc_x + (int)(((float)(i+1) / 2048.0f) * osc_w);
                
                int y1 = osc_y + osc_h / 2 - (int)(val1 * (osc_h / 2.0f));
                int y2 = osc_y + osc_h / 2 - (int)(val2 * (osc_h / 2.0f));
                
                DrawLine(x1, y1, x2, y2, (Color){ 59, 130, 246, 255 }); // Blue 500
            }

            // Draw Snapshot Oscilloscope
            int snap_y = osc_y + osc_h + 10; // below the live osc
            int snap_h = osc_h;

            DrawRectangle(osc_x, snap_y, osc_w, snap_h, (Color){ 39, 39, 42, 255 }); 
            DrawRectangleLines(osc_x, snap_y, osc_w, snap_h, (Color){ 82, 82, 91, 255 }); 
            DrawText("Last Note Snapshot", osc_x + 10, snap_y + 10, 20, GRAY);
            
            if (has_snapshot) {
                for (int i = 0; i < 2047; i++) {
                    float visual_gain = 2.5f;
                    float val1 = snapshot_waveform[i] * visual_gain;
                    float val2 = snapshot_waveform[i+1] * visual_gain;
                    
                    if (val1 > 1.0f) val1 = 1.0f; else if (val1 < -1.0f) val1 = -1.0f;
                    if (val2 > 1.0f) val2 = 1.0f; else if (val2 < -1.0f) val2 = -1.0f;
                    
                    int x1 = osc_x + (int)(((float)i / 2048.0f) * osc_w);
                    int x2 = osc_x + (int)(((float)(i+1) / 2048.0f) * osc_w);

                    int y1 = snap_y + snap_h / 2 - (int)(val1 * (snap_h / 2.0f));
                    int y2 = snap_y + snap_h / 2 - (int)(val2 * (snap_h / 2.0f));
                    
                    DrawLine(x1, y1, x2, y2, (Color){ 245, 158, 11, 255 }); // Amber 500
                }
            }
            } else if (current_mode == MODE_PRACTICE) {
                // Practice Mode Logic
                
                // ROW 1: Generator (y=60)
                GuiCheckBox((Rectangle){ 20, 60, 20, 20 }, "Circle of Fifths", &is_circle_of_fifths);
                
                // ROW 2: Playback & File (y=100)
                if (GuiButton((Rectangle){ 20, 100, 80, 30 }, playback_active ? "PAUSE" : "PLAY")) {
                    playback_active = !playback_active;
                }
                if (GuiButton((Rectangle){ 110, 100, 80, 30 }, "STOP")) {
                    playback_active = false;
                    current_beat = -4.0f;
                    // Reset hits
                    for (auto& note : current_song.notes) note.hit_state = 0;
                }
                
                GuiCheckBox((Rectangle){ 210, 100, 20, 20 }, "Loop", &loop_playback);
                
                if (GuiButton((Rectangle){ 300, 100, 100, 30 }, "Save Song")) {
                    SaveSongToFile(current_song, "practice_save.txt");
                }
                if (GuiButton((Rectangle){ 410, 100, 100, 30 }, "Load Song")) {
                    LoadSongFromFile(current_song, "practice_save.txt");
                    current_beat = -4.0f;
                }
                
                // Tempo Slider
                char tempo_text[32];
                sprintf(tempo_text, "Tempo: %d", current_song.tempo);
                DrawText(tempo_text, 530, 105, 20, LIGHTGRAY);
                float tempo_f = (float)current_song.tempo;
                GuiSlider((Rectangle){ 630, 105, 120, 20 }, "", "", &tempo_f, 60.0f, 240.0f);
                current_song.tempo = (int)tempo_f;
                
                if (playback_active) {
                    current_beat += (current_song.tempo / 60.0f) * GetFrameTime();
                    
                    // Hit detection logic
                    float tolerance = 0.25f; // beats
                    int string_base[] = { 40, 45, 50, 55, 59, 64 };
                    float max_beat = 0.0f;
                    
                    for (auto& note : current_song.notes) {
                        if (note.beat + note.duration > max_beat) max_beat = note.beat + note.duration;
                        
                        if (note.hit_state == 0) {
                            if (current_beat >= note.beat - tolerance && current_beat <= note.beat + tolerance) {
                                int target_midi = string_base[note.string_idx] + note.fret;
                                bool hit = false;
                                for (int i = 0; i < active_count; i++) {
                                    if (active_midi[i] == target_midi) { hit = true; break; }
                                }
                                if (hit) note.hit_state = 1;
                            } else if (current_beat > note.beat + tolerance) {
                                note.hit_state = -1; // missed
                            }
                        }
                    }
                    
                    // Loop Logic
                    if (loop_playback && current_beat > max_beat + 1.0f) { // 1 beat rest at end before loop
                        current_beat = 0.0f;
                        for (auto& note : current_song.notes) note.hit_state = 0;
                    }
                }
                
                // Render Sheet
                DrawTabSheet(current_song, current_beat, (Rectangle){ 20, 150, (float)GetScreenWidth() - 250, (float)GetScreenHeight() - 160 }, font);
                
                // Draw Count-In text if necessary
                if (current_beat < 0.0f) {
                    int count = (int)(-current_beat) + 1;
                    char count_text[32];
                    sprintf(count_text, "Get Ready: %d", count);
                    DrawText(count_text, GetScreenWidth() / 2 - 100, GetScreenHeight() / 2 - 50, 40, ORANGE);
                }
                
                // MUST DRAW DROPDOWNS LAST so they overlap the tab sheet
                if (GuiButton((Rectangle){ 480, 55, 100, 30 }, "Generate")) {
                    if (is_circle_of_fifths) {
                        current_song = GenerateCircleOfFifthsWorkout(scale_selection);
                    } else {
                        current_song = GenerateDynamicScale(root_selection, SCALES[scale_selection].intervals, SCALES[scale_selection].name, 0.0f);
                    }
                    playback_active = false;
                    current_beat = -4.0f;
                }
                
                if (GuiDropdownBox((Rectangle){ 280, 55, 180, 30 }, scale_names_str, &scale_selection, scale_dropdown_active)) {
                    scale_dropdown_active = !scale_dropdown_active;
                }
                if (!is_circle_of_fifths) {
                    if (GuiDropdownBox((Rectangle){ 200, 55, 60, 30 }, root_names_str, &root_selection, root_dropdown_active)) {
                        root_dropdown_active = !root_dropdown_active;
                    }
                }
            } else if (current_mode == MODE_FRETBOARD) {
                // Fretboard Explorer Logic
                Rectangle bounds = { 20, 100, (float)GetScreenWidth() - 280, 300 };
                DrawFretboard(bounds, current_song, active_midi, active_count);
                
                DrawText("Fretboard Explorer", 20, 60, 24, LIGHTGRAY);
                DrawText(TextFormat("Current Scale: %s", current_song.title.c_str()), 20, 420, 20, GRAY);
            }
            
            // Draw Settings Panel on the Right
            int panel_w = 200;
            int panel_x = GetScreenWidth() - panel_w - 20;
            int panel_y = 50;

            GuiPanel((Rectangle){ (float)panel_x, (float)panel_y, (float)panel_w, 320.0f }, "Calibration Settings");

            int slider_y = panel_y + 40;
            float rms = AudioCapture_GetCurrentRMS();
            GuiProgressBar((Rectangle){ (float)panel_x + 60, (float)slider_y, (float)panel_w - 100, 20.0f }, "Vol RMS", NULL, &rms, 0.0f, 0.05f);
            slider_y += 40;

            GuiSlider((Rectangle){ (float)panel_x + 60, (float)slider_y, (float)panel_w - 100, 20.0f }, "Gate", TextFormat("%.3f", app_config.noise_gate_rms), &app_config.noise_gate_rms, 0.0f, 0.05f);
            slider_y += 40;

            GuiSlider((Rectangle){ (float)panel_x + 60, (float)slider_y, (float)panel_w - 100, 20.0f }, "Confid.", TextFormat("%.2f", app_config.nn_confidence_threshold), &app_config.nn_confidence_threshold, 0.0f, 1.0f);
            slider_y += 40;

            float float_smoothing = (float)app_config.smoothing_window_frames;
            GuiSlider((Rectangle){ (float)panel_x + 60, (float)slider_y, (float)panel_w - 100, 20.0f }, "Smooth", TextFormat("%d", (int)float_smoothing), &float_smoothing, 1.0f, 50.0f);
            app_config.smoothing_window_frames = (int)float_smoothing;
            slider_y += 60;

            if (GuiButton((Rectangle){ (float)panel_x + 20, (float)slider_y, (float)panel_w - 40, 30.0f }, "Save Settings")) {
                ConfigManager::SaveConfig("rockforge_config.ini", app_config);
            }

            // Sync config to audio thread
            AudioCapture_SetDetectionConfig(app_config);

        }

        EndDrawing();
    }

    AudioCapture_CloseContext();
    CloseWindow();
    return 0;
}
