// Source: http://www.simplyembedded.org/tutorials/interrupt-free-ring-buffer/

#include "ring_buffer.h"

int rg_buf_init(struct rg_buf *rg_buf, struct rg_buf_attr *attr)
{
	if (!rg_buf || !attr || !attr->buf)
		return -1;

	// check if the size is power of 2
	if (((attr->size - 1) & attr->size) != 0)
		return -1;

	rg_buf->top = 0;
	rg_buf->tail = 0;
	rg_buf->buf = attr->buf;
	rg_buf->size = attr->size;

	return 0;
}

bool rg_buf_is_full(struct rg_buf *rg_buf)
{
	if (!rg_buf)
		return false;

	return rg_buf->top - rg_buf->tail == rg_buf->size;
}

int rg_buf_space_left(struct rg_buf *rg_buf)
{
	// Possible race condition, if rg_buf->top get's increased right after
	// it read from the memory, the amount of space left will be more then
	// actual is. Another thread should not interrupt this function.
	return rg_buf->size - (rg_buf->top - rg_buf->tail);
}

int rg_buf_put_data(struct rg_buf *rg_buf, buf_t d)
{
	size_t indx;

	// don't block, return immediately
	if (!rg_buf || rg_buf_is_full(rg_buf))
		return -1;

	indx = rg_buf->top & (rg_buf->size - 1);
	rg_buf->buf[indx] = d;
	rg_buf->top++;

	return 0;
}

bool rg_buf_is_empty(struct rg_buf *rg_buf)
{
	return rg_buf->top == rg_buf->tail;
}

int rg_buf_get_data(struct rg_buf *rg_buf, buf_t *d)
{
	size_t indx;

	// don't block, return immediately
	if (!rg_buf || rg_buf_is_empty(rg_buf))
		return -1;

	indx = rg_buf->tail & (rg_buf->size - 1);
	*d = rg_buf->buf[indx];
	rg_buf->tail++;

	return 0;
}
