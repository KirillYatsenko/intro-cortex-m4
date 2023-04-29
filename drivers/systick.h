#ifndef _SYSTICK_H
#define _SYSTICK_H

#include <stdint.h>

// expects 80MHz bus clock
void systick_init(void);
void systick_wait_10ms(uint32_t delay);

#endif
