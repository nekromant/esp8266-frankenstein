/* Circular buffer example, keeps one slot open */
 
#include <stdio.h>
#include <stdlib.h>
#include "cb.h"

  
int cb_is_full(struct cbuffer *cb) 
{
	return cb->count == cb->size; 
}
 
int cb_is_empty(struct cbuffer *cb) 
{
	return cb->count == 0; 
}

char *cb_read(struct cbuffer *b)
{
	struct rf_packet *ret = NULL;
	if (!cb_is_empty(b)) {
		ret = &b->elems[b->start];
		b->start = (b->start + 1) % b->size;
		b->count--;	
	}
	return ret;
}

char *cb_peek(struct cbuffer *b)
{
	struct rf_packet *ret = NULL;
	if (!cb_is_empty(b)) {
		ret = &b->elems[b->start];
	}
	return ret;
}

void cb_flush(struct cbuffer *cb)
{
	cb->count = 0;
}
