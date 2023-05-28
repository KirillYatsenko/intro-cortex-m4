#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

// use PE3 as ADC input
void adc_init(void);

uint32_t adc_read(void);

#endif
