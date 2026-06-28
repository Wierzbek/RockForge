#ifndef SONG_DATA_H
#define SONG_DATA_H

#include <vector>
#include <string>

struct NoteEvent {
    float beat;         // When the note occurs (0.0 = start of song, 1.0 = beat 2 of measure 1, etc.)
    float duration;     // Duration in beats
    int string_idx;     // 0 = Low E, 1 = A, 2 = D, 3 = G, 4 = B, 5 = High e
    int fret;           // Fret number
    int hit_state;      // 0 = Unplayed, 1 = Hit, -1 = Missed
};

struct Song {
    std::string title;
    int tempo;          // BPM (Beats Per Minute)
    int beats_per_measure; // Numerator of time signature (e.g. 4 for 4/4)
    int beat_value;        // Denominator (e.g. 4 for 4/4)
    std::vector<NoteEvent> notes;
};

struct ScaleDefinition {
    const char* name;
    std::vector<int> intervals;
};

// Expose these for UI selection
extern const std::vector<ScaleDefinition> SCALES;
extern const char* ROOT_NAMES[];

// Generates a simple practice scale (Legacy)
Song GeneratePracticeScale(const char* scale_type);

// Procedurally generates an ascending/descending tab for a given key and scale
Song GenerateDynamicScale(int root_pitch, const std::vector<int>& intervals, const char* name, float start_beat = 0.0f);

// Stitches all 12 keys together for a massive endurance track
Song GenerateCircleOfFifthsWorkout(int scale_index);

#endif // SONG_DATA_H
