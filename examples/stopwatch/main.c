#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "pll.h"
#include "tm4c123gh6pm.h"
#include "printf.h"

extern void EnableInterrupts(void);

static volatile uint16_t mseconds;
static volatile uint8_t seconds;
static volatile uint8_t minutes;
static volatile uint8_t hours;

void timer_init(uint32_t period_us, uint8_t bus_fq_mhz)
{
	SYSCTL_RCGCTIMER_R |= 0x01; // enable timer0 clock
	while ((SYSCTL_RCGCTIMER_R & 0x01) == 0);

	TIMER0_CTL_R = 0; // disable the timer
	TIMER0_CFG_R = 0; // configure for 32-bit mode

	TIMER0_TAMR_R = 0x02; // periodic mode
	TIMER0_TAILR_R = (period_us * bus_fq_mhz) - 1;  // reload value
	TIMER0_ICR_R = 0x01; // clear timer0 timeout flag
	TIMER0_IMR_R |= 0x01; // enable timeout interrupt
	NVIC_PRI4_R = (NVIC_PRI4_R & 0x0FFFFFFF) | 0x40000000; // priority 2
	NVIC_EN0_R |= 0x80000; // enable irq 19
	TIMER0_CTL_R |= 0x01; // enable timer

	EnableInterrupts();
}

void Timer0_IntHandler(void)
{
	TIMER0_ICR_R = 0x01; // ack timeout

	if (mseconds == 999) {
		mseconds = 0;
		seconds++;
	} else {
		mseconds++;
	}

	if (seconds == 60) {
		seconds = 0;
		minutes++;
	}

	if (minutes == 60) {
		minutes = 0;
		hours++;
	}
}

void disable_timer_irq(void)
{
	TIMER0_IMR_R &= ~0x01;
}

void enable_timer_irq(void)
{
	TIMER0_IMR_R |= 0x01;
}

int main(void)
{
	uint16_t tmp_mseconds;
	uint8_t tmp_seconds;
	uint8_t tmp_minutes;
	uint8_t tmp_hours;

	pll_init_80mhz();
	uart_init();
	timer_init(1000 * 1, 80); // 1ms

	while (true) {
		// Read stopwatch data to tmp values so interrupts can be
		// re-enabled
		disable_timer_irq();
		tmp_mseconds = mseconds;
		tmp_seconds = seconds;
		tmp_minutes = minutes;
		tmp_hours = hours;
		enable_timer_irq();

		printf("%d:%d:%d:%d\n", tmp_hours, tmp_minutes,
		       tmp_seconds, tmp_mseconds);
	}
}
