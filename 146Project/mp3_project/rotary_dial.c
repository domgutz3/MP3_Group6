#include "rotary_dial.h"

gpio_s vol_enc_a = {0, 15};
gpio_s vol_enc_b = {0, 18};
gpio_s vol_switch = {0, 1};

gpio_s menu_enc_a = {1, 31};
gpio_s menu_enc_b = {1, 20};
gpio_s menu_switch = {1, 28};

void volume_dial_init() {
  gpio__set_as_input(vol_enc_a);
  gpio__set_as_input(vol_enc_b);
  gpio__set_as_input(vol_switch);
}

void menu_dial_init() {
  gpio__set_as_input(menu_enc_a);
  gpio__set_as_input(menu_enc_b);
  gpio__set_as_input(menu_switch);
}

uint8_t get_vol_dial_value() {
  uint8_t value;
  value = gpio__get(vol_enc_a) << 1;
  value |= gpio__get(vol_enc_b);
  return value;
}

uint8_t get_menu_dial_value() {
  uint8_t value;
  value = gpio__get(menu_enc_a) << 1;
  value |= gpio__get(menu_enc_b);
  return value;
}

uint8_t get_vol_switch_value() { return gpio__get(vol_switch); }

uint8_t get_menu_switch_value() { return gpio__get(menu_switch); }
