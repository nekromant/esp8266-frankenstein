#ifndef CB_H
#define CB_H

/* Circular buffer object */
struct cbuffer {
    int         size;   /* maximum number of elements           */
    int         start;  /* index of oldest element              */
    int         count;    /* index at which to write new element  */
    char       *elems;  /* vector of elements           */
};


void cb_flush(struct cbuffer *cb);
int cb_is_full(struct cbuffer *cb);
int cb_is_empty(struct cbuffer *cb);
char *cb_read(struct cbuffer *b);
char *cb_peek(struct cbuffer *b);

#endif
