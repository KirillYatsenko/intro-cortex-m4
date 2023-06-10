#include <stdint.h>
#include <stdbool.h>

#include "pll.h"
#include "systick.h"
#include "st7735r.h"
#include "tetrominos.h"
#include "uart.h"
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
	int16_t y_pos;
	int16_t x_pos;
	struct tetromino tetromino;
};

enum new_position {
	POS_UNCHANGED,
	POS_LEFT,
	POS_RIGHT,
	POS_ROTATE
};

static struct game_arena arena;
static enum new_position new_pos;
static unsigned random_seed = 1;

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
		new_pos = POS_ROTATE;
		GPIO_PORTC_ICR_R = 0x20;
	} else if (GPIO_PORTC_MIS_R & 0x10) {
		// right pressed
		GPIO_PORTC_ICR_R = 0x10;
		new_pos = POS_RIGHT;
	}

	GPIO_PORTC_ICR_R = 0x70; // clear interrupt
}

static void clone_piece(struct piece *src, struct piece *dst)
{
	dst->y_pos = src->y_pos;
	dst->x_pos = src->x_pos;
	tetromino_clone(&src->tetromino, &dst->tetromino);
}

static unsigned get_arena_sum()
{
	unsigned sum = 0;
	uint8_t row, col;

	for (row = 0; row < SCREEN_MAX_CELLS_Y; row++)
		for (col = 0; col < SCREEN_MAX_CELLS_X; col++)
			sum += arena.bitmap[row][col] * (row + 1) * (col + 1);

	return sum;
}

static unsigned get_random()
{
	random_seed = (random_seed * 1664525) + 1013904223 + get_arena_sum();
	return random_seed;
}

static void generate_piece(struct piece *p)
{
	unsigned tetr_t = get_random() % COUNT_TETR;

	p->y_pos = 0;
	p->x_pos = SCREEN_MAX_CELLS_X / 2;
	tetromino_clone(&tetrominos[tetr_t], &p->tetromino);
}

static void draw_piece_bit(bool erase, struct piece *p, uint16_t row,
						   uint16_t col)
{
	int16_t x1, x2, y1, y2;
	uint16_t color = erase ? BACKGROUD_COLOR : p->tetromino.color;

	y1 = (p->y_pos - TETROMINO_SIZE + row) * CELL_SIZE;
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
	for (row = 0; row < TETROMINO_SIZE; row++) {
		for (col = 0; col < TETROMINO_SIZE; col++) {
			if (!p->tetromino.bitmap[row][col])
				continue;

			draw_piece_bit(erase, p, row, col);
		}
	}
}

// merge piece into existing arena,
// return error if unable to merge = GAME LOST
static int merge_piece(struct piece *p)
{
	uint8_t row, col;
	int16_t arena_y, arena_x;

	for (row = 0; row < TETROMINO_SIZE; row++) {
		for (col = 0; col < TETROMINO_SIZE; col++) {
			if (!p->tetromino.bitmap[row][col])
				continue;

			arena_y = p->y_pos - TETROMINO_SIZE + row;
			arena_x = p->x_pos + col;

			if (arena_y < 0)
				return -1;

			arena.bitmap[arena_y][arena_x] = p->tetromino.color;
		}
	}

	return 0;
}

// iterate over existing arena and check if collision occured
static bool detect_collision(struct piece *p)
{
	uint8_t col, row;
	int16_t arena_y, arena_x;

	// obvious checks
	if (p->x_pos < 0 || p->y_pos > SCREEN_MAX_CELLS_Y)
		return true;

	for (row = 0; row < TETROMINO_SIZE; row++) {
		for (col = 0; col < TETROMINO_SIZE; col++) {
			if (!p->tetromino.bitmap[row][col])
				continue;

			arena_y = (p->y_pos - TETROMINO_SIZE + row);
			arena_x = p->x_pos + col;

			if (arena_y < 0)
				continue; // ignore not visible bit

			if (arena_x >= SCREEN_MAX_CELLS_X)
				return true;

			if (arena.bitmap[arena_y][arena_x])
				return true;
		}
	}

	return false;
}

static void change_position(struct piece *p)
{
	struct piece p_tmp;

	p->y_pos++;

	if (new_pos == POS_UNCHANGED)
		return;

	clone_piece(p, &p_tmp);

	if (new_pos == POS_LEFT)
		p_tmp.x_pos--;
	else if (new_pos == POS_RIGHT)
		p_tmp.x_pos++;
	else if (new_pos == POS_ROTATE)
		tetromino_rotate(&p_tmp.tetromino);

	if (!detect_collision(&p_tmp))
		clone_piece(&p_tmp, p);

	new_pos = POS_UNCHANGED;
}

static bool detect_possible_collision(struct piece *p)
{
	bool ret;

	p->y_pos++;
	ret = detect_collision(p);
	p->y_pos--;

	return ret;
}

// return non-zero if piece can't be moved
static int move_piece(struct piece *p)
{
	if (detect_possible_collision(p)) {
		if (merge_piece(p))
			return GAME_LOST;

		return PIECE_MERGED;
	}

	draw_piece(p, true); // erase piece first
	change_position(p);
	draw_piece(p, false);

	return 0;
}

static void redraw_line(uint8_t row)
{
	int16_t x1, x2, y1, y2;
	uint8_t col;

	for (col = 0; col < SCREEN_MAX_CELLS_X; col++) {
		y1 = row * CELL_SIZE;
		y2 = y1 + CELL_SIZE;

		x1 = col * CELL_SIZE;
		x2 = x1 + CELL_SIZE;

		if (arena.bitmap[row][col]) {
			st7735r_set_color(arena.bitmap[row][col]);
			st7735r_draw_rectangle(true, x1, x2, y1, y2);

			st7735r_set_color(BACKGROUD_COLOR);
			st7735r_draw_rectangle(false, x1, x2, y1, y2);
		} else { // erase branch
			st7735r_set_color(BACKGROUD_COLOR);
			st7735r_draw_rectangle(true, x1, x2, y1, y2);
		}
	}
}

static bool filled_line_exists(uint8_t *row)
{
	uint8_t col, row_tmp;
	bool filled_exist = false;

	for (row_tmp = SCREEN_MAX_CELLS_Y - 1; row_tmp > 0; row_tmp--) {
		for (col = 0; col < SCREEN_MAX_CELLS_X; col++) {
			if (!arena.bitmap[row_tmp][col])
				break;

			if (col == SCREEN_MAX_CELLS_X - 1)
				filled_exist = true;
		}

		if (filled_exist) {
			*row = row_tmp;
			return true;
		}
	}

	return false;
}

// check if combo is achieved and shift the arena if so
static bool clear_line()
{
	uint8_t col, row;
	bool line_empty;

	if (!filled_line_exists(&row))
		return false;

	// shift down the arena, row specifies first filled line
	for (; row > 0;  row--) {
		line_empty = true;

		for (col = 0; col < SCREEN_MAX_CELLS_X; col++) {
			arena.bitmap[row][col] = arena.bitmap[row - 1][col];
			if (arena.bitmap[row][col])
				line_empty = false;
		}

		redraw_line(row);

		if (line_empty)
			break;
	}

	return true;
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
			systick_wait_10ms(45);
		}

		if (ret == GAME_LOST) {
			// clear_display();
			return;
		}

		while (clear_line());
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
