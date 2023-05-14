/*
 * Video lecture by Valvano: https://youtu.be/GVEX90Buzi0
 *
 * The keypad diagram looks as follows:
 *
 * PA7—| 1 | 2 | 3 | A |
 * PA6—| 4 | 5 | 6 | B |
 * PA5—| 7 | 8 | 9 | C |
 * PB4—| * | 0 | # | D |
 *       |   |   |   |
 *      PE5 PE4 PB1 PB0
 */

#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "leds.h"
#include "pll.h"
#include "systick.h"
#include "tm4c123gh6pm.h"
#include "printf.h"
#include "ring_buffer.h"

#define ROWS 4
#define COLS 4
#define BUF_SIZE 8

extern void EnableInterrupts(void);
static char matrix[] = "123A456B789C*0#D";

static struct rg_buf rg_buf;
static char rg_buf_mem[BUF_SIZE];

void clocks_init(void)
{
	// Enable clock to Ports: A, B, E
	int clocks = SYSCTL_RCGCGPIO_R0 | SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R4;
	SYSCTL_RCGCGPIO_R |= clocks;

	// wait until the clock is stabilized
	while ((SYSCTL_RCGCGPIO_R & clocks) != clocks);
}

void rows_init(void)
{
	GPIO_PORTA_DIR_R |= 0xE0; // set PA7-5 as output
	GPIO_PORTB_DIR_R |= 0x10; // set PB4 as output

	GPIO_PORTA_AFSEL_R &= ~0xE0; // disable PA7-5 alt functionality
	GPIO_PORTB_AFSEL_R &= ~0x10; // disable PB4 alt functionality

	GPIO_PORTA_DEN_R |= 0xE0; // enable digital signal on PA7-5
	GPIO_PORTB_DEN_R |= 0x10; // enable digital signal on PB4

	GPIO_PORTA_DATA_R |= ~0xE0; // set PA7-5 to low output
	GPIO_PORTB_DATA_R |= ~0x10; // set PB4 to low output
}

void cols_init(void)
{
	// Set all cols as input, enable pull ups
	GPIO_PORTE_DIR_R &= ~0x30; // set PE5-4 as input
	GPIO_PORTB_DIR_R &= ~0x03; // set PB1-0 as input

	GPIO_PORTE_AFSEL_R &= ~0x30; // disable PE5-4 alt functionality
	GPIO_PORTB_AFSEL_R &= ~0x03; // disable PB1-0 alt functionality

	GPIO_PORTE_PUR_R |= 0x30; // enable pull-up on PE5-4 pins
	GPIO_PORTB_PUR_R |= 0x03; // enable pull up on PB1-0 pins

	GPIO_PORTE_DEN_R |= 0x30; // enable digital signal on PE5-4
	GPIO_PORTB_DEN_R |= 0x03; // enable digital signal on PB1-0

	// PORTB = interrupt #1, PORTE = interrupt #4
	NVIC_PRI1_R = (NVIC_PRI1_R & ~NVIC_PRI0_INT0_M) | 0x800; // PORTE prio 4
	NVIC_PRI0_R = (NVIC_PRI0_R & ~NVIC_PRI0_INT1_M) | 0x8000; // PORTB prio 4

	NVIC_EN0_R |= 0x02 | 0x10; // enable irq 1 & 4

	GPIO_PORTE_ICR_R = 0x03; // clear interrupt flags for PB1-0
	GPIO_PORTB_ICR_R = 0x30; // clear interrupt flags for PB1-0

	GPIO_PORTE_IM_R |= 0x30; // unmask interrupt for PE5-4 pins
	GPIO_PORTB_IM_R |= 0x03; // unmask interrupt for PB1-0 pins
}

void pins_init(void)
{
	clocks_init();
	rows_init();
	cols_init();
}

// set row to output low
void enable_row(uint8_t row)
{
	if (row == 3) {
		// PB4 = row #3
		GPIO_PORTB_DIR_R |= 0x10;
		GPIO_PORTB_DATA_R &= ~0x10;
	} else {
		// PA7 = row #0, PA6 = row #1, PA5 = row #2
		GPIO_PORTA_DIR_R |= ( 1 << (7 - row));
		GPIO_PORTA_DATA_R &= ~(1 << (7 - row));
	}
}

void enable_rows(int8_t rows)
{
	while (rows >= 0)
		enable_row(--rows);
}

// set row to input (HiZ output)
void disable_row(uint8_t row)
{
	if (row == 3) {
		// PB4 = row #3
		GPIO_PORTB_DIR_R &= ~0x10;
	} else {
		// PA7 = row #0, PA6 = row #1, PA5 = row #2
		GPIO_PORTA_DIR_R &= ~(1 << (7 - row));
	}
}

void disable_rows(int8_t rows)
{
	while (rows >= 0)
		disable_row(--rows);
}

// return true if the given col is logical high(low voltage)
bool scan_col(uint8_t col)
{
	if (col == 0 || col == 1) {
		// PE5 = col 0, PE4 = col 1
		return !(GPIO_PORTE_DATA_R & (1 << (5 - col)));
	}

	// PB1 = col 2, PB2 = col 3
	return !(GPIO_PORTB_DATA_R & (1 << (1 - col % 2)));
}

/*
 * Disable the rows first, so we can determine which exactly row triggered the
 * button press irq. Then scan all the rows one by one. Re-enable rows so the
 * next interrupt can be handled.
 */
void scan_keypad(void)
{
	uint8_t row = 0;
	uint8_t col = 0;

	// ignore the char if there is no space for it
	if (rg_buf_is_full(&rg_buf))
		return;

	disable_rows(ROWS);

	for (row = 0; row < ROWS; row++) {
		enable_row(row);

		/* we should wait here for 1ms because the output of the row may
		 * not be stabilized by the time we read the col because of
		 * capacitance of the circuit lines
		 * refer to: https://youtu.be/GVEX90Buzi0?t=225
		 */
		systick_wait_1ms(1);

		for (col = 0; col < COLS; col++) {
			if (!scan_col(col))
				continue;

			rg_buf_put_char(&rg_buf, matrix[row * COLS + col]);
		}

		disable_row(row);
	}

	enable_rows(ROWS);
}

void GpioPortB_Handler(void)
{
	scan_keypad();
	GPIO_PORTB_ICR_R = 0x03; // clear PORTB interrupt status
}

void GpioPortE_Handler(void)
{
	scan_keypad();
	GPIO_PORTE_ICR_R = 0x30; // clear PORTE interrupt status
}

int main(void)
{
	struct rg_buf_attr buf_attr = { rg_buf_mem, BUF_SIZE };

	leds_init_builtin();
	pll_init_80mhz();
	uart_init();
	systick_init();
	rg_buf_init(&rg_buf, &buf_attr);

	pins_init();

	printf("\nThis is driver for matrix keypad\n");

	while (true) {
		char c;

		if (rg_buf_get_char(&rg_buf, &c))
			continue;

		printf("%c", c);
	}
}
