#include "tab_renderer.h"
#include <stdio.h>

void DrawTabSheet(const Song& song, float current_beat, Rectangle bounds, Font font) {
    // Basic constants
    float staff_spacing = 150.0f; // vertical space between staffs
    float string_spacing = 15.0f;
    float staff_height = 5 * string_spacing;
    
    float measure_width = 250.0f; // Width per measure
    int measures_per_line = (int)(bounds.width / measure_width);
    if (measures_per_line < 1) measures_per_line = 1;
    
    // String names
    const char* string_names[] = {"e", "B", "G", "D", "A", "E"};
    
    // Calculate total measures based on last note
    float max_beat = 0.0f;
    for (const auto& note : song.notes) {
        if (note.beat > max_beat) max_beat = note.beat;
    }
    int total_measures = (int)(max_beat / song.beats_per_measure) + 1;
    
    // Draw Title
    DrawTextEx(font, song.title.c_str(), (Vector2){ bounds.x, bounds.y }, 24, 2, WHITE);
    
    float start_y = bounds.y + 50.0f;
    
    // Determine which line the playhead is currently on
    int current_measure = (int)(current_beat / song.beats_per_measure);
    int current_line = current_measure / measures_per_line;
    
    // We only render a few lines centered around the current line (scrolling effect if song is long)
    // Or just render all lines and let it clip/scroll manually. Let's render everything for now.
    
    for (int line = 0; line <= total_measures / measures_per_line; line++) {
        float line_y = start_y + line * staff_spacing;
        
        // Stop drawing if we go off the bottom of the screen
        if (line_y > bounds.y + bounds.height) break; 
        
        // Only draw lines that are on screen or close to the playhead
        // Scroll vertically if needed
        float scroll_offset = 0;
        if (current_line > 2) {
            scroll_offset = (current_line - 2) * staff_spacing;
        }
        
        float draw_y = line_y - scroll_offset;
        
        // Skip rendering if off screen due to scrolling
        if (draw_y < bounds.y || draw_y > bounds.y + bounds.height) continue;
        
        // Draw the staff lines (6 strings)
        for (int s = 0; s < 6; s++) {
            float sy = draw_y + s * string_spacing;
            DrawLine(bounds.x + 30, sy, bounds.x + bounds.width - 30, sy, (Color){ 82, 82, 91, 255 }); // Zinc 600
            DrawText(string_names[s], bounds.x, sy - 10, 20, LIGHTGRAY);
        }
        
        // Draw Measure Bar Lines
        int start_measure_idx = line * measures_per_line;
        for (int m = 0; m <= measures_per_line; m++) {
            float bar_x = bounds.x + 30 + m * measure_width;
            if (bar_x > bounds.x + bounds.width - 30) break;
            
            DrawLine(bar_x, draw_y, bar_x, draw_y + staff_height, (Color){ 161, 161, 170, 255 }); // Zinc 400
            
            // Draw measure number
            char m_text[10];
            sprintf(m_text, "%d", start_measure_idx + m + 1);
            DrawText(m_text, bar_x + 5, draw_y - 20, 10, GRAY);
        }
        
        // Draw Playhead for this line
        if (line == current_line) {
            float line_beat = current_beat - (start_measure_idx * song.beats_per_measure);
            float playhead_x = bounds.x + 30 + (line_beat / song.beats_per_measure) * measure_width;
            
            DrawLineEx((Vector2){playhead_x, draw_y - 10}, (Vector2){playhead_x, draw_y + staff_height + 10}, 2.0f, GREEN);
        }
        
        // Draw Notes for this line
        for (const auto& note : song.notes) {
            int note_measure = (int)(note.beat / song.beats_per_measure);
            int note_line = note_measure / measures_per_line;
            
            if (note_line == line) {
                float local_beat = note.beat - (start_measure_idx * song.beats_per_measure);
                float note_x = bounds.x + 30 + (local_beat / song.beats_per_measure) * measure_width;
                
                // Inverse string rendering: 5 is High E (top), 0 is Low E (bottom)
                float note_y = draw_y + (5 - note.string_idx) * string_spacing;
                
                // Background circle to hide the staff line behind the number
                DrawCircle(note_x, note_y, 10, GetColor(0x18181bff)); // Match BACKGROUND_COLOR
                
                // Determine color
                Color c = WHITE;
                if (note.hit_state == 1) {
                    c = GREEN;
                    DrawCircle(note_x, note_y, 14, (Color){0, 255, 0, 100}); // green glow
                } else if (note.hit_state == -1) {
                    c = RED;
                } else if (current_beat >= note.beat && current_beat <= note.beat + note.duration) {
                    // Unplayed but active
                    c = ORANGE;
                    DrawCircle(note_x, note_y, 14, (Color){255, 165, 0, 100}); // orange glow
                }
                
                char fret_text[4];
                sprintf(fret_text, "%d", note.fret);
                int tw = MeasureText(fret_text, 16);
                DrawText(fret_text, note_x - tw/2, note_y - 8, 16, c);
            }
        }
    }
}
