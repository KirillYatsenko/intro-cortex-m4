#include <stdint.h>
#include <stdbool.h>
#include "st7735r.h"

#include "example_bmp.h"

int main(void)
{
	st7735r_init();

	st7735r_clear_display();
	st7735r_draw_bitmap(0, 159, image, 128, 160);

	while (true) { }
}
