#include <stdint.h>
#include <stdbool.h>

#include "nokia5110.h"
#include "systick.h"
#include "pll.h"
#include "spi.h"
#include "printf.h"
#include "uart.h"
#include "adc.h"

int main(void)
{
	float converted;
	float conversion = 3.3 / 4096;

	pll_init_80mhz();
	systick_init();
	adc_init();
	nokia5110_init();
	uart_init();

	while (true) {
		converted = adc_read() * conversion;

		nokia5110_clear_row(2);
		nokia5110_set_col(6);
		fctprintf(nokia5110_put_char, NULL, "%.2f V", converted);

		printf("%.2f V\n", converted);
	}
}
