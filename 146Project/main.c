#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sensors.h"
#include "sj2_cli.h"

#include "ff.h"
#include "mp3_screen.h"
#include "mp3_spi.h"
#include "queue.h"
#include "rotary_dial.h"
#include "song_list.h"

typedef char songname_t[32];
bool prev_paused;
bool paused;

bool skipped;
bool stop;

bool display_info;

bool audio_screen;
int audio_screen_selection;
bool audio_selected_printed;

int prev_vol_switch_value;
int prev_menu_switch_value;

size_t song_number;
size_t playing;
uint8_t volume;
int8_t treble;
uint8_t bass;

typedef enum {
  VOLUME,
  TREBLE,
  BASS,
} audio_type;

audio_type current_audio_type;

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;
SemaphoreHandle_t decoder_mutex;

TaskHandle_t player_handle;

FIL file;

static void create_uart_task(void);
static void uart_task(void *params);

void volume_task(void *p) {
  uint8_t prev_value = 3;
  uint8_t next_value = 3;
  uint8_t prev_volume = 50;
  int8_t prev_treble = 0;
  uint8_t prev_bass = 0;
  const uint8_t min_volume = 255;
  const uint8_t v_step = 13;
  const int8_t t_step = 1;
  const uint8_t b_step = 1;

  while (1) {
    next_value = get_vol_dial_value();
    vTaskDelay(10);
    if (prev_value == 3 && next_value == 1) { // Dial Turned Clockwise (increase)
      if (xSemaphoreTake(decoder_mutex, portMAX_DELAY)) {
        if (current_audio_type == VOLUME) {
          if (volume - v_step >= 0) {
            volume -= v_step; // Higher Value = Lower Volume
          }
          vTaskDelay(10);
          mp3_direct_volume(volume);
          vTaskDelay(10);
        } else if (current_audio_type == TREBLE) {
          if (treble + t_step <= 7) {
            treble += t_step;
          }
          vTaskDelay(10);
          mp3_set_treble(treble);
          vTaskDelay(10);
        } else if (current_audio_type == BASS) {
          if (bass + b_step <= 15) {
            bass += b_step;
          }
          vTaskDelay(10);
          mp3_set_bass(bass);
          vTaskDelay(10);
        }
        xSemaphoreGive(decoder_mutex);
      }
    } else if (prev_value == 3 && next_value == 2) {
      if (xSemaphoreTake(decoder_mutex, portMAX_DELAY)) {
        if (current_audio_type == VOLUME) {
          if (volume + v_step < min_volume) {
            volume += v_step;
          }
          vTaskDelay(10);
          mp3_direct_volume(volume);
          vTaskDelay(10);
        } else if (current_audio_type == TREBLE) {
          if (treble - t_step >= -8) {
            treble -= t_step;
          }
          vTaskDelay(10);
          mp3_set_treble(treble);
          vTaskDelay(10);
        } else if (current_audio_type == BASS) {
          if (bass - b_step >= 0) {
            bass -= b_step;
          }
          vTaskDelay(10);
          mp3_set_bass(bass);
          vTaskDelay(10);
        }
        xSemaphoreGive(decoder_mutex);
      }
    }
    prev_value = next_value;
    if (prev_volume != volume) {
      if (playing == -1 && !audio_screen && !audio_selected_printed) {
        print_idle_menu(song_number, volume);
      } else if (audio_screen) {
        print_audio_menu(audio_screen_selection, volume, treble, bass);
      } else if (audio_selected_printed) {
        print_audio_selected(volume);
      }
      prev_volume = volume;
    }
    if (prev_treble != treble) {
      if (audio_screen) {
        print_audio_menu(audio_screen_selection, volume, treble, bass);
      }
      prev_treble = treble;
    }
    if (prev_bass != bass) {
      if (audio_screen) {
        print_audio_menu(audio_screen_selection, volume, treble, bass);
      }
      prev_bass = bass;
    }
  }
}

bool check_select_button() {
  int new_value = get_menu_switch_value();
  if (prev_menu_switch_value && !new_value) {
    prev_menu_switch_value = new_value;
    return true;
  } else {
    prev_menu_switch_value = new_value;
    return false;
  }
}

