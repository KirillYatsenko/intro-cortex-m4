#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct rg_buf {
	volatile size_t tail;
	volatile size_t top;
	uint32_t size;
	char *buf;
};

struct rg_buf_attr {
	char *buf;
	size_t size;
};

int rg_buf_init(struct rg_buf *rg_buf, struct rg_buf_attr *attr);
bool rg_buf_is_full(struct rg_buf *rg_buf);
bool rg_buf_is_empty(struct rg_buf *rg_buf);
int rg_buf_put_char(struct rg_buf *rg_buf, char c);
int rg_buf_get_char(struct rg_buf *rg_buf, char *c);

#endif
