#include "song_data.h"
#include <string.h>
#include <algorithm>

const char* ROOT_NAMES[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};

const std::vector<ScaleDefinition> SCALES = {
    {"Major (Ionian)", {2, 2, 1, 2, 2, 2, 1}},
    {"Minor (Aeolian)", {2, 1, 2, 2, 1, 2, 2}},
    {"Minor Pentatonic", {3, 2, 2, 3, 2}},
    {"Major Pentatonic", {2, 2, 3, 2, 3}},
    {"Blues", {3, 2, 1, 1, 3, 2}},
    {"Dorian", {2, 1, 2, 2, 2, 1, 2}},
    {"Phrygian", {1, 2, 2, 2, 1, 2, 2}},
    {"Lydian", {2, 2, 2, 1, 2, 2, 1}},
    {"Mixolydian", {2, 2, 1, 2, 2, 1, 2}},
    {"Locrian", {1, 2, 2, 1, 2, 2, 2}}
};

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

static bool IsInScale(int pitch_class, int root, const std::vector<int>& intervals) {
    int current = root;
    for (int step : intervals) {
        if ((current % 12) == pitch_class) return true;
        current += step;
    }
    // The intervals should loop back to root anyway
    return (current % 12) == pitch_class;
}

Song GenerateDynamicScale(int root_pitch, const std::vector<int>& intervals, const char* name, float start_beat) {
    Song s;
    s.title = std::string(ROOT_NAMES[root_pitch]) + " " + name;
    s.tempo = 120;
    s.beats_per_measure = 4;
    s.beat_value = 4;
    
    // Determine the base fret on the Low E string
    // E string is MIDI 40
    int e_string_pitch_class = 40 % 12; // 4 (E)
    int base_fret = (root_pitch - e_string_pitch_class + 12) % 12;
    // We want a 5-fret box starting from base_fret.
    
    int string_base[] = { 40, 45, 50, 55, 59, 64 };
    std::vector<NoteEvent> playable_notes;
    
    for (int str = 0; str < 6; str++) {
        for (int fret = base_fret; fret <= base_fret + 5; fret++) {
            int midi = string_base[str] + fret;
            int pc = midi % 12;
            if (IsInScale(pc, root_pitch, intervals)) {
                playable_notes.push_back({0.0f, 1.0f, str, fret, 0});
            }
        }
    }
    
    // Sort notes by pitch
    std::sort(playable_notes.begin(), playable_notes.end(), [&](const NoteEvent& a, const NoteEvent& b) {
        return (string_base[a.string_idx] + a.fret) < (string_base[b.string_idx] + b.fret);
    });
    
    // Remove duplicate pitches (same MIDI note on different strings)
    playable_notes.erase(std::unique(playable_notes.begin(), playable_notes.end(), [&](const NoteEvent& a, const NoteEvent& b) {
        return (string_base[a.string_idx] + a.fret) == (string_base[b.string_idx] + b.fret);
    }), playable_notes.end());
    
    float current_beat = start_beat;
    float note_duration = 0.5f; // Eighth notes
    
    // Ascending
    for (size_t i = 0; i < playable_notes.size(); i++) {
        NoteEvent n = playable_notes[i];
        n.beat = current_beat;
        n.duration = note_duration;
        s.notes.push_back(n);
        current_beat += note_duration;
    }
    
    // Descending
    for (int i = (int)playable_notes.size() - 2; i >= 0; i--) {
        NoteEvent n = playable_notes[i];
        n.beat = current_beat;
        n.duration = note_duration;
        s.notes.push_back(n);
        current_beat += note_duration;
    }
    
    // Final hold note (the first note of the run, usually root)
    if (!playable_notes.empty()) {
        NoteEvent final_note = playable_notes[0];
        final_note.beat = current_beat;
        final_note.duration = 4.0f;
        s.notes.push_back(final_note);
    }
    
    return s;
}

Song GenerateCircleOfFifthsWorkout(int scale_index) {
    const int circle_of_fifths[] = { 0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5 }; // C, G, D, A, E, B, F#, Db, Ab, Eb, Bb, F
    
    Song workout;
    workout.title = std::string("Circle of Fifths: ") + SCALES[scale_index].name;
    workout.tempo = 120;
    workout.beats_per_measure = 4;
    workout.beat_value = 4;
    
    float current_beat = 0.0f;
    for (int i = 0; i < 12; i++) {
        Song part = GenerateDynamicScale(circle_of_fifths[i], SCALES[scale_index].intervals, SCALES[scale_index].name, current_beat);
        
        // Append part notes to workout notes
        for (const auto& note : part.notes) {
            workout.notes.push_back(note);
        }
        
        // Update current_beat to the end of this part, plus a short rest
        if (!part.notes.empty()) {
            current_beat = part.notes.back().beat + part.notes.back().duration + 4.0f; // 4 beats rest between keys
        }
    }
    
    return workout;
}
