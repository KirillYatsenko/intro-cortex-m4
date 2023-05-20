#include <stdint.h>
#include <stdbool.h>

#include "systick.h"
#include "pll.h"
#include "tm4c123gh6pm.h"
#include "spi.h"
#include "uart.h"
#include "printf.h"
#include "nokia5110.h"

int main(void)
{
	pll_init_80mhz();
	systick_init();
	uart_init();
	nokia5110_init();

	nokia5110_write_str("Hello World!");

	while (true) {};
}
