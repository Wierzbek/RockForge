#include "file_manager.h"
#include <stdio.h>
#include <string.h>

bool SaveSongToFile(const Song& song, const char* filepath) {
    FILE* f = fopen(filepath, "w");
    if (!f) return false;
    
    fprintf(f, "title: %s\n", song.title.c_str());
    fprintf(f, "tempo: %d\n", song.tempo);
    fprintf(f, "time: %d %d\n", song.beats_per_measure, song.beat_value);
    
    for (const auto& note : song.notes) {
        fprintf(f, "note: %f %f %d %d\n", note.beat, note.duration, note.string_idx, note.fret);
    }
    
    fclose(f);
    return true;
}

bool LoadSongFromFile(Song& out_song, const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;
    
    out_song.notes.clear();
    out_song.title = "Loaded Song";
    out_song.tempo = 120;
    out_song.beats_per_measure = 4;
    out_song.beat_value = 4;
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "title: ", 7) == 0) {
            char title_buf[128];
            sscanf(line + 7, "%[^\n]", title_buf);
            out_song.title = title_buf;
        } else if (strncmp(line, "tempo: ", 7) == 0) {
            sscanf(line + 7, "%d", &out_song.tempo);
        } else if (strncmp(line, "time: ", 6) == 0) {
            sscanf(line + 6, "%d %d", &out_song.beats_per_measure, &out_song.beat_value);
        } else if (strncmp(line, "note: ", 6) == 0) {
            NoteEvent n;
            n.hit_state = 0;
            if (sscanf(line + 6, "%f %f %d %d", &n.beat, &n.duration, &n.string_idx, &n.fret) == 4) {
                out_song.notes.push_back(n);
            }
        }
    }
    
    fclose(f);
    return true;
}
