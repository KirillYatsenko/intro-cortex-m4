#include <stdint.h>
#include <stdbool.h>

#include "systick.h"
#include "uart.h"
#include "pll.h"
#include "tm4c123gh6pm.h"
#include "printf.h"
#include "nokia5110.h"

#define ESC 0x1B

extern void EnableInterrupts(void);

static volatile uint16_t MSECONDS;
static volatile uint8_t SECONDS;
static volatile uint8_t MINUTES;
static volatile uint8_t HOURS;

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

	if (MSECONDS == 999) {
		MSECONDS = 0;
		SECONDS++;
	} else {
		MSECONDS++;
	}

	if (SECONDS == 60) {
		SECONDS = 0;
		MINUTES++;
	}

	if (MINUTES == 60) {
		MINUTES = 0;
		HOURS++;
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

// naive implementation
char *atoi(unsigned val, char str[])
{
	unsigned i = 0;
	unsigned j = 0;
	char tmp = 0;

	if (val == 0) {
		str[0] = '0';
		str[1] = '\0';
		return str;
	}

	while (val) {
		str[j++] = (val % 10) + '0'; // get last digit
		val /= 10; // trim last digit
	}

	str[j] = '\0';
	j--; // should point to the last elem

	for (i = 0; i < j; i++, j--) {
		tmp = str[i];
		str[i] = str[j];
		str[j] = tmp;
	}

	return str;
}

void print_display(uint16_t ms, uint8_t s, uint8_t m, uint8_t h)
{
	char atoi_str[10];

	nokia5110_clear_row(3);
	nokia5110_set_col(3);

	nokia5110_write_str(atoi(h, atoi_str));
	nokia5110_write_str(":");

	nokia5110_write_str(atoi(m, atoi_str));
	nokia5110_write_str(":");

	nokia5110_write_str(atoi(s, atoi_str));
	nokia5110_write_str(":");

	nokia5110_write_str(atoi(ms, atoi_str));
}

int main(void)
{
	uint16_t tmp_ms;
	uint8_t tmp_s;
	uint8_t tmp_m;
	uint8_t tmp_h;

	pll_init_80mhz();
	systick_init();
	uart_init();

	nokia5110_init();
	nokia5110_clear_display();
	nokia5110_set_row(1);
	nokia5110_write_str("  hh:mm:ss:ms");

	timer_init(1000 * 1, 80); // 1ms

	while (true) {
		// Read stopwatch data to tmp values so interrupts can be
		// re-enabled
		disable_timer_irq();
		tmp_ms = MSECONDS;
		tmp_s = SECONDS;
		tmp_m = MINUTES;
		tmp_h = HOURS;
		enable_timer_irq();

		print_display(tmp_ms, tmp_s, tmp_m, tmp_h);

		// read: https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
		printf("%c[2K", ESC); // clear the line
		printf("%c[0G", ESC); // move cursor to the first column

		printf("%d:%d:%d:%d", tmp_h, tmp_m,
		       tmp_s, tmp_ms);

		// give user time to recognize the values before clearing the
		// line, otherwise it would be too quick
		systick_wait_10ms(2);
	}
}
