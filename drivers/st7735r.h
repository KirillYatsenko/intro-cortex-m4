#ifndef _ST7735R_H_
#define _ST7735R_H_

#include <stdint.h>

/*  Wiring
 * PA7 - D/C
 * PA6 - RST
 * PB5 - SCE
 * PB4 - SCLK
 * PB7 - MOSI
 */
void st7735r_init();
void st7735r_clear_display();
void st7735r_set_color(uint16_t red, uint16_t green, uint16_t blue);
void st7735r_draw_rectangle(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2);
void st7735r_draw_bitmap(int16_t x, int16_t y, const uint16_t *image,
			 int16_t width, int16_t height);

#endif
