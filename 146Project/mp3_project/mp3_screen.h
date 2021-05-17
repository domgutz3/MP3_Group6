#pragma once

#include "FreeRTOS.h"
#include "delay.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "song_info.h"
#include "song_list.h"
#include "task.h"
#include "uart_lab.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define SPECIAL_COMMAND 254
#define SETTING_COMMAND 0x7C

// OpenLCD commands
#define CLEAR_COMMAND 0x2D                  // 45, -, the dash character: command to clear and home the display
#define CONTRAST_COMMAND 0x18               // Command to change the contrast setting
#define ADDRESS_COMMAND 0x19                // Command to change the i2c address
#define SET_RGB_COMMAND 0x2B                // 43, +, the plus character: command to set backlight RGB value
#define ENABLE_SYSTEM_MESSAGE_DISPLAY 0x2E  // 46, ., command to enable system messages being displayed
#define DISABLE_SYSTEM_MESSAGE_DISPLAY 0x2F // 47, /, command to disable system messages being displayed
#define ENABLE_SPLASH_DISPLAY 0x30          // 48, 0, command to enable splash screen at power on
#define DISABLE_SPLASH_DISPLAY 0x31         // 49, 1, command to disable splash screen at power on
#define SAVE_CURRENT_DISPLAY_AS_SPLASH 0x0A // 10, Ctrl+j, command to save current text on display as splash

// special commands
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

#define WIDTH 20

void mp3_screen_put(char byte);

void mp3_screen_put_from_task(char byte); // "from task" functions use vTaskDelay() instead of delay__ms()

void mp3_screen_clear();

void mp3_screen_clear_from_task();

void mp3_screen_init();

bool mp3_screen_println(char *byte);

bool mp3_screen_println_from_task(char *byte);

void print_idle_menu(size_t song_number, int volume);

void print_playing_menu(size_t song_number, size_t playing);

void print_info_selected(size_t playing);

void print_song_info(size_t playing, int selection);

void print_audio_menu(int selection, uint8_t volume, int8_t treble, uint8_t bass);