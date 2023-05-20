
#include <stdint.h>
#include <stdbool.h>

#include "nokia5110.h"
#include "systick.h"
#include "pll.h"
#include "spi.h"
#include "uart.h"
#include "printf.h"
#include "../inc/tm4c123gh6pm.h"

#define MAIN_CLK_FQ_MHZ		80
#define SSI_CLK_FQ_MHZ		4

static uint8_t ROW;	// 48/8 = 6 rows
static uint8_t COLUMN;  // 84/5 = 16 cols

// configure RST and D/C pins
static void init_control_pins()
{
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;
	while ((SYSCTL_RCGCGPIO_R & SYSCTL_RCGCGPIO_R0) == 0);

	GPIO_PORTA_DEN_R |= 0xC0; // digital enable PA6-7
	GPIO_PORTA_DIR_R |= 0xC0; // set direction to output PA6-7

	GPIO_PORTA_DATA_R |= 0x40; // set PA6(RST) to high
}

// toggle RST pin
static void do_reset()
{
	GPIO_PORTA_DATA_R &= ~0x40; // set PA6(RST) to low
	systick_wait_1ms(1);
	GPIO_PORTA_DATA_R |= 0x40; // set PA6(RST) to high
}

static void set_data_mode()
{
	GPIO_PORTA_DATA_R |= 0x80; // set PA7(D/C) to high
}

static void set_cmd_mode()
{
	GPIO_PORTA_DATA_R &= ~0x80; // set PA7(D/C) to low
}

static void write_cmd(uint8_t cmd[], unsigned size)
{
	set_cmd_mode();
	spi_write_wait(cmd, size);
}

static void write_data(uint8_t data[], unsigned size)
{
	set_data_mode();
	spi_write_wait(data, size);
}

void nokia5110_clear_display(void)
{
	uint8_t empty[] = {0x00};

	nokia5110_set_row(0);
	nokia5110_set_col(0);

	for (unsigned i = 0; i < 504; i++) {
		write_data(empty, sizeof(empty));
	}
}

void nokia5110_clear_row(uint8_t row)
{
	uint8_t empty[] = {0x00};

	if (row >= 6)
		return;

	nokia5110_set_row(row);

	for (unsigned i = 0; i < 84; i++) {
		write_data(empty, sizeof(empty));
	}

	nokia5110_set_row(row);
	nokia5110_set_col(0); // get back to the start of the row
}

void nokia5110_write_raw_sym(uint8_t *raw)
{
	if (!raw)
		return;

	write_data(raw, SYM_SIZE);
}

void nokia5110_write_str(char *str)
{
	unsigned i;

	if (!str)
		return;

	for (i = 0; str[i] != '\0'; i++)
		write_data((uint8_t *)(ASCII[str[i] - ASCII_OFFSET]), SYM_SIZE);
}

void nokia5110_set_row(uint8_t row)
{
	uint8_t cmd_set_row[] = {0x40};

	if (row >= 6)
		return;

	ROW = row;
	cmd_set_row[0] |= row;

	write_cmd(cmd_set_row, 1);
}

void nokia5110_set_col(uint8_t col)
{
	uint8_t cmd_set_col[] = {0x80};

	if (col >= 16)
		return;

	COLUMN = col;
	cmd_set_col[0] |= col * SYM_SIZE;

	write_cmd(cmd_set_col, 1);
}

void nokia5110_init(void)
{
	uint8_t cmd_init[] = {0x21, 0x90, 0x20, 0x0C, 0x40, 0x80 };
	uint8_t cmd_setup_cursor[] = {0x04, 0x1F};

	pll_init_80mhz();
	systick_init();
	spi_init(MAIN_CLK_FQ_MHZ, SSI_CLK_FQ_MHZ);

	init_control_pins();

	systick_wait_10ms(10);
	do_reset();
	systick_wait_10ms(10);

	write_cmd(cmd_init, sizeof(cmd_init));
	write_cmd(cmd_setup_cursor, sizeof(cmd_setup_cursor));

	nokia5110_clear_display();
}
