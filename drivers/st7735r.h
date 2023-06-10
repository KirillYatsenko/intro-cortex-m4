#ifndef _ST7735R_H_
#define _ST7735R_H_

#include <stdint.h>
#include <stdbool.h>

#define ST7735R_MAX_HEIGHT	160 - 1
#define ST7735R_MAX_WIDTH	128 - 1
#define ST7735R_CENTER		80
#define SYMBOL_SIZE 	36

/*  Wiring
 * PA7 - D/C
 * PA6 - RST
 * PB5 - SCE
 * PB4 - SCLK
 * PB7 - MOSI
 */
void st7735r_init();
void st7735r_set_color(uint16_t color);
void st7735r_draw_rectangle(bool filled, uint8_t x1, uint8_t x2, uint8_t y1,
							uint8_t y2);
void st7735r_draw_bitmap(int16_t x, int16_t y, const uint16_t *image,
						 int16_t width, int16_t height);
void st7735r_print_number(unsigned number, uint8_t x, uint8_t y);

#endif