void check_pause_button() {
  int new_value = get_vol_switch_value();
  if (paused && prev_vol_switch_value && !new_value) {
    paused = false;
    prev_vol_switch_value = 0;
  } else if (!paused && prev_vol_switch_value && !new_value) {
    paused = true;
    prev_vol_switch_value = 0;
  } else {
    prev_vol_switch_value = new_value;
  }
}

void pause_song() { vTaskSuspend(player_handle); }

void resume_song() { vTaskResume(player_handle); }

void cancel_song() {
  uint16_t SM_CANCEL = (1 << 3);
  char endFillByte[32];
  if (xSemaphoreTake(decoder_mutex, portMAX_DELAY)) {
    uint16_t mode = mp3_read_SCI(0x0);
    vTaskDelay(10);
    mode |= SM_CANCEL;
    mp3_write_SCI(0x0, mode);
    vTaskDelay(10);
    xSemaphoreGive(decoder_mutex);
  }
  vTaskDelay(1000);
  if (xSemaphoreTake(decoder_mutex, portMAX_DELAY)) {
    spi_send_to_mp3_decoder(endFillByte);
    spi_send_to_mp3_decoder(endFillByte);
    xSemaphoreGive(decoder_mutex);
  }
}

void screen_task(void *p) {
  uint8_t prev_value = 3;
  uint8_t next_value = 3;
  uint8_t prev_song = 0;
  prev_paused = false;
  paused = false;

  bool info_selected = false;
  bool info_selected_printed = false;
  bool info_screen = false;
  int prev_info_screen_selection = 0;
  int info_screen_selection = 0;

  bool audio_selected = false;
  audio_selected_printed = false;
  audio_screen = false;
  int prev_audio_screen_selection = 0;
  audio_screen_selection = 0;

  song_number = 0;
  playing = -1;
  vTaskDelay(200);

  print_idle_menu(song_number, volume);

  while (1) {

    if (!info_screen && !info_selected && !audio_screen && !audio_selected && check_select_button()) {
      playing = song_number;
      print_playing_menu(song_number, playing);
      xQueueSend(Q_songname, song_list__get_name_for_item(playing), portMAX_DELAY);
    }

    check_pause_button();
    if (paused && !prev_paused) {
      pause_song();
      prev_paused = true;
    } else if (!paused && prev_paused) {
      resume_song();
      prev_paused = false;
    }

    // Start of dial handling
    next_value = get_menu_dial_value();
    vTaskDelay(10);
    if (prev_value == 3 && next_value == 1) { // Dial turned clockwise
      if (info_screen && info_screen_selection == 0) {
        info_screen_selection = 1;
      } else if (info_screen) {
        // Do nothing
      } else if (playing != -1 && song_number + 1 == song_list__get_item_count() && !info_selected) {

        info_selected = true;
        print_info_selected(playing);
        info_selected_printed = true;

      } else if (audio_screen && audio_screen_selection + 1 <= 3) {
        audio_screen_selection += 1;
      } else if (playing == -1 && song_number + 1 == song_list__get_item_count() && !audio_selected) {

        audio_selected = true;
        print_audio_selected(volume);
        audio_selected_printed = true;

      } else if (song_number + 1 < song_list__get_item_count()) {
        song_number += 1;
      }
    } else if (prev_value == 3 && next_value == 2) { // Dial turned counter-clockwise
      if (info_screen && info_screen_selection == 1) {
        info_screen_selection = 0;
      } else if (info_screen) {
        // Do nothing
      } else if (info_selected) {

        info_selected = false;
        print_playing_menu(song_number, playing);
        info_selected_printed = false;

      } else if (audio_screen && audio_screen_selection - 1 >= 0) {
        audio_screen_selection -= 1;
      } else if (audio_selected) {

        audio_selected = false;
        print_idle_menu(song_number, volume);
        audio_selected_printed = false;

      } else if ((int)song_number - 1 > -1) {
        song_number -= 1;
      }
    }
    prev_value = next_value;
    // End of dial handling

    // Start of audio screen handling
    if (audio_selected && !audio_screen) {
      if (check_select_button()) {
        audio_screen = true;
        print_audio_menu(audio_screen_selection, volume, treble, bass);
      }
    }
    if (audio_screen && audio_screen_selection != prev_audio_screen_selection) {
      print_audio_menu(audio_screen_selection, volume, treble, bass);
      prev_audio_screen_selection = audio_screen_selection;
    }
    if (audio_screen && audio_screen_selection == 0) {
      current_audio_type = VOLUME;
    } else if (audio_screen && audio_screen_selection == 1) {
      current_audio_type = TREBLE;
    } else if (audio_screen && audio_screen_selection == 2) {
      current_audio_type = BASS;
    } else if (audio_screen && audio_screen_selection == 3 && check_select_button()) {
      audio_screen = false;
      audio_screen_selection = 0;
      prev_audio_screen_selection = 0;
      current_audio_type = VOLUME;
      print_audio_selected(volume);
    }
    // End of audio screen handling

    // Start of info screen handling
    // Go to metadata screen if button pressed
    if (info_selected && !info_screen) {
      if (check_select_button()) {
        info_screen = true;
        print_song_info(playing, info_screen_selection);
      }
    }
    if (info_screen && info_screen_selection != prev_info_screen_selection) {
      print_song_info(playing, info_screen_selection);
      prev_info_screen_selection = info_screen_selection;
    }
    if (info_screen && info_screen_selection == 0 && check_select_button()) {
      info_screen = false;
      info_screen_selection = 0;
      prev_info_screen_selection = 0;
      print_info_selected(playing);
    } else if (info_screen && info_screen_selection == 1 && check_select_button()) {
      resume_song();
      paused = false;
      stop = true;
      info_screen = false;
      info_selected = false;
      info_screen_selection = 0;
      prev_info_screen_selection = 0;
      info_selected_printed = false;
      print_idle_menu(song_number, volume);
    }
    // End of info screen handling

    if (prev_song != song_number && !info_selected && !audio_selected) {
      if (playing == -1) {
        print_idle_menu(song_number, volume);
        info_selected_printed = false;
      } else {
        print_playing_menu(song_number, playing);
        info_selected_printed = false;
      }
      prev_song = song_number;
    }
  }
}

