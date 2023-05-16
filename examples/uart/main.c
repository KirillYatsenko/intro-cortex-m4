#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "leds.h"
#include "pll.h"
#include "systick.h"
#include "tm4c123gh6pm.h"
#include "printf.h"

int main(void)
{
	char str[] = "Beyond the horizon of the place we lived when we were young\n" \
		     "In a world of magnets and miracles\n" \
		     "Our thoughts strayed constantly and without boundary\n" \
		     "The ringing of the division bell had begun\n" \
		     "Along the Long Road and on down the Causeway\n" \
		     "Do they still meet there by the Cut\n" \
		     "There was a ragged band that followed in our footsteps\n" \
		     "Running before time took our dreams away\n" \
		     "Leaving the myriad small creatures trying to tie us to the ground\n" \
		     "To a life consumed by slow decay\n\n";

	pll_init_80mhz();
	leds_init_builtin();
	uart_init();
	systick_init();

	printf("%s\n", str);
	printf("This is: %s on line: %d\n", __FILE__, __LINE__);

	while (true) {
		char c;

		if (!uart_get_char(&c))
			printf("%c", c);
	}
}
