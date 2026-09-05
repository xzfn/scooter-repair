#ifndef USER_COMMON_H_
#define USER_COMMON_H_

#include <stdint.h>

// milliseconds since start
extern volatile uint32_t g_ms;

// init tim2 timer. use g_ms or get_ms() get the milliseconds since start
void init_tim2_timer_interrupt();

// milliseconds since start
uint32_t get_ms();

// delay ms
void delay_ms(uint32_t ms);

#endif
