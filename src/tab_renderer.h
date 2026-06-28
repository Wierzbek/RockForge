#ifndef TAB_RENDERER_H
#define TAB_RENDERER_H

#include "raylib.h"
#include "song_data.h"

// Renders the tab sheet inside the given bounds. 
// current_beat is used to draw the playhead.
void DrawTabSheet(const Song& song, float current_beat, Rectangle bounds, Font font);

#endif // TAB_RENDERER_H
