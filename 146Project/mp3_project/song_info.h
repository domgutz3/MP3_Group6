#pragma once

#include "ff.h"
#include "song_list.h"
#include "stdbool.h"
#include "stdio.h"
#include <string.h>

typedef struct {
  char song_name[128];
  char artist_name[128];
  char album_name[128];
} song_metadata;

song_metadata get_metadata(song_memory_t song_name);
bool has_metadata();
void metadata_remove_tags();
void song_info_extract();