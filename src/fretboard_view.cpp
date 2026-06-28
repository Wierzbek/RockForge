#include "fretboard_view.h"
#include <stdio.h>

void DrawFretboard(Rectangle bounds, const Song& song, int* active_midi, int active_count) {
    int num_frets = 22;
    int num_strings = 6;
    
    // String names and base midi values (0=Low E, 5=High e)
    const char* string_names[] = {"E", "A", "D", "G", "B", "e"};
    int string_base[] = { 40, 45, 50, 55, 59, 64 };
    
    // 1. Analyze the song to extract all pitch classes (0-11)
    bool pitch_classes[12] = { false };
    for (const auto& note : song.notes) {
        int midi = string_base[note.string_idx] + note.fret;
        pitch_classes[midi % 12] = true;
    }
    
    // 2. Draw Fretboard Wood / Background
    float string_spacing = bounds.height / (num_strings + 1);
    float fret_spacing = bounds.width / (num_frets + 1); // Fret 0 is open string
    
    DrawRectangleRec(bounds, (Color){ 30, 25, 20, 255 }); // Dark wood
    DrawRectangleLinesEx(bounds, 2.0f, (Color){ 60, 50, 40, 255 });
    
    // 3. Draw Frets
    for (int f = 1; f <= num_frets; f++) {
        float x = bounds.x + f * fret_spacing;
        DrawLineEx((Vector2){x, bounds.y}, (Vector2){x, bounds.y + bounds.height}, 3.0f, (Color){ 200, 200, 200, 255 });
        
        // Fret markers (3, 5, 7, 9, 12, 15, 17, 19, 21)
        if (f==3 || f==5 || f==7 || f==9 || f==15 || f==17 || f==19 || f==21) {
            DrawCircle(x - fret_spacing/2, bounds.y + bounds.height/2, 8, (Color){ 255, 255, 255, 100 });
        } else if (f == 12) {
            DrawCircle(x - fret_spacing/2, bounds.y + bounds.height/3, 8, (Color){ 255, 255, 255, 100 });
            DrawCircle(x - fret_spacing/2, bounds.y + bounds.height*2/3, 8, (Color){ 255, 255, 255, 100 });
        }
    }
    
    // Nut
    DrawLineEx((Vector2){bounds.x + fret_spacing, bounds.y}, (Vector2){bounds.x + fret_spacing, bounds.y + bounds.height}, 8.0f, WHITE);
    
    // 4. Draw Strings
    for (int s = 0; s < num_strings; s++) {
        // Reverse order visually: High e is on top (visual string 0), Low E is bottom
        float y = bounds.y + (5 - s + 1) * string_spacing; 
        DrawLineEx((Vector2){bounds.x, y}, (Vector2){bounds.x + bounds.width, y}, 1.0f + (s * 0.5f), (Color){ 180, 180, 180, 255 });
        
        // Draw string name on the left (open string area)
        DrawText(string_names[s], bounds.x + 10, y - 10, 20, LIGHTGRAY);
    }
    
    // 5. Draw Notes
    const char* note_names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    for (int s = 0; s < num_strings; s++) {
        float y = bounds.y + (5 - s + 1) * string_spacing;
        
        for (int f = 0; f <= num_frets; f++) {
            int midi = string_base[s] + f;
            int pitch_class = midi % 12;
            
            // Check if this note is in the scale
            if (pitch_classes[pitch_class]) {
                float x = (f == 0) ? (bounds.x + fret_spacing/2 - 10) : (bounds.x + f * fret_spacing - fret_spacing/2);
                
                // Check if actively played
                bool is_active = false;
                for (int i = 0; i < active_count; i++) {
                    if (active_midi[i] == midi) {
                        is_active = true; break;
                    }
                }
                
                Color c = (Color){ 59, 130, 246, 255 }; // Blue (Scale note)
                if (is_active) {
                    c = GREEN; // Bright green (Played note)
                    DrawCircle(x, y, 22, (Color){ 0, 255, 0, 100 }); // Glow
                }
                
                DrawCircle(x, y, 16, c);
                DrawCircleLines(x, y, 16, WHITE);
                
                const char* name = note_names[pitch_class];
                int tw = MeasureText(name, 16);
                DrawText(name, x - tw/2, y - 8, 16, WHITE);
            }
        }
    }
}
