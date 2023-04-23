#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

extern void EnableInterrupts(void);

#define SYSDIV2 4
#define PF2 (*((volatile uint32_t *)0x40025010))

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

// expects 16MHz clock
void timer_init(uint32_t period_us)
{
	SYSCTL_RCGCTIMER_R |= 0x01; // enable timer0 clock
	while ((SYSCTL_RCGCTIMER_R & 0x01) == 0);

	TIMER0_CTL_R = 0; // disable the timer
	TIMER0_CFG_R = 0; // configure for 32-bit mode

	TIMER0_TAMR_R = 0x02; // periodic mode
	TIMER0_TAILR_R = (period_us * 16) - 1;  // reload value
	TIMER0_ICR_R = 0x01; // clear timer0 timeout flag
	TIMER0_IMR_R |= 0x01; // enable timeout interrupt
	NVIC_PRI4_R = (NVIC_PRI4_R & 0x0FFFFFFF) | 0x40000000; // priority 2 (why it's 2 and not 4)?
	NVIC_EN0_R |= 0x80000; // enable irq 19
	TIMER0_CTL_R |= 0x01; // enable timer

	EnableInterrupts();
}

void Timer0A_Handler(void)
{
	TIMER0_ICR_R = 0x01; // ack timeout
	led_toggle();
}

int main(void)
{
	led_init();
	timer_init(500000); // 500000us = 0.5s

	while (true);
}
