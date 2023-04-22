#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define PF2 (*((volatile uint32_t *)0x40025010))

extern void EnableInterrupts(void);

// BLUE LED - PF2
void led_init(void)
{
	SYSCTL_RCGCGPIO_R |= 0x20; // Enable clock to Port F
	while ((SYSCTL_RCGCGPIO_R & 0x20) == 0);

	GPIO_PORTF_DIR_R |= 0x04;
	GPIO_PORTF_DEN_R |= 0x04;
}

void led_toggle(void)
{
	PF2 = PF2 ^ 0x04;
}

// SW1 - PF4
void button_init(void)
{
	SYSCTL_RCGCGPIO_R |= 0x20; // activate clock for port F
	while ((SYSCTL_RCGCGPIO_R & 0x20) == 0);

	GPIO_PORTF_DIR_R &= ~0x10; // make PF4 in (build-in button)
	GPIO_PORTF_AFSEL_R &= ~0x10; // disable alt func
	GPIO_PORTF_DEN_R |= 0x10; // enable digital IO
	GPIO_PORTF_PCTL_R &= ~0x000F0000; // configure PF4 as GPIO
	GPIO_PORTF_AMSEL_R &= ~0x10; // disable analog functionality
	GPIO_PORTF_PUR_R |= 0x10; // enable weak pull up on PF4
	GPIO_PORTF_IS_R &= ~0x10; // configure as edge sensitive
	GPIO_PORTF_IBE_R &= ~0x10; // is not both edged
	GPIO_PORTF_IEV_R &= ~0x10; // falling edge
	GPIO_PORTF_ICR_R = 0x10; // clear interrupt flag
	GPIO_PORTF_IM_R |= 0x10; // arm the interrupt
	NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF) | 0x00A00000; // priority 5
	NVIC_EN0_R = 0x40000000; // enable interrupt 30 in NVIC

	EnableInterrupts();
}

void GpioPortFhandler(void)
{
	GPIO_PORTF_ICR_R = 0x10;
	led_toggle();
}

int main(void)
{
	led_init();
	button_init();

	while (true) {
	}
}
