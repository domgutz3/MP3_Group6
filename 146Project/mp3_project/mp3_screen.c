#include "mp3_screen.h"

void mp3_screen_put(char byte) {
  while (!uart_lab__polled_put(UART_2, byte)) {
    ;
  }

  delay__ms(5);
}

void mp3_screen_put_from_task(char byte) {
  while (!uart_lab__polled_put(UART_2, byte)) {
    ;
  }

  vTaskDelay(2);
}

static void mp3_screen_special_command(char byte) {
  mp3_screen_put(SPECIAL_COMMAND);
  mp3_screen_put(byte);
  delay__ms(50);
}

static void mp3_screen_setting_command(char byte) {
  mp3_screen_put(SETTING_COMMAND);
  mp3_screen_put(byte);
  delay__ms(10);
}

void mp3_screen_clear() { mp3_screen_setting_command(CLEAR_COMMAND); }

void mp3_screen_clear_from_task() {
  mp3_screen_put_from_task(SETTING_COMMAND);
  mp3_screen_put_from_task(CLEAR_COMMAND);
  vTaskDelay(10);
}

static void screen_init() {
  mp3_screen_special_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);
  mp3_screen_special_command(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);
  mp3_screen_clear();
  delay__ms(50);
}

static void screen_soft_reset() {
  mp3_screen_setting_command(0x08);
  delay__ms(1000);
}

static void controls_message() {
  mp3_screen_clear();
  mp3_screen_println("Left Dial Menu");
  delay__ms(5);
  mp3_screen_println("Click to Select");
  delay__ms(5);
  mp3_screen_println("Right Dial Volume");
  delay__ms(5);
  mp3_screen_println("Click to Pause/Play");
  delay__ms(3000);
  mp3_screen_clear();
}

static void create_music_note() {
  mp3_screen_put(SETTING_COMMAND);
  mp3_screen_put(0x1B);
  mp3_screen_put(0x04);
  mp3_screen_put(0x06);
  mp3_screen_put(0x05);
  mp3_screen_put(0x05);
  mp3_screen_put(0x0C);
  mp3_screen_put(0x3C);
  mp3_screen_put(0x3C);
  mp3_screen_put(0x0C);

  delay__ms(200);
}

static void mp3_screen_put_music_note() { mp3_screen_setting_command(0x23); }

song_metadata metadata[100];

static void populate_metadata() {
  for (int i = 0; i < song_list__get_item_count(); i++) {
    metadata[i] = get_metadata(song_list__get_name_for_item(i));
  }
}

void mp3_screen_init() {
  uart_lab__init(UART_2, 96 * 1000 * 1000, 9600); // TX = P2.8
  screen_soft_reset();
  screen_init();
  create_music_note();
  controls_message();
  mp3_screen_clear();
  populate_metadata();
  mp3_screen_setting_command(0x9D);
}

int to_display_volume(int volume) {
  int volume_percentage = (int)((1 - ((float)volume - 11) / 234) * 100);
  return volume_percentage;
}

// Returns false if line didn't fit the screen and will be cut off
// Perhaps we can use this to implement a horizontal scrolling functionality for long lines of text
bool mp3_screen_println(char *byte) {
  int size = strlen(byte);
  bool fit = false;
  if (size <= WIDTH) {
    fit = true;
    for (int i = 0; i < size; i++) {
      mp3_screen_put(byte[i]);
    }
    for (int i = size; i <= WIDTH - 1; i++) {
      mp3_screen_put(' ');
    }
  } else {
    for (int i = 0; i < WIDTH; i++) {
      mp3_screen_put(byte[i]);
    }
  }
  return fit;
}

bool mp3_screen_println_from_task(char *byte) {
  int size = strlen(byte);
  bool fit = false;
  if (size <= WIDTH) {
    fit = true;
    for (int i = 0; i < size; i++) {
      mp3_screen_put_from_task(byte[i]);
    }
    for (int i = size; i <= WIDTH - 1; i++) {
      mp3_screen_put_from_task(' ');
    }
  } else {
    for (int i = 0; i < WIDTH; i++) {
      mp3_screen_put_from_task(byte[i]);
    }
  }
  return fit;
}

bool mp3_screen_music_note_line_from_task(char *byte) {
  mp3_screen_put_music_note();
  mp3_screen_put_from_task(' ');
  int size = strlen(byte);
  bool fit = false;
  if (size <= WIDTH - 2) {
    fit = true;
    for (int i = 0; i < size; i++) {
      mp3_screen_put_from_task(byte[i]);
    }
    for (int i = size; i <= WIDTH - 3; i++) {
      mp3_screen_put_from_task(' ');
    }
  } else {
    for (int i = 0; i < WIDTH - 2; i++) {
      mp3_screen_put_from_task(byte[i]);
    }
  }
  return fit;
}

