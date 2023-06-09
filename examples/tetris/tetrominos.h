#ifndef _TETROMINOS_H_
#define _TETROMINOS_H_

#include <stdint.h>
#include <stdbool.h>
#include <printf.h>

#define TETROMINO_SIZE		4

// R means Rotated
enum tetromino_t {
	CUBE_TETR = 0,
	STRAIGHT_TETR,
	Z_TETR,
	L_TETR,
	T_TETR,
	COUNT_TETR,
};

struct tetromino {
	uint16_t color; // encoded in 5-6-5 rgb format
	enum tetromino_t type;
	bool bitmap[TETROMINO_SIZE][TETROMINO_SIZE];
};

struct tetromino tetrominos[] = {
	{	 // CUBE TETR
		.color = 0xff2c,
		.type = CUBE_TETR,
		.bitmap = {
			{ 0, 0, 0, 0 },
			{ 0, 0, 0, 0 },
			{ 1, 1, 0, 0 },
			{ 1, 1, 0, 0 }
		}
	},
	{	// STRAIGHT TETR
		.color = 0x2ae8,
		.type = STRAIGHT_TETR,
		.bitmap = {
			{ 1, 0, 0, 0 },
			{ 1, 0, 0, 0 },
			{ 1, 0, 0, 0 },
			{ 1, 0, 0, 0 }
		}
	},
	{	// Z TETR
		.color = 0x8cd6,
		.type = Z_TETR,
		.bitmap = {
			{ 0, 0, 0, 0 },
			{ 0, 0, 0, 0 },
			{ 1, 1, 0, 0 },
			{ 0, 1, 1, 0 },
		}
	},
	{	// L TETR
		.color = 0x71e7,
		.type = L_TETR,
		.bitmap = {
			{ 0, 0, 0, 0 },
			{ 1, 0, 0, 0 },
			{ 1, 0, 0, 0 },
			{ 1, 1, 0, 0 },
		}
	},
	{	// T TETR
		.color = 0xe5b2,
		.type = T_TETR,
		.bitmap = {
			{ 0, 0, 0, 0 },
			{ 0, 0, 0, 0 },
			{ 1, 1, 1, 0 },
			{ 0, 1, 0, 0 },
		}
	}
};

static void tetromino_clone(struct tetromino *src, struct tetromino *dst)
{
	uint8_t row, col;

	if (!src || !dst)
		return;

	dst->type = src->type;
	dst->color = src->color;

	for (row = 0; row < TETROMINO_SIZE; row++)
		for (col = 0; col < TETROMINO_SIZE; col++)
			dst->bitmap[row][col] = src->bitmap[row][col];
}

static void _allign_bottom(struct tetromino *tetr)
{
	int row, col;
	short empty_lines = 0;

	for (row = TETROMINO_SIZE - 1; row >= 0; row--) {
		for (col = 0; col < TETROMINO_SIZE; col++) {
			if (tetr->bitmap[row][col])
				goto lines_counted;
		}
		empty_lines++;
	}
lines_counted:
	if (!empty_lines)
		return; // piece is already alligned

	// row now contains first non empty line
	for (; row >= 0; row--) {
		for (col = 0; col < TETROMINO_SIZE; col++) {
			tetr->bitmap[row + empty_lines][col] = tetr->bitmap[row][col];
			tetr->bitmap[row][col] = 0;
		}
	}
}

// rotate the tetromino bitmap such way that it's bottom alligned
static void tetromino_rotate(struct tetromino *tetr)
{
	uint8_t row, col;
	struct tetromino tetr_tmp;

	if (!tetr)
		return;

	if (tetr->type == CUBE_TETR)
		return; // no need to rotate cube

	tetr_tmp.color = tetr->color;
	tetr_tmp.type = tetr->type;

	for (row = 0; row < TETROMINO_SIZE; row++)
		for (col = 0; col < TETROMINO_SIZE; col++)
			tetr_tmp.bitmap[row][col] =
				tetr->bitmap[TETROMINO_SIZE - 1 - col][row];

	_allign_bottom(&tetr_tmp);
	tetromino_clone(&tetr_tmp, tetr);
}

#endif
