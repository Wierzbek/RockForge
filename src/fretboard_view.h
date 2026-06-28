#ifndef FRETBOARD_VIEW_H
#define FRETBOARD_VIEW_H

#include "raylib.h"
#include "song_data.h"

// Draws a full 22-fret guitar neck.
// Highlights all pitch classes present in the `song`.
// Specifically brightens notes that are currently in `active_midi`.
void DrawFretboard(Rectangle bounds, const Song& song, int* active_midi, int active_count);

#endif // FRETBOARD_VIEW_H
