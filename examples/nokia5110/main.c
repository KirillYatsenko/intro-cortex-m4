#include <stdint.h>
#include <stdbool.h>

#include "systick.h"
#include "pll.h"
#include "tm4c123gh6pm.h"
#include "spi.h"
#include "uart.h"
#include "printf.h"
#include "nokia5110.h"

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

int main(void)
{
	char atoi_str[4];
	uint8_t i = 0;

	pll_init_80mhz();
	systick_init();
	uart_init();

	nokia5110_init();
	nokia5110_clear_display();

	while (true) {
		nokia5110_clear_row(0);
		nokia5110_write_str(atoi(i++, atoi_str));
		systick_wait_10ms(10);
	}
}
