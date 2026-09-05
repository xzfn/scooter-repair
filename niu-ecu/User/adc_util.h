#ifndef ADC_UTIL_H_
#define ADC_UTIL_H_

// PC4 as ADC input

#include <stdint.h>


void adc_init();

uint16_t adc_measure();


#endif