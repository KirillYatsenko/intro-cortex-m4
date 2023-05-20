
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

static void clear_display(void)
{
	uint8_t empty[] = {0x00};
	for (unsigned i = 0; i < 504; i++) {
		write_data(empty, sizeof(empty));
	}
}

static uint8_t h_char[] = {0x1F, 0x04, 0x1F, 0x00};
static uint8_t e_char[] = {0x1F, 0x15, 0x15, 0x00};
static uint8_t l_char[] = {0x1F, 0x10, 0x10, 0x00};
static uint8_t o_char[] = {0x1F, 0x11, 0x1F, 0x00};
static uint8_t space_char[] = {0x00, 0x00};
static uint8_t w_char[] = {0x0F, 0x10, 0x0E, 0x10, 0x0F, 0x00};
static uint8_t r_char[] = {0x1F, 0x09, 0x16, 0x00};
static uint8_t d_char[] = {0x1F, 0x11, 0x11, 0x0E, 0x00};
static uint8_t exclam_char[] = {0x17, 0x00};

void nokia5110_write_str(char *str)
{
	unsigned i;

	if (!str)
		return;

	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] == 'H' || str[i] == 'h')
			write_data(h_char, sizeof(h_char));
		else if (str[i] == 'E' || str[i] == 'e')
			write_data(e_char, sizeof(e_char));
		else if (str[i] == 'L' || str[i] == 'l')
			write_data(l_char, sizeof(l_char));
		else if (str[i] == 'O' || str[i] == 'o')
			write_data(o_char, sizeof(o_char));
		else if (str[i] == ' ')
			write_data(space_char, sizeof(space_char));
		else if (str[i] == 'W' || str[i] == 'w')
			write_data(w_char, sizeof(w_char));
		else if (str[i] == 'R' || str[i] == 'r')
			write_data(r_char, sizeof(r_char));
		else if (str[i] == 'D' || str[i] == 'd')
			write_data(d_char, sizeof(d_char));
		else if (str[i] == '!')
			write_data(exclam_char, sizeof(exclam_char));
	}
}

void nokia5110_init(void)
{
	pll_init_80mhz();
	systick_init();
	spi_init(MAIN_CLK_FQ_MHZ, SSI_CLK_FQ_MHZ);

	init_control_pins();

	uint8_t cmd_init[] = {0x21, 0x90, 0x20, 0x0C, 0x40, 0x80 };
	uint8_t cmd_set_start[] = {0x04, 0x1F};

	systick_wait_10ms(10);
	do_reset();
	systick_wait_10ms(10);

	write_cmd(cmd_init, sizeof(cmd_init));
	write_cmd(cmd_set_start, sizeof(cmd_set_start));
	clear_display();
}
