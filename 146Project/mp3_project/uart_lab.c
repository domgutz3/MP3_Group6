#include "uart_lab.h"

// Private queue handle of our uart_lab.c
static QueueHandle_t your_uart_rx_queue;

static void your_receive_interrupt(void) {
  uint32_t INTSTATUS = (1 << 0);
  bool uart2 = false;
  bool uart3 = false;
  if (!(LPC_UART2->IIR & INTSTATUS)) {
    uart2 = true;
  } else if (!(LPC_UART3->IIR & INTSTATUS)) {
    uart3 = true;
  }
  char byte;
  if (uart2 && (LPC_UART2->LSR & (1 << 0))) {
    byte = (LPC_UART2->RBR & 0xFF);
  }
  if (uart3 && (LPC_UART3->LSR & (1 << 0))) {
    byte = (LPC_UART3->RBR & 0xFF);
  }

  xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
}

void power_on_uart(uart_number_e uart) {
  uint32_t PCUART_ = (1 << (24 + uart));
  LPC_SC->PCONP |= PCUART_;
}

void set_baud_rate(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  uint32_t DLAB = (1 << 7);
  uint16_t DL_value = peripheral_clock / (baud_rate * 16);
  uint8_t DLL_value = ((DL_value >> 0) & 0xFF);
  uint8_t DLM_value = ((DL_value >> 8) & 0xFF);

  if (uart == UART_2) {
    LPC_UART2->LCR |= DLAB;
    LPC_UART2->DLL = DLL_value;
    LPC_UART2->DLM = DLM_value;
    LPC_UART2->FDR = (0 << 0) | (1 << 4);
    LPC_UART2->LCR &= ~DLAB;
  }
  if (uart == UART_3) {
    LPC_UART3->LCR |= DLAB;
    LPC_UART3->DLL = DLL_value;
    LPC_UART3->DLM = DLM_value;
    LPC_UART3->FDR = (0 << 0) | (1 << 4);
    LPC_UART3->LCR &= ~DLAB;
  }
}

void configure_pins(uart_number_e uart) {
  if (uart == UART_2) {
    LPC_IOCON->P2_8 &= ~(0b111 << 0); // U2_TXD
    LPC_IOCON->P2_9 &= ~(0b111 << 0); // U2_RXD
    LPC_IOCON->P2_8 |= (0b010 << 0);
    LPC_IOCON->P2_9 |= (0b010 << 0);
  }
  if (uart == UART_3) {
    LPC_IOCON->P0_25 &= ~(0b111 << 0); // U3_TXD
    LPC_IOCON->P0_26 &= ~(0b111 << 0); // U3_RXD
    LPC_IOCON->P0_25 |= (0b011 << 0);
    LPC_IOCON->P0_26 |= (0b011 << 0);
  }
}

void set_number_of_bits(uart_number_e uart) {
  if (uart == UART_2) {
    LPC_UART2->LCR = 0x3 << 0;
  }
  if (uart == UART_3) {
    LPC_UART3->LCR = 0x3 << 0;
  }
}

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  power_on_uart(uart);
  set_baud_rate(uart, peripheral_clock, baud_rate);
  configure_pins(uart);
  set_number_of_bits(uart);
}

bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  bool ready = false;
  if (uart == UART_2) {
    ready = (LPC_UART2->LSR & (1 << 0));
    if (ready) {
      *input_byte = (LPC_UART2->RBR & 0xFF);
    }
  }
  if (uart == UART_3) {
    ready = (LPC_UART3->LSR & (1 << 0));
    if (ready) {
      *input_byte = (LPC_UART3->RBR & 0xFF);
    }
  }
  return ready;
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  bool empty = false;
  if (uart == UART_2) {
    empty = (LPC_UART2->LSR & (1 << 5));
    if (empty) {
      LPC_UART2->THR = output_byte;
    }
  }
  if (uart == UART_3) {
    empty = (LPC_UART3->LSR & (1 << 5));
    if (empty) {
      LPC_UART3->THR = output_byte;
    }
  }
  return empty;
}

void uart__enable_receive_interrupt(uart_number_e uart_number) {
  uint32_t RBRIE = (1 << 0);
  if (uart_number == UART_2) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, your_receive_interrupt, "unused");
    LPC_UART2->IER |= RBRIE;
  }
  if (uart_number == UART_3) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, your_receive_interrupt, "unused");
    LPC_UART3->IER |= RBRIE;
  }

  your_uart_rx_queue = xQueueCreate(10, sizeof(char));
}

bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(your_uart_rx_queue, input_byte, timeout);
}