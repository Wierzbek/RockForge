#include "note_math.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

int FreqToMidi(float freq) {
    if (freq <= 0.0f) return -1;
    return (int)roundf(69.0f + 12.0f * log2f(freq / 440.0f));
}

void MidiToName(int midi, char* out_name) {
    if (midi < 0) {
        strcpy(out_name, "--");
        return;
    }
    
    const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midi / 12) - 1;
    int note_index = midi % 12;
    
    sprintf(out_name, "%s%d", notes[note_index], octave);
}

static const int string_bases[] = { 40, 45, 50, 55, 59, 64 };

GuitarNote MidiToGuitarNote(int midi) {
    GuitarNote note = { -1, -1, midi };
    if (midi < 0) return note;

    // Standard Tuning MIDI bases: Low E (40), A (45), D (50), G (55), B (59), High E (64)
    
    // Simple heuristic: find the highest string that can play the note without going above fret 5 (unless it has to)
    // We prefer playing lower down the neck for basic open chords/notes, but if we go high, use higher strings.
    
    int best_string = -1;
    int best_fret = 999;
    
    for (int i = 5; i >= 0; i--) {
        if (midi >= string_bases[i]) {
            int fret = midi - string_bases[i];
            if (fret <= 22) { // Assuming 22 frets max
                if (best_string == -1 || fret < best_fret) {
                    // This creates a bias towards higher strings, lower frets
                    best_string = i;
                    best_fret = fret;
                    if (fret <= 5) break; // Good enough position
                }
            }
        }
    }
    
    if (best_string != -1) {
        note.string_idx = best_string;
        note.fret = best_fret;
    }
    
    return note;
}

float MidiToFreq(int midi) {
    return 440.0f * powf(2.0f, (midi - 69.0f) / 12.0f);
}

float GetCentsOffset(float freq, int midi_note) {
    float ideal_freq = MidiToFreq(midi_note);
    if (ideal_freq <= 0.0f || freq <= 0.0f) return 0.0f;
    return 1200.0f * log2f(freq / ideal_freq);
}

struct VoicingState {
    int current_assignment[6]; // string assigned to each note (-1 if none)
    int best_assignment[6];
    int best_cost;
};

static void SolveVoicing(const int* midi_notes, int count, int note_index, VoicingState* state, int used_strings_mask) {
    if (note_index == count) {
        // Evaluate cost
        int min_fret = 999;
        int max_fret = -1;
        
        for (int i = 0; i < count; i++) {
            int string_idx = state->current_assignment[i];
            if (string_idx == -1) continue;
            int fret = midi_notes[i] - string_bases[string_idx];
            if (fret > 0) { // ignore open strings for span calc
                if (fret < min_fret) min_fret = fret;
                if (fret > max_fret) max_fret = fret;
            }
        }
        
        int span = 0;
        if (max_fret != -1 && min_fret != 999) {
            span = max_fret - min_fret;
        }
        
        int cost = span * 10;
        
        // Massive penalty for unplayable stretch
        if (span > 4) cost += 1000;
        
        // Penalty for playing high on neck when not needed
        if (min_fret != 999) {
            cost += min_fret; // bias towards lower frets overall
        }
        
        if (cost < state->best_cost) {
            state->best_cost = cost;
            for (int i = 0; i < count; i++) {
                state->best_assignment[i] = state->current_assignment[i];
            }
        }
        return;
    }
    
    // Try assigning current note to each possible string
    int midi = midi_notes[note_index];
    bool assigned = false;
    
    for (int s = 0; s < 6; s++) {
        // If string already used in this chord, skip
        if (used_strings_mask & (1 << s)) continue;
        
        int fret = midi - string_bases[s];
        if (fret >= 0 && fret <= 22) {
            state->current_assignment[note_index] = s;
            SolveVoicing(midi_notes, count, note_index + 1, state, used_strings_mask | (1 << s));
            assigned = true;
        }
    }
    
    if (!assigned) {
        // Could not assign to any string (e.g. note is too high/low or all valid strings used)
        // Leave unassigned and continue
        state->current_assignment[note_index] = -1;
        SolveVoicing(midi_notes, count, note_index + 1, state, used_strings_mask);
    }
}

void OptimizeChordVoicing(const int* midi_notes, int count, GuitarNote* out_notes) {
    if (count <= 0) return;
    
    if (count == 1) {
        // For single notes, fallback to the standard heuristic (prefers lower frets)
        out_notes[0] = MidiToGuitarNote(midi_notes[0]);
        return;
    }
    
    VoicingState state;
    state.best_cost = 999999;
    for (int i = 0; i < 6; i++) {
        state.current_assignment[i] = -1;
        state.best_assignment[i] = -1;
    }
    
    SolveVoicing(midi_notes, count, 0, &state, 0);
    
    for (int i = 0; i < count; i++) {
        int best_string = state.best_assignment[i];
        if (best_string != -1) {
            out_notes[i].string_idx = best_string;
            out_notes[i].fret = midi_notes[i] - string_bases[best_string];
            out_notes[i].midi_note = midi_notes[i];
        } else {
            // Fallback if optimization completely failed for a note
            out_notes[i] = MidiToGuitarNote(midi_notes[i]);
        }
    }
}
