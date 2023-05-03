#include <stdint.h>

#include "leds.h"
#include "../inc/tm4c123gh6pm.h"

#define PF2_3 (*((volatile uint32_t *)0x40025030))

void leds_init_builtin(void)
{
	SYSCTL_RCGCGPIO_R |= 0x20; // Enable clock to Port F
	while ((SYSCTL_RCGCGPIO_R & 0x20) == 0);

	GPIO_PORTF_DIR_R |= 0x0C;
	GPIO_PORTF_DEN_R |= 0x0C;
}

void leds_green_toggle(void)
{
	PF2_3 = PF2_3 ^ 0x08;
}

void leds_green_on(void)
{
	PF2_3 = PF2_3 | 0x08;
}

void leds_green_off(void)
{
	PF2_3 = PF2_3 & ~0x08;
}

void leds_blue_toggle(void)
{
	PF2_3 = PF2_3 ^ 0x04;
}

void leds_blue_on(void)
{
	PF2_3 = PF2_3 | 0x04;
}

void leds_blue_off(void)
{
	PF2_3 = PF2_3 & ~0x04;
}
