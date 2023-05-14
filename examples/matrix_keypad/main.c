#include <stdbool.h>

#include "matrix_keypad.h"
#include "uart.h"
#include "pll.h"
#include "printf.h"

static char matrix[] = "123A456B789C*0#D";

int main(void)
{
	pll_init_80mhz();
	uart_init();
	matrix_keypad_init(matrix);

	printf("\nThis is matrix keypad example usage\n");

	while (true) {
		char c;

		if (matrix_keypad_read(&c))
			continue;

		printf("%c", c);
	}
}
