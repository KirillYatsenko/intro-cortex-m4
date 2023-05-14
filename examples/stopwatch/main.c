#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "leds.h"
#include "pll.h"
#include "systick.h"
#include "tm4c123gh6pm.h"
#include "printf.h"

extern void EnableInterrupts(void);
extern void DisableInterrupts(void);

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

static volatile uint16_t mseconds;
static volatile uint8_t seconds;
static volatile uint8_t minutes;
static volatile uint8_t hours;

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

static char str[] = "Beyond the horizon of the place we lived when we were young\n" \
		    "In a world of magnets and miracles\n" \
		    "Our thoughts strayed constantly and without boundary\n" \
		    "The ringing of the division bell had begun\n" \
		    "Along the Long Road and on down the Causeway\n" \
		    "Do they still meet there by the Cut\n" \
		    "There was a ragged band that followed in our footsteps\n" \
		    "Running before time took our dreams away\n" \
		    "Leaving the myriad small creatures trying to tie us to the ground\n" \
		    "To a life consumed by slow decay\n\n";
int main(void)
{
// 	uint16_t tmp_mseconds;
// 	uint8_t tmp_seconds;
// 	uint8_t tmp_seconds_prev = 0;
// 	uint8_t tmp_minutes;
// 	uint8_t tmp_hours;

	pll_init_80mhz();
	uart_init();
	systick_init();
	timer_init(1000 * 1, 80); // 1ms

	printf("%s\n", str);

	while (true) {
		printf("%d:%d:%d:%d\n", hours, minutes,
		       seconds, mseconds);

		// Read stopwatch data to tmp values so interrupts can be
		// re-enabled
// 		DisableInterrupts();
// 		tmp_mseconds = mseconds;
// 		tmp_seconds = seconds;
// 		tmp_minutes = minutes;
// 		tmp_hours = hours;
// 
// 		printf("%d:%d:%d:%d\n", tmp_hours, tmp_minutes,
// 		       tmp_seconds, tmp_mseconds);
// 
// 		EnableInterrupts();
// 
// 		systick_wait_10ms(1);
	}
}

