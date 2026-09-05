#ifndef GLOBAL_DATA_H_
#define GLOBAL_DATA_H_

#include <stdint.h>

#define UART_BUFFER_SIZE 128

extern uint8_t g_uart_send_buffer[UART_BUFFER_SIZE];
extern uint8_t g_uart_receive_buffer[UART_BUFFER_SIZE];

#endif
