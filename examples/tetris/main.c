#include <stdint.h>
#include <stdbool.h>

#include "pll.h"
#include "systick.h"
#include "st7735r.h"
#include "tetrominos.h"
#include "uart.h"
#include "printf.h"
#include "leds.h"
#include "tm4c123gh6pm.h"

#define MAX_PIECES				35
#define CELL_SIZE				12 // in pixels
#define BACKGROUD_COLOR			0x18a4

#define SCREEN_MAX_CELLS_Y		13
#define SCREEN_MAX_CELLS_X		10

#define PIECE_MERGED			-1
#define GAME_LOST				-2

struct game_arena {
	uint16_t bitmap[SCREEN_MAX_CELLS_Y][SCREEN_MAX_CELLS_X];
};

struct piece {
	uint8_t y_pos;
	uint8_t x_pos;
	uint8_t y_trimmed;
	bool displayed;
	struct tetromino tetromino;
};

enum new_position {
	POS_UNCHANGED,
	POS_LEFT,
	POS_RIGHT,
};

static struct game_arena arena;
static enum new_position new_pos;

static void clear_display(void)
{
	st7735r_set_color(BACKGROUD_COLOR);
	st7735r_draw_rectangle(true, 0, ST7735R_MAX_WIDTH, 0, ST7735R_MAX_HEIGHT);
}

static void prepare_display(void)
{
	st7735r_init();
	clear_display();
}

// PC6 - left button
// PC5 - center button
// PC4 - right button
static void buttons_init(void)
{
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2; // activate clock for port C
	while ((SYSCTL_RCGCGPIO_R & SYSCTL_RCGCGPIO_R2) == 0);

	GPIO_PORTC_PUR_R |= 0x70; // enable pull ups
	GPIO_PORTC_AFSEL_R &= ~0x70; // disable alt func
	GPIO_PORTC_DEN_R |= 0x70; // digital enable
	GPIO_PORTC_AMSEL_R &= ~0x70; // disable analog functionality

	GPIO_PORTC_PUR_R |= 0x70; // enable weak pull up on PF4
	GPIO_PORTC_IS_R &= ~0x70; // configure as edge sensitive
	GPIO_PORTC_IBE_R &= ~0x70; // is not both edged
	GPIO_PORTC_IEV_R &= ~0x70; // falling edge
	GPIO_PORTC_ICR_R = 0x70; // prevent unwanted interrupts by clearing

	NVIC_PRI0_R = (NVIC_PRI0_R & 0xFF0FFFFF) | 0x800000; // priority 4
	NVIC_EN0_R = 0x04; // enable interrupt #2 in NVIC
	GPIO_PORTC_IM_R |= 0x70; // unmask interrupts
}

void GpioPortCHandler(void)
{
	if (GPIO_PORTC_MIS_R & 0x40) {
		// left pressed
		GPIO_PORTC_ICR_R = 0x40;
		new_pos = POS_LEFT;
	} else if (GPIO_PORTC_MIS_R & 0x20) {
		// center pressed
		GPIO_PORTC_ICR_R = 0x20;
	} else if (GPIO_PORTC_MIS_R & 0x10) {
		// right pressed
		GPIO_PORTC_ICR_R = 0x10;
		new_pos = POS_RIGHT;
	}

	GPIO_PORTC_ICR_R = 0x70; // clear interrupt
}

static void set_piece_location(struct piece *p)
{
	p->x_pos = SCREEN_MAX_CELLS_X / 2;
}

static void generate_piece(struct piece *p)
{
	static int tetr_t = 0;
	tetr_t = tetr_t % COUNT_TETR;

	p->y_pos = 0;
	p->y_trimmed = 0;
	p->displayed = true;
	tetromino_clone(&tetrominos[tetr_t], &p->tetromino);

	set_piece_location(p);

	tetr_t++;
}

static void draw_piece_bit(bool erase, struct piece *p, uint16_t row,
						   uint16_t col)
{
	int16_t x1, x2, y1, y2;
	uint16_t color = erase ? BACKGROUD_COLOR : p->tetromino.color;

	y1 = (p->y_pos - TETROMINO_HEIGHT + row) * CELL_SIZE;
	y2 = y1 + CELL_SIZE;

	// ignore non-visible bit
	if (y1 < 0)
		return;

	x1 = (p->x_pos + col) * CELL_SIZE;
	x2 = x1 + CELL_SIZE;

	st7735r_set_color(color);
	st7735r_draw_rectangle(true, x1, x2, y1, y2);

	st7735r_set_color(BACKGROUD_COLOR);
	st7735r_draw_rectangle(false, x1, x2, y1, y2);
}

