#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "song_data.h"

// Saves a song to a custom text/JSON-like format
bool SaveSongToFile(const Song& song, const char* filepath);

// Loads a song from a custom text/JSON-like format
bool LoadSongFromFile(Song& out_song, const char* filepath);

#endif // FILE_MANAGER_H