void mp3_reader_task(void *p) {
  songname_t name;
  char bytes_512[512];
  FRESULT result;
  UINT bytes_read;

  while (1) {
    xQueueReceive(Q_songname, &name[0], portMAX_DELAY);
    vTaskDelay(100);

    result = f_open(&file, name, FA_READ);

    if (result == FR_OK) {
      while (!f_eof(&file)) {

        if (uxQueueMessagesWaiting(Q_songname) > 0) {
          skipped = true;
          cancel_song();
          break;
        }
        if (stop) {
          cancel_song();
          break;
        }

        f_read(&file, (void *)&bytes_512, 512, &bytes_read);
        xQueueSend(Q_songdata, &bytes_512[0], portMAX_DELAY);
      }
      f_close(&file);

      if (skipped) {
        skipped = false;
      } else if (stop) {
        playing = -1;
        stop = false;
      } else {
        playing = -1;
        print_idle_menu(song_number, volume);
      }

      vTaskDelay(1000);
    } else {
      printf("Error: file %s not found\n", name);
    }
  }
}

void mp3_player_task(void *p) {
  char bytes_512[512];
  while (1) {
    xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
    for (int i = 0; i < sizeof(bytes_512);) {
      if (xSemaphoreTake(decoder_mutex, portMAX_DELAY)) {
        if (!mp3_decoder_needs_data()) {
          vTaskDelay(1);
        } else {
          spi_send_to_mp3_decoder(&bytes_512[i]);
          i += 32;
        }
        xSemaphoreGive(decoder_mutex);
      }
    }
  }
}

int main(void) {
  create_uart_task(); // For CLI
  mp3_init();
  volume_dial_init();
  menu_dial_init();
  song_list__populate();
  mp3_screen_init();

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, 512);
  decoder_mutex = xSemaphoreCreateMutex();
  paused = false;
  current_audio_type = VOLUME;
  volume = 0x32;

  xTaskCreate(volume_task, "volume", 8000 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(screen_task, "screen", 8000 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_reader_task, "reader", 4000 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "player", 4000 / sizeof(void *), NULL, PRIORITY_MEDIUM, &player_handle);

  vTaskStartScheduler();
  return 0;
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}
