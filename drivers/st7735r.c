#include <stdint.h>
#include <stdbool.h>

#include "spi.h"
#include "pll.h"
#include "systick.h"
#include "st7735r.h"
#include "../inc/tm4c123gh6pm.h"

#define MAIN_CLK_FQ_MHZ		80 // main clock speed
#define SSI_CLK_FQ_MHZ		8  // SPI speed

#define CMD_SWRST		0x01 // software reset
#define CMD_SLPOUT		0x11 // sleep-out mode
#define CMD_COLMOD		0x3A // color mode
#define CMD_DISPON		0x29 // display on
#define CMD_CASET		0x2A // column address set
#define CMD_RASET		0x2B // row address set
#define CMD_RAMWR		0x2C // ram memory write
#define CMD_MADCTL		0x36 // memory data access control
#define CMD_READID		0x09 // read display id
#define CMD_FRMCTRL		0xB1 // frame rate control

#define MAX_WIDTH		128
#define MAX_HEIGHT		160

/*
 *   Color encoding
 *  red - green - blue
 *  5  -   6   -  5
 *  refer to section: 9.7.21
 */
static uint16_t selected_color;

static void set_data_mode()
{
	GPIO_PORTA_DATA_R |= 0x80; // set PA7(D/C) to high
}

static void set_cmd_mode()
{
	GPIO_PORTA_DATA_R &= ~0x80; // set PA7(D/C) to low
}

static void write_data(uint8_t data[], unsigned size)
{
	set_data_mode();
	spi_write_wait(data, size);
}

static void write_cmd(uint8_t cmd, uint8_t args[], unsigned args_size,
		      uint32_t wait_ms)
{
	uint8_t cmd_arr[] = { cmd };

	set_cmd_mode();
	spi_write_wait(cmd_arr, sizeof(cmd_arr));

	if (args_size)
		write_data(args, args_size);

	if (wait_ms)
		systick_wait_1ms(wait_ms);
}

static void write_data_init(void)
{
	write_cmd(CMD_RAMWR, 0, 0, 0); // write to ram
}

static void write_pixel()
{
	uint8_t color[] = { selected_color >> 8, selected_color };
	write_data(color, sizeof(color));
}

static void set_window(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2)
{
	uint8_t col_data[4] = { 0 };
	uint8_t row_data[4] = { 0 };

	if (x2 >= MAX_WIDTH)
		x2 = MAX_WIDTH - 1;

	if (y2 >= MAX_WIDTH)
		y2 = MAX_HEIGHT - 1;

	col_data[1] = x1;
	col_data[3] = x2;

	row_data[1] = y1;
	row_data[3] = y2;

	write_cmd(CMD_CASET, col_data, sizeof(col_data), 0);
	write_cmd(CMD_RASET, row_data, sizeof(row_data), 0);
}

// void st7735r_set_color(uint8_t red, uint8_t green, uint8_t blue)
// {
// 	red = red & 0x1F;
// 	green = green & 0x3F;
// 	blue = blue & 0x1F;
//
// 	/*           #1 byte                     #0 byte
// 	 * [red(5-bits),green(3-bits)] [green(3-bits),blue(5-bits)]
// 	*/
// 	selected_color = (red << 11) | (green << 5) | blue;
// }

void st7735r_set_color(uint16_t color)
{
	selected_color = color;
}

static void draw_rectangle_filled(uint8_t x1, uint8_t x2, uint8_t y1,
				  uint8_t y2)
{
	uint16_t i;
	uint16_t pixels_count = (x2 - x1 + 1) * (y2 - y1 + 1);

	set_window(x1, x2, y1, y2);
	write_data_init();

	for (i = 0; i < pixels_count; i++)
		write_pixel();
}

void st7735r_draw_rectangle(bool filled, uint8_t x1, uint8_t x2, uint8_t y1,
			    uint8_t y2)
{
	if (filled) {
		draw_rectangle_filled(x1, x2, y1, y2);
		return;
	}

	// draw rectangle border only
	draw_rectangle_filled(x1, x2, y1, y1);
	draw_rectangle_filled(x2, x2, y1, y2);
	draw_rectangle_filled(x1, x1, y1, y2);
	draw_rectangle_filled(x1, x2, y2, y2);
}

