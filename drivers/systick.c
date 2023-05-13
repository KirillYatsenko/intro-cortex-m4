#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

static void systick_wait(uint32_t delay)
{
	NVIC_ST_RELOAD_R = delay - 1;
	NVIC_ST_CURRENT_R = 0; // any value written to CURRENT clears
	while ((NVIC_ST_CTRL_R & 0x10000) == 0); // waits for COUNT flag
}

void systick_wait_1ms(uint32_t delay)
{
	uint32_t i;
	for (i = 0; i < delay; i++) {
		/* expects 80MHz source clock
		 * 1/80Mhz = 12.5 ns
		 * 12.5ns * 80000 = 1ms
		 */
		systick_wait(80000); // wait 1ms
	}
}

void systick_wait_10ms(uint32_t delay)
{
	uint32_t i;
	for (i = 0; i < delay; i++) {
		/* expects 80MHz source clock
		 * 1/80Mhz = 12.5 ns
		 * 12.5ns * 800000 = 10ms
		 */
		systick_wait(800000); // wait 10ms
	}
}

void systick_init(void)
{
	NVIC_ST_CTRL_R = 0; // disable systick
	NVIC_ST_RELOAD_R = 0x00FFFFFF; // maximum reload value 2^24
	NVIC_ST_CURRENT_R = 0; // any write to current will clear it
	NVIC_ST_CTRL_R = 0x05; // enable systick with core clock
}
