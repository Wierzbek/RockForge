#include "song_data.h"
#include <string.h>

Song GeneratePracticeScale(const char* scale_type) {
    Song s;
    s.tempo = 120;
    s.beats_per_measure = 4;
    s.beat_value = 4;
    
    if (strcmp(scale_type, "A Minor Pentatonic") == 0) {
        s.title = "A Minor Pentatonic (Ascending/Descending)";
        
        // A Minor Pentatonic starting on 5th fret Low E
        // Strings: 0=LowE, 1=A, 2=D, 3=G, 4=B, 5=HighE
        int scale_frets[][2] = {
            {0, 5}, {0, 8},   // E string: A, C
            {1, 5}, {1, 7},   // A string: D, E
            {2, 5}, {2, 7},   // D string: G, A
            {3, 5}, {3, 7},   // G string: C, D
            {4, 5}, {4, 8},   // B string: E, G
            {5, 5}, {5, 8}    // e string: A, C
        };
        
        float current_beat = 0.0f;
        float note_duration = 1.0f; // Quarter notes
        
        // Ascending
        for (int i = 0; i < 12; i++) {
            s.notes.push_back({current_beat, note_duration, scale_frets[i][0], scale_frets[i][1], 0});
            current_beat += note_duration;
        }
        
        // Descending
        for (int i = 10; i >= 0; i--) {
            s.notes.push_back({current_beat, note_duration, scale_frets[i][0], scale_frets[i][1], 0});
            current_beat += note_duration;
        }
        
        // Final long note
        s.notes.push_back({current_beat, 4.0f, 0, 5, 0}); // Root note A on Low E
    } else {
        // G Major Scale default
        s.title = "G Major Scale (2 Octaves)";
        int scale_frets[][2] = {
            {0, 3}, {0, 5},           // E: G, A
            {1, 2}, {1, 3}, {1, 5},   // A: B, C, D
            {2, 2}, {2, 4}, {2, 5},   // D: E, F#, G
            {3, 2}, {3, 4}, {3, 5},   // G: A, B, C
            {4, 3}, {4, 5},           // B: D, E
            {5, 2}, {5, 3}            // e: F#, G
        };
        
        float current_beat = 0.0f;
        float note_duration = 0.5f; // Eighth notes
        
        for (int i = 0; i < 15; i++) {
            s.notes.push_back({current_beat, note_duration, scale_frets[i][0], scale_frets[i][1], 0});
            current_beat += note_duration;
        }
        for (int i = 13; i >= 0; i--) {
            s.notes.push_back({current_beat, note_duration, scale_frets[i][0], scale_frets[i][1], 0});
            current_beat += note_duration;
        }
        s.notes.push_back({current_beat, 4.0f, 0, 3, 0});
    }
    
    return s;
}