// configure RST and D/C pins
static void init_control_pins(void)
{
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;
	while ((SYSCTL_RCGCGPIO_R & SYSCTL_RCGCGPIO_R0) == 0);

	GPIO_PORTA_DEN_R |= 0xC0; // digital enable PA6-7
	GPIO_PORTA_DIR_R |= 0xC0; // set direction to output PA6-7

	GPIO_PORTA_DATA_R |= 0x40; // set PA6(RST) to high
}

static void init_display(void)
{
	uint8_t colmod_data[] = { 0x05 }; // color mode: 16-bit
	uint8_t madctl_data[] = { 0xA0 }; // rotate to left

	write_cmd(CMD_SWRST, 0, 0, 150); // software reset
	write_cmd(CMD_SLPOUT, 0, 0, 500); // sleep-out mode
	write_cmd(CMD_COLMOD, colmod_data, sizeof(colmod_data), 10);
	write_cmd(CMD_DISPON, 0, 0, 200); // display on
}

//------------st7735r_draw_bitmap------------
// Displays a 16-bit color BMP image.  A bitmap file that is created
// by a PC image processing program has a header and may be padded
// with dummy columns so the data have four byte alignment.  This
// function assumes that all of that has been stripped out, and the
// array image[] has one 16-bit halfword for each pixel to be
// displayed on the screen (encoded in reverse order, which is
// standard for bitmap files).  An array can be created in this
// format from a 24-bit-per-pixel .bmp file using the associated
// converter program.
// (x,y) is the screen location of the lower left corner of BMP image
// Requires (11 + 2*width*h) bytes of transmission (assuming image fully on screen)
// Input: x     horizontal position of the bottom left corner of the image, columns from the left edge
//        y     vertical position of the bottom left corner of the image, rows from the top edge
//        image pointer to a 16-bit color BMP image
//        width     number of pixels wide
//        h     number of pixels tall
// Output: none
// Must be less than or equal to 128 pixels wide by 160 pixels high
void st7735r_draw_bitmap(int16_t x, int16_t y, const uint16_t *image,
						 int16_t width, int16_t height)
{

	// non-zero if columns need to be skipped due to clipping
	int16_t skipC = 0;

	// save this value; even if not all columns fit on the screen,
	// the image is still this width in ROM
	int16_t originalWidth = width;
	int i = width * (height - 1);
	uint8_t data[1];

	if ((x >= MAX_WIDTH) || ((y - height + 1) >= MAX_HEIGHT) ||
	    ((x + width) <= 0) || (y < 0))
		return;	 // image is totally off the screen, do nothing

	if ((width > MAX_WIDTH) || (height > MAX_HEIGHT)) {
		// image is too wide for the screen, do nothing
		//***This isn't necessarily a fatal error, but it makes the
		// following logic much more complicated, since you can have
		// an image that exceeds multiple boundaries and needs to be
		// clipped on more than one side.
		return;
	}
	if ((x + width - 1) >= MAX_WIDTH) {
		// image exceeds right of screen
		skipC = (x + width) - MAX_WIDTH;  // skip cut off columns
		width = MAX_WIDTH - x;
	}
	if ((y - height + 1) < 0) {
		// image exceeds top of screen
		i = i - (height - y - 1) * originalWidth;  // skip the last cut off rows
		height = y + 1;
	}
	if (x < 0) {  // image exceeds left of screen
		width = width + x;
		skipC = -1 * x;	 // skip cut off columns
		i = i - x;	 // skip the first cut off columns
		x = 0;
	}
	if (y >= MAX_HEIGHT) {	// image exceeds bottom of screen
		height = height - (y - MAX_HEIGHT + 1);
		y = MAX_HEIGHT - 1;
	}

	set_window(x, x + width - 1, y - height + 1, y);
	write_data_init();

	for (y = 0; y < height; y = y + 1) {
		for (x = 0; x < width; x = x + 1) {
			// send the top 8 bits
			data[0] = image[i] >> 8;
			write_data(data, 1);

			// send the bottom 8 bits
			data[0] = image[i];
			write_data(data, 1);

			i = i + 1;  // go to the next pixel
		}
		i = i + skipC;
		i = i - 2 * originalWidth;
	}
}

void st7735r_init(void)
{
	pll_init_80mhz();
	systick_init();

	spi_init(MAIN_CLK_FQ_MHZ, SSI_CLK_FQ_MHZ);

	init_control_pins();
	init_display();
}
