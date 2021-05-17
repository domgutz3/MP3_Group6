#include "song_info.h"

char bytes_128[128];
song_metadata return_metadata;

// 8 indexes

song_metadata get_metadata(song_memory_t song_name) {
  FIL temp_file;
  FRESULT result;
  UINT bytes_read;

  // Reinitialize return_metadata struct
  strcpy(return_metadata.song_name, "");
  strcpy(return_metadata.album_name, "");
  strcpy(return_metadata.artist_name, "");

  printf("Song to extract: %s\n", song_name);

  result = f_open(&temp_file, song_name, FA_READ);

  if (result == FR_OK) {
    f_read(&temp_file, (void *)&bytes_128, 128, &bytes_read);
    f_close(&temp_file);

    // Song Info Extraction
    if (has_metadata()) {
      song_info_extract();
    }

    // metadata_remove_tags();

  } else {
    printf("Error: file %s not found\n", song_name);
  }
  return return_metadata;
}

void song_info_extract() {
  int term_count = 0;
  int temp_index;

  for (int i = 16; i < 128; i++) {
    if (bytes_128[i] != 0x00) {
      if (bytes_128[i + 1] == 0x00)
        continue;
      temp_index = i;
      while (bytes_128[temp_index] != 0x00) {
        char *temp = bytes_128[temp_index];
        if (term_count == 0)
          strcat(return_metadata.song_name, &temp);
        else if (term_count == 1)
          strcat(return_metadata.album_name, &temp);
        else if (term_count == 2)
          strcat(return_metadata.artist_name, &temp);
        temp_index++;
      }
      i = temp_index;
      term_count++;
    }
  }
}

void metadata_remove_tags() {

  if (strcmp(return_metadata.song_name, "")) { // Ensures we have atleast one mp3 metadata item
    if (strcmp(return_metadata.album_name, "")) {
      // Shorten by 4 characters
      return_metadata.song_name[strlen(return_metadata.song_name) - 4] = '\0';
      if (strcmp(return_metadata.artist_name, "")) {
        return_metadata.album_name[strlen(return_metadata.album_name) - 4] = '\0';
      }
    }
  }
}

bool has_metadata() {
  if (bytes_128[0] == 0xFF) {
    return false;
  } else {
    return true;
  }
}