static void draw_piece(struct piece *p, bool erase)
{
	uint16_t row, col;

	// iterate over tetromino bitmap and draw respected bits
	for (row = 0; row < TETROMINO_HEIGHT; row++) {
		for (col = 0; col < TETROMINO_WIDHT; col++) {
			if (!p->tetromino.bitmap[row][col])
				continue;

			draw_piece_bit(erase, p, row, col);
		}
	}
}

static void print_matrix(bool matrix[4][4])
{
	printf("\n");
	for (int row = 0; row < 4; row++) {
		printf("[ ");
		for (int col = 0; col < 4; col++) {
			printf("\t%d", matrix[row][col]);
			if (col != 3)
				printf(", ");
			else
				printf(" ");
		}
		printf("]\n");
	}
}

// merge piece into existing arena,
// return error if unable to merge = GAME LOST
static int merge_piece(struct piece *p)
{
	uint8_t row, col;
	int16_t arena_y, arena_x;

	// print_matrix(piece->tetromino.bitmap);

	for (row = 0; row < TETROMINO_HEIGHT; row++) {
		for (col = 0; col < TETROMINO_WIDHT; col++) {
			if (!p->tetromino.bitmap[row][col])
				continue;

			arena_y = p->y_pos - TETROMINO_HEIGHT + row;
			arena_x = p->x_pos + col;

			if (arena_y < 0)
				return -1;

			arena.bitmap[arena_y][arena_x] = p->tetromino.color;
		}
	}

	return 0;
}

// check if vertical collision occured with existing
// pieces in the arena or the piece is in the very bottom
static bool y_collision(struct piece *p)
{
	uint8_t col, row;
	int16_t arena_y, arena_x;

	if (p->y_pos == SCREEN_MAX_CELLS_Y)
		return true;

	for (row = 0; row < TETROMINO_HEIGHT; row++) {
		for (col = 0; col < TETROMINO_WIDHT; col++) {
			if (!p->tetromino.bitmap[row][col])
				continue;

			// check if the pixel below is occupied
			arena_y = (p->y_pos - TETROMINO_HEIGHT + row) + 1;
			arena_x = p->x_pos + col;

			// piece's bit is not visible yet
			if (arena_y < 0)
				continue;

			if (arena.bitmap[arena_y][arena_x])
				return true;
		}
	}

	return false;
}

// dir == false = left
// dir == true = right
static bool x_collision(struct piece *p, enum new_position pos)
{
	uint8_t col, row;
	int16_t arena_y, arena_x;

	if (pos == POS_LEFT && p->x_pos == 0)
			return true;

	for (row = 0; row < TETROMINO_HEIGHT; row++) {
		for (col = 0; col < TETROMINO_WIDHT; col++) {
			if (!p->tetromino.bitmap[row][col])
				continue;

			if ((p->x_pos + col == SCREEN_MAX_CELLS_X - 1) && pos == POS_RIGHT)
				return true;

			arena_y = (p->y_pos - TETROMINO_HEIGHT + row);

			arena_x = p->x_pos + col;
			arena_x += pos == POS_LEFT ? -1 : +1;

			// piece's bit is not visible yet
			if (arena_y < 0)
				continue;

			if (arena.bitmap[arena_y][arena_x])
				return true;
		}
	}

	return false;
}

// return non-zero if piece can't be moved
static int move_piece(struct piece *p)
{
	if (y_collision(p)) {
		if (merge_piece(p))
			return GAME_LOST;

		return PIECE_MERGED;
	}

	draw_piece(p, true); // erase piece first

	p->y_pos++;

	if (new_pos == POS_LEFT && !x_collision(p, new_pos))
			p->x_pos--;
	else if (new_pos == POS_RIGHT && !x_collision(p, new_pos))
			p->x_pos++;

	new_pos = POS_UNCHANGED;

	draw_piece(p, false);

	return 0;
}

static void start_game(void)
{
	int ret;
	struct piece p;

	while (true) {
		generate_piece(&p);
		new_pos = POS_UNCHANGED;

		while (!(ret = move_piece(&p))) {
			// systick irq should be used
			systick_wait_10ms(50);
		}

		if (ret == GAME_LOST) {
			clear_display();
			printf("game lost!\n");
			return;
		}
	}
}

// Todo: try to use code from adafruit, maybe refresh rate could be better
// first do MVP then optimize
int main(void)
{
	pll_init_80mhz();
	uart_init();
	leds_init_builtin();

	prepare_display();
	buttons_init();

	start_game();

	while (true);
}
