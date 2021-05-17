#pragma once

#include "delay.h"
#include "lpc40xx.h"
#include "ssp2.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

void mp3_increase_volume();

void mp3_decrease_volume();

void mp3_set_volume(int percent);

void mp3_direct_volume(uint8_t value);

void mp3_set_bass(uint8_t value);

void mp3_set_treble(int8_t value);

void mp3_init();

uint8_t ssp2_exchange_byte(uint8_t data_out);

bool mp3_decoder_needs_data();

void spi_send_to_mp3_decoder(char *byte);

uint16_t mp3_read_SCI(uint8_t address);

void mp3_write_SCI(uint8_t address, uint16_t data);
