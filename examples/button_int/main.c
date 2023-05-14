#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define PF2_3 (*((volatile uint32_t *)0x40025030))
#define UNLOCK_PORT_VAL 0x4C4F434B // page #684 in datasheet

extern void EnableInterrupts(void);

void button_int_arm(void);
void button_int_disarm(void);

// PF2 pin - blue
// PF3 pin - green
void leds_init(void)
{
	SYSCTL_RCGCGPIO_R |= 0x20; // Enable clock to Port F
	while ((SYSCTL_RCGCGPIO_R & 0x20) == 0);

	GPIO_PORTF_DIR_R |= 0x0C;
	GPIO_PORTF_DEN_R |= 0x0C;
}

void led_toggle_blue(void)
{
	PF2_3 = PF2_3 ^ 0x04;
}

void led_toggle_green(void)
{
	PF2_3 = PF2_3 ^ 0x08;
}

void timer_arm()
{
	TIMER0_ICR_R = 0x01; // clear timer0 timeout flag
	TIMER0_IMR_R |= 0x01; // enable timeout interrupt
	TIMER0_CTL_R |= 0x01; // enable timer
}

void timer_disarm()
{
	TIMER0_IMR_R &= ~0x01; // mask timeout interrupt
}

// expects 16MHz clock
void timer_init()
{
	SYSCTL_RCGCTIMER_R |= 0x01; // enable timer0 clock
	while ((SYSCTL_RCGCTIMER_R & 0x01) == 0);

	TIMER0_CTL_R = 0; // disable the timer
	TIMER0_CFG_R = 0; // configure for 32-bit mode

	TIMER0_TAMR_R = 0x01; // one shot mode
	TIMER0_TAILR_R = (200 * 1000 * 16) - 1;  // reload value - 200ms
	TIMER0_ICR_R = 0x01; // clear timer0 timeout flag
	NVIC_PRI4_R = (NVIC_PRI4_R & 0x0FFFFFFF) | 0x40000000; // priority 2
	NVIC_EN0_R |= 0x80000; // enable irq 19
}

void Timer0A_Handler(void)
{
	timer_disarm();
	button_int_arm();
}

void button_int_arm(void)
{
	GPIO_PORTF_ICR_R = 0x11; // clear interrupt flag
	GPIO_PORTF_IM_R |= 0x11;
}

void button_int_disarm(void)
{
	GPIO_PORTF_IM_R &= ~0x11;
}

// SW1 - PF4
// SW2 - PF0
void buttons_init(void)
{
	SYSCTL_RCGCGPIO_R |= 0x20; // activate clock for port F
	while ((SYSCTL_RCGCGPIO_R & 0x20) == 0);

	GPIO_PORTF_LOCK_R = UNLOCK_PORT_VAL; // unlock GPIOCR register
	GPIO_PORTF_CR_R = 0xFF; // enable commit register for all bits in PORT F

	GPIO_PORTF_DIR_R &= ~0x11; // make PF4 & PF0 in (build-in button)
	GPIO_PORTF_AFSEL_R &= ~0x11; // disable alt func
	GPIO_PORTF_DEN_R |= 0x11; // enable digital IO
	GPIO_PORTF_PCTL_R &= ~0x000F000F; // configure PF4 & PF0 as GPIO
	GPIO_PORTF_AMSEL_R &= ~0x11; // disable analog functionality
	GPIO_PORTF_PUR_R |= 0x11; // enable weak pull up on PF4
	GPIO_PORTF_IS_R &= ~0x11; // configure as edge sensitive
	GPIO_PORTF_IBE_R &= ~0x11; // is not both edged
	GPIO_PORTF_IEV_R &= ~0x11; // falling edge
	GPIO_PORTF_ICR_R = 0x11; // clear interrupt flag
	button_int_arm();
	NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF) | 0x00A00000; // priority 5
	NVIC_EN0_R = 0x40000000; // enable interrupt 30 in NVIC

	EnableInterrupts();
}

void GpioPortFhandler(void)
{
	button_int_disarm();

	if (GPIO_PORTF_RIS_R & 0x10) { // SW1 - PF4 pressed
		GPIO_PORTF_ICR_R |= 0x10;
		led_toggle_blue();
	} else if (GPIO_PORTF_RIS_R & 0x01) { // SW2 - PF0 pressed
		GPIO_PORTF_ICR_R |= 0x01;
		led_toggle_green();
	} else {
		// error: who the hell triggered the interrupt?
	}

	timer_arm(); // arm one shot timer
}

int main(void)
{
	leds_init();
	timer_init();
	buttons_init();

	EnableInterrupts();
	while (true);
}