// Once we have screen working, change printfs to screen prints.
// Can improve this by handling cases where the end of list is reached but
// song_number - song_list__get_item_count() != 1 meaning that less than 3 songs will be displayed on the screen.
void print_idle_menu(size_t song_number, int volume) {
  char buffer[100];
  mp3_screen_clear();

  volume = to_display_volume(volume);

  if (song_list__get_item_count() == 0) {
    mp3_screen_println_from_task("   NO SONGS FOUND   ");
    mp3_screen_println_from_task("Add Songs Onto Your");
    mp3_screen_println_from_task("MicroSD Card and Try");
    mp3_screen_println_from_task("        Again       ");
  } else if (song_list__get_item_count() == 1) {
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");
    mp3_screen_println_from_task(" ");

    sprintf(buffer, "  Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);
  } else if (song_list__get_item_count() == 2 && song_number == 0) {

    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 2, song_list__get_name_for_item(song_number + 1));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");

    sprintf(buffer, "  Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);

  } else if (song_list__get_item_count() == 2 && song_number == 1) {

    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");

    sprintf(buffer, "  Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);

  } else if (song_number % 3 == 0 &&
             song_number == song_list__get_item_count() - 1) { // Last song supposed to be on top

    sprintf(buffer, "  %d) %s\n", song_number - 1, song_list__get_name_for_item(song_number - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);

  } else if (song_number % 3 == 0 && song_number == song_list__get_item_count() - 2) { // 2nd to Last

    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 2, song_list__get_name_for_item(song_number + 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);

  } else if (song_number % 3 == 0) {

    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 2, song_list__get_name_for_item(song_number + 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 3, song_list__get_name_for_item(song_number + 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);

  } else if (song_number % 3 == 1 && song_number == song_list__get_item_count() - 1) { // Last song in middle

    sprintf(buffer, "  %d) %s\n", song_number - 1, song_list__get_name_for_item(song_number - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);

  } else if (song_number % 3 == 1) {

    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 2, song_list__get_name_for_item(song_number + 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);

  } else {

    sprintf(buffer, "  %d) %s\n", song_number - 1, song_list__get_name_for_item(song_number - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);
  }
}

void print_playing_menu(size_t song_number, size_t playing) {
  char buffer[100];
  mp3_screen_clear();

  if (song_list__get_item_count() == 0) {
    mp3_screen_println_from_task("   NO SONGS FOUND   ");
    mp3_screen_println_from_task("Add Songs Onto Your");
    mp3_screen_println_from_task("MicroSD Card and Try");
    mp3_screen_println_from_task("        Again       ");
  } else if (song_list__get_item_count() == 1) {
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");
    mp3_screen_println_from_task(" ");

    sprintf(buffer, "%s", song_list__get_name_for_item(playing));
    mp3_screen_music_note_line_from_task(buffer);
  } else if (song_list__get_item_count() == 2 && song_number == 0) {

    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 2, song_list__get_name_for_item(song_number + 1));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");

    sprintf(buffer, "%s", song_list__get_name_for_item(playing));
    mp3_screen_music_note_line_from_task(buffer);

  } else if (song_list__get_item_count() == 2 && song_number == 1) {

    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");

    sprintf(buffer, "%s", song_list__get_name_for_item(playing));
    mp3_screen_music_note_line_from_task(buffer);

  } else if (song_number % 3 == 0 &&
             song_number == song_list__get_item_count() - 1) { // Last song supposed to be on top

    sprintf(buffer, "  %d) %s\n", song_number - 1, song_list__get_name_for_item(song_number - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "%s", song_list__get_name_for_item(playing));
    mp3_screen_music_note_line_from_task(buffer);

  } else if (song_number % 3 == 0 && song_number == song_list__get_item_count() - 2) { // 2nd to Last

    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 2, song_list__get_name_for_item(song_number + 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "%s", song_list__get_name_for_item(playing));
    mp3_screen_music_note_line_from_task(buffer);

  } else if (song_number % 3 == 0) {

    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 2, song_list__get_name_for_item(song_number + 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 3, song_list__get_name_for_item(song_number + 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "%s", song_list__get_name_for_item(playing));
    mp3_screen_music_note_line_from_task(buffer);

  } else if (song_number % 3 == 1 && song_number == song_list__get_item_count() - 1) { // Last song in middle

    sprintf(buffer, "  %d) %s\n", song_number - 1, song_list__get_name_for_item(song_number - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "%s", song_list__get_name_for_item(playing));
    mp3_screen_music_note_line_from_task(buffer);

  } else if (song_number % 3 == 1) {

    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number + 2, song_list__get_name_for_item(song_number + 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "%s", song_list__get_name_for_item(playing));
    mp3_screen_music_note_line_from_task(buffer);

  } else {

    sprintf(buffer, "  %d) %s\n", song_number - 1, song_list__get_name_for_item(song_number - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_number, song_list__get_name_for_item(song_number - 1));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> %d) %s\n", song_number + 1, song_list__get_name_for_item(song_number));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "%s", song_list__get_name_for_item(playing));
    mp3_screen_music_note_line_from_task(buffer);
  }
}

void print_info_selected(size_t playing) {
  char buffer[100];
  mp3_screen_clear();
  int song_count = song_list__get_item_count();

  if (song_count == 1) {
    sprintf(buffer, "  %d) %s\n", song_count, song_list__get_name_for_item(song_count - 1));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");
    mp3_screen_println_from_task(" ");

    sprintf(buffer, "? %s\n", song_list__get_name_for_item(playing));
    mp3_screen_println_from_task(buffer);
  } else if (song_count == 2) {
    sprintf(buffer, "  %d) %s\n", song_count - 1, song_list__get_name_for_item(song_count - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_count, song_list__get_name_for_item(song_count - 1));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");

    sprintf(buffer, "? %s\n", song_list__get_name_for_item(playing));
    mp3_screen_println_from_task(buffer);
  } else {
    sprintf(buffer, "  %d) %s\n", song_count - 2, song_list__get_name_for_item(song_count - 3));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_count - 1, song_list__get_name_for_item(song_count - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_count, song_list__get_name_for_item(song_count - 1));
    mp3_screen_println_from_task(buffer);

    sprintf(buffer, "? %s\n", song_list__get_name_for_item(playing));
    mp3_screen_println_from_task(buffer);
  }
}

void print_song_info(size_t playing, int selection) {
  char buffer[100];
  mp3_screen_clear();
  char data[20];

  for (int i = 0; i < 20; i++) {
    data[i] = metadata[playing].song_name[i];
  }

  sprintf(buffer, "%s\n", data);
  mp3_screen_println_from_task(buffer);

  for (int i = 0; i < 20; i++) {
    data[i] = metadata[playing].artist_name[i];
  }

  sprintf(buffer, "%s\n", data);
  mp3_screen_println_from_task(buffer);

  for (int i = 0; i < 20; i++) {
    data[i] = metadata[playing].album_name[i];
  }

  sprintf(buffer, "%s\n", data);
  mp3_screen_println_from_task(buffer);

  if (selection == 0) {
    mp3_screen_println_from_task("> back         stop");
  } else {
    mp3_screen_println_from_task("  back       > stop");
  }
}

void print_audio_selected(int volume) {
  char buffer[100];
  mp3_screen_clear();
  int song_count = song_list__get_item_count();
  volume = to_display_volume(volume);

  if (song_count == 1) {
    sprintf(buffer, "  %d) %s\n", song_count, song_list__get_name_for_item(song_count - 1));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");
    mp3_screen_println_from_task(" ");

    sprintf(buffer, "> Volume: %d\n", volume); //
    mp3_screen_println_from_task(buffer);
  } else if (song_count == 2) {
    sprintf(buffer, "  %d) %s\n", song_count - 1, song_list__get_name_for_item(song_count - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_count, song_list__get_name_for_item(song_count - 1));
    mp3_screen_println_from_task(buffer);

    mp3_screen_println_from_task(" ");

    sprintf(buffer, "> Volume: %d\n", volume); //
    mp3_screen_println_from_task(buffer);
  } else {
    sprintf(buffer, "  %d) %s\n", song_count - 2, song_list__get_name_for_item(song_count - 3));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_count - 1, song_list__get_name_for_item(song_count - 2));
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  %d) %s\n", song_count, song_list__get_name_for_item(song_count - 1));
    mp3_screen_println_from_task(buffer);

    sprintf(buffer, "> Volume: %d\n", volume);
    mp3_screen_println_from_task(buffer);
  }
}

void print_audio_menu(int selection, uint8_t volume, int8_t treble, uint8_t bass) {
  char buffer[100];
  mp3_screen_clear();
  volume = to_display_volume(volume);

  if (selection == 0) {
    sprintf(buffer, "> Volume: %d", volume);
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Treble: %d", treble);
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Bass: %d", bass);
    mp3_screen_println_from_task(buffer);
    mp3_screen_println_from_task("  Back ");
  } else if (selection == 1) {
    sprintf(buffer, "  Volume: %d", volume);
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> Treble: %d", treble);
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Bass: %d", bass);
    mp3_screen_println_from_task(buffer);
    mp3_screen_println_from_task("  Back ");
  } else if (selection == 2) {
    sprintf(buffer, "  Volume: %d", volume);
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Treble: %d", treble);
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "> Bass: %d", bass);
    mp3_screen_println_from_task(buffer);
    mp3_screen_println_from_task("  Back ");
  } else if (selection == 3) {
    sprintf(buffer, "  Volume: %d", volume);
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Treble: %d", treble);
    mp3_screen_println_from_task(buffer);
    sprintf(buffer, "  Bass: %d", bass);
    mp3_screen_println_from_task(buffer);
    mp3_screen_println_from_task("> Back ");
  }
}
