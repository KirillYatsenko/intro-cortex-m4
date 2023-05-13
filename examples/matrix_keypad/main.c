#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "leds.h"
#include "pll.h"
#include "systick.h"
#include "tm4c123gh6pm.h"
#include "printf.h"

// Video lecture by Valvano: https://youtu.be/GVEX90Buzi0

/* The keypad diagram looks as follows:
 *
 * PA7—| 1 | 2 | 3 | A |
 * PA6—| 4 | 5 | 6 | B |
 * PA5—| 7 | 8 | 9 | C |
 * PB4—| * | 0 | # | D |
 *       |   |   |   |
 *      PE5 PE4 PB1 PB0
 */

#define ROWS 4
#define COLS 4

static char matrix[] = "123A456B789C*0#D";

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
	// Set all rows as HiZ(set the pin to the input mode)
	GPIO_PORTA_DIR_R &= ~0xE0; // set PAT7-5 as input
	GPIO_PORTB_DIR_R &= ~0x10; // set PB4 as input

	GPIO_PORTA_AFSEL_R &= ~0xE0; // disable PA7-5 alt functionality
	GPIO_PORTB_AFSEL_R &= ~0x10; // disable PB4 alt functionality

	GPIO_PORTA_DEN_R |= 0xE0; // enable digital signal on PA7-5
	GPIO_PORTB_DEN_R |= 0x10; // enable digital signal on PB4
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

void start_scanning(void)
{
	uint8_t row = 0;
	uint8_t col = 0;

	while (true) {
		enable_row(row % ROWS);

		// refer to: https://youtu.be/GVEX90Buzi0?t=225
		systick_wait_1ms(1);

		for (col = 0; col < COLS; col++) {
			if (!scan_col(col))
				continue;

			printf("%c", matrix[(row % ROWS) * 4 + col]);
		}

		disable_row((row++) % ROWS);

		systick_wait_10ms(1); // wait 10ms before scanning again
	}
}

int main(void)
{
	leds_init_builtin();
	pll_init_80mhz();
	uart_init();
	systick_init();

	pins_init();

	printf("\nThis is driver for matrix keypad\n");

	start_scanning();
}
