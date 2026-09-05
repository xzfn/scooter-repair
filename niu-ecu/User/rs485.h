/*
UART in half-duplex mode with a RS485 transciver chip

Connection:
    PD6 - DI and RO (or PD6 - 1k resistor - DI and RO)
    PD6 - 10k pull-up - Vcc
    PA2 - DE and RE#


*/

#ifndef RS485_H_
#define RS485_H_

#include <stdint.h>
#include <stdbool.h>

// init uart rs485 half-duplex
void rs485_init_half_duplex();

// disable both TX and RX
void rs485_disable();

// enable TX and disable RX
void rs485_enable_send();

// enable RX and disable TX
void rs485_enable_receive();

// send single byte without wait TC
void uart_send_byte(uint8_t c);

// wait TC
void uart_wait_transmission_complete();

// send frame and wait TC
void uart_send_frame(const uint8_t *buffer, uint8_t buffer_length);

// receive byte
uint8_t uart_receive_byte();

// receive has data RXNE
bool uart_receive_has_data();

// receive frame, first byte use timeout first_timeout_ms, following bytes use timeout idle_timeout_ms
int uart_receive_frame_timeout(uint8_t *out_buffer, uint8_t out_buffer_length, uint32_t first_timeout_ms, uint32_t idle_timeout_ms);

// receive and discard until line idle
void uart_wait_line_idle(uint32_t idle_timeout_ms);

#endif
