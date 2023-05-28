#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint32_t buf_t;

struct rg_buf {
	volatile size_t tail;
	volatile size_t top;
	uint32_t size;
	buf_t *buf;
};

struct rg_buf_attr {
	buf_t *buf;
	size_t size;
};

int rg_buf_init(struct rg_buf *rg_buf, struct rg_buf_attr *attr);
bool rg_buf_is_full(struct rg_buf *rg_buf);
bool rg_buf_is_empty(struct rg_buf *rg_buf);
int rg_buf_space_left(struct rg_buf *rg_buf);
int rg_buf_put_data(struct rg_buf *rg_buf, buf_t d);
int rg_buf_get_data(struct rg_buf *rg_buf, buf_t *d);

#endif
