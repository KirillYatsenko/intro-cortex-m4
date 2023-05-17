#include <stdint.h>
#include <stdbool.h>

#include "systick.h"
#include "pll.h"
#include "tm4c123gh6pm.h"
#include "spi.h"
#include "uart.h"
#include "printf.h"

#define MAIN_CLK_FQ_MHZ		80
#define SSI_CLK_FQ_MHZ		4

static uint8_t spi_test_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
static uint8_t spi_test_big_data[128]; // size is the same as spi buffer

int main(void)
{
	uint8_t i = 0;

	pll_init_80mhz();
	systick_init();
	uart_init();
	spi_init(MAIN_CLK_FQ_MHZ, SSI_CLK_FQ_MHZ);

	for (i = 0; i < sizeof(spi_test_big_data); i++)
		spi_test_big_data[i] = i;

	while (true) {
		if (spi_write(spi_test_data, sizeof(spi_test_data)))
			printf("failed to send spi_test_data\n");

		systick_wait_10ms(1);

		if (spi_write(spi_test_big_data, sizeof(spi_test_big_data)))
			printf("failed to send spi_test_big_data\n");

		systick_wait_10ms(5);
	}
}
