#ifndef NOTE_MATH_H
#define NOTE_MATH_H

typedef struct {
    int string_idx; // 0 = Low E (6th string), 1 = A, 2 = D, 3 = G, 4 = B, 5 = High E (1st string)
    int fret;
    int midi_note;
} GuitarNote;

// Convert frequency in Hz to closest MIDI note
int FreqToMidi(float freq);

// Get Note Name (e.g. "E2", "A2")
void MidiToName(int midi, char* out_name);

// Map a MIDI note to the best string/fret for Standard E tuning
GuitarNote MidiToGuitarNote(int midi);

// Return the exact frequency of a given MIDI note
float MidiToFreq(int midi);

// Return the offset in cents of a frequency from a target MIDI note
float GetCentsOffset(float freq, int midi_note);

// Takes an array of raw MIDI notes and outputs an optimized array of GuitarNotes
// that minimizes hand stretch and avoids string collisions.
void OptimizeChordVoicing(const int* midi_notes, int count, GuitarNote* out_notes);

#endif // NOTE_MATH_H
