#include "mp3_spi.h"

static const uint32_t mp3_XDCS = (1 << 1);
static const uint32_t mp3_CS = (1 << 7);
static const uint32_t mp3_DREQ = (1 << 6);
static const uint32_t mp3_RST = (1 << 4);

static void mp3_SDI_activate() { LPC_GPIO2->PIN &= ~mp3_XDCS; }

static void mp3_SDI_deactivate() { LPC_GPIO2->PIN |= mp3_XDCS; }

static void mp3_SCI_activate() { LPC_GPIO2->PIN &= ~mp3_CS; }

static void mp3_SCI_deactivate() { LPC_GPIO2->PIN |= mp3_CS; }

static void mp3_iocon_pins() {
  LPC_IOCON->P2_1 &= 0b000; // XDCS (output)
  LPC_GPIO2->DIR |= mp3_XDCS;
  mp3_SDI_deactivate();

  LPC_IOCON->P2_7 &= 0b000; // CS (output)
  LPC_GPIO2->DIR |= mp3_CS;
  mp3_SCI_deactivate();

  LPC_IOCON->P2_6 &= 0b000; // DREQ (input)
  LPC_GPIO2->DIR &= ~mp3_DREQ;

  LPC_IOCON->P2_4 &= 0b000; // RST (output)
  LPC_GPIO2->DIR |= mp3_RST;
  LPC_GPIO2->SET |= mp3_RST; // CLR
  mp3_SCI_deactivate();
}

static void mp3_softreset(void) {
  mp3_write_SCI(0x0, 0x0804);
  delay__ms(100);
}

static void mp3_reset(void) {
  if ((LPC_GPIO2->PIN & mp3_RST)) {
    LPC_GPIO2->CLR |= mp3_RST;
    delay__ms(100);
  }
  LPC_GPIO2->SET |= mp3_RST;
  delay__ms(100);
  mp3_softreset();
  delay__ms(100);

  mp3_write_SCI(0x3, 0x6000);
}

void mp3_increase_volume() {
  const uint16_t current_volume = mp3_read_SCI(0xB);
  const uint8_t step = 0x0F;
  uint8_t current_left = (current_volume >> 8);
  uint8_t current_right = (current_volume & 0xFF);
  uint8_t new_left;
  uint8_t new_right;
  uint16_t new_volume;

  if (current_left < step) {
    new_left = 0x00;
  } else {
    new_left = current_left - step;
  }

  if (current_right < step) {
    new_right = 0x00;
  } else {
    new_right = current_right - step;
  }

  new_volume = (new_left << 8) | new_right;

  if (current_volume != 0x0000) {
    mp3_write_SCI(0xB, new_volume);
    printf("New Volume: %x\n", new_volume);
  }
}

void mp3_decrease_volume() {
  const uint16_t current_volume = mp3_read_SCI(0xB);
  const uint8_t step = 0x0F;
  uint8_t current_left = (current_volume >> 8);
  uint8_t current_right = (current_volume & 0xFF);
  uint8_t new_left;
  uint8_t new_right;
  uint16_t new_volume;
  uint16_t incremented;

  incremented = current_left + step;
  if (incremented > 0xFF) {
    new_left = 0xFF;
  } else {
    new_left = incremented;
  }

  incremented = current_right + step;
  if (incremented > 0xFF) {
    new_right = 0xFF;
  } else {
    new_right = incremented;
  }

  new_volume = (new_left << 8) | new_right;

  if (current_volume != 0xFFFF) {
    mp3_write_SCI(0xB, new_volume);
    printf("New D Volume: %x\n", new_volume);
  }
}

void mp3_set_volume(int percent) {
  float attentuation = 0xFF * (1 - ((float)percent / 100));
  attentuation = floor(attentuation);
  uint16_t volume = ((uint8_t)attentuation | ((uint8_t)attentuation << 8));
  mp3_write_SCI(0xB, volume);
}

void mp3_direct_volume(uint8_t value) {
  while (!mp3_decoder_needs_data()) {
    ;
  }
  mp3_write_SCI(0xB, (value | (value << 8)));
}

static void mp3_default_volume(int level) { mp3_set_volume(80); }

void mp3_init() {
  ssp2__initialize(1000);
  mp3_iocon_pins();
  mp3_reset();
  mp3_default_volume(80);
}

static void wait_while_ssp2_busy() {
  const uint32_t busy = (1 << 4);
  while (LPC_SSP2->SR & busy) {
    ;
  }
}

uint8_t ssp2_exchange_byte(uint8_t data_out) {
  LPC_SSP2->DR = data_out;
  wait_while_ssp2_busy();
  return LPC_SSP2->DR;
}

bool mp3_decoder_needs_data() { return (LPC_GPIO2->PIN & mp3_DREQ); }

void spi_send_to_mp3_decoder(char *byte) {
  mp3_SDI_activate();
  for (int i = 0; i < 32; i++) {
    ssp2_exchange_byte(*byte);
    byte = byte + 1;
  }
  mp3_SDI_deactivate();
}

uint16_t mp3_read_SCI(uint8_t address) {
  mp3_SCI_activate();
  uint16_t data = 0;
  uint8_t op_code = 0x03;
  uint8_t dummyByte = 0xFF;
  ssp2_exchange_byte(op_code);
  ssp2_exchange_byte(address);
  data |= ssp2_exchange_byte(dummyByte);
  data = data << 8;
  data |= ssp2_exchange_byte(dummyByte);
  mp3_SCI_deactivate();
  return data;
}

void mp3_write_SCI(uint8_t address, uint16_t data) {
  mp3_SCI_activate();
  uint8_t LSByte = (data & 0xFF);
  uint8_t MSByte = (data >> 8);
  uint8_t op_code = 0x02;
  ssp2_exchange_byte(op_code);
  ssp2_exchange_byte(address);
  ssp2_exchange_byte(MSByte);
  ssp2_exchange_byte(LSByte);
  mp3_SCI_deactivate();
}

void mp3_set_bass(uint8_t value) {
  uint8_t bass_address = 0x02;
  uint16_t bass_bytes = (0b1111 << 4);
  uint16_t current_setting;
  current_setting = mp3_read_SCI(bass_address);
  current_setting &= ~bass_bytes;
  current_setting |= value << 4;
  mp3_write_SCI(bass_address, current_setting);
}

// May cause error
void mp3_set_treble(int8_t value) {
  uint8_t treble_address = 0x02;
  uint16_t treble_bytes = (0b1111 << 12);
  uint16_t current_setting;
  current_setting = mp3_read_SCI(treble_address);
  current_setting &= ~treble_bytes;
  current_setting |= value << 12;
  mp3_write_SCI(treble_address, current_setting);
}