#pragma once

#include "gpio.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// P0.15 = Encoder A = CLK pin
// P0.18 = Encoder B = DT pin
// P0.1  = Switch    = SW
void volume_dial_init();

// P1.31 = Encoder A = CLK pin
// P1.20 = Encoder B = DT pin
// P1.28 = Switch    = SW
void menu_dial_init();

// If this returns 3: Dial is not being turned
// If this returns 2 immediately after returning 3: Dial turned Counter Clockwise
// If this returns 1 immediately after returning 3: Dial turned Clockwise
uint8_t get_vol_dial_value();
uint8_t get_menu_dial_value();

// Returns 1 if switch not pressed, 0 if pressed
uint8_t get_vol_switch_value();
uint8_t get_menu_switch_value();