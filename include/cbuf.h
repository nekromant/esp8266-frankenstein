#ifndef _CBUF_H_
#define _CBUF_H_

#include <stdlib.h>
#include <c_types.h>

typedef uint16_t cbsize_t;

// Circular buffer object
typedef struct cbuf_s
{
	cbsize_t	size;		// buffer size
	cbsize_t	read;		// index of oldest data
	cbsize_t	write;		// index at which to write new data
	cbsize_t	unread;		// index of next unread data
	char*		buf;		// data buffer
	char		empty;		// no data
	char		allread;	// (=(read == unread)) => all is swallowed)
} cbuf_t;

// GIVE log2() of desired size
#define CBUF_INIT(userbuf,sizelog2)	{ .size = 1 << (sizelog2), .buf = (userbuf), .write = 0, .read = 0, .unread = 0, .empty = 1, .allread = 1, }

//        read              unread                      write
//  ________|_________________|___________________________|_________________
// |eeeeeeee.rrrrrrrrrrrrrrrrr.uuuuuuuuuuuuuuuuuuuuuuuuuuu.eeeeeeeeeeeeeeeee|
//  \                               buffer                                 /
// r: data are given to user with cbuf_read_ptr(), will be freed with cbuf_ack()
// u: unread data, can be read with cbuf_read() or pointed with cbuf_read_ptr()
// e: empty space, filled with cbuf_write() or pointed to by cbuf_write_ptr()
// for *_ptr(): returned len is desired_len truncated to buffer's border
// so use cbuf_all_is_read() to check whether more data is readable
// and cbuf_is_full() to check whether more data is writable
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DO NOT MIX cbuf_read() and cbuf_read_ptr()+cbuf_ack() (cbuf_write*() can be mixed)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

///////////////////////////////////////////////////////////
// macro always available

// cbuf_is_empty() can be false while cbuf_all_is_read is true when using cbuf_read_ptr()
// cbuf_is_empty()=cbuf_all_is_read() when using cbuf_read()
#define cbuf_is_empty(cb) 		((cb)->empty)
#define cbuf_all_is_read(cb)		((cb)->allread)
#define cbuf_is_full(cb)  		(((cb)->read == (cb)->write) && !(cb)->empty)
#define cbuf_flush(cb)			do { (cb)->read = (cb)->unread = (cb)->write = 0; (cb)->allread = (cb)->empty = 1; } while (0)

// return available space for write
size_t cbuf_write_available (cbuf_t* cb);

void cbuf_init (cbuf_t* cb, char* userbuf, char sizelog2);

///////////////////////////////////////////////////////////
// functions with copy

// regular write/copy user data into buffer
// return effective size copied <= desired_len
cbsize_t	cbuf_write	(cbuf_t* cb, const void* data, cbsize_t desired_len);

// regular read/copy buffer into user data
// return effctive size copied <= desired_len
cbsize_t	cbuf_read	(cbuf_t* cb, char* buf, cbsize_t desired_len);

///////////////////////////////////////////////////////////
// functions with pointers

// get buffer pointer to directly write into it
// return available len <= desired_len
cbsize_t	cbuf_write_ptr	(cbuf_t* cb, char** buf, cbsize_t desired_len);

// get buffer pointer to read
// return available len <= desired_len
cbsize_t	cbuf_read_ptr	(cbuf_t* cb, char** buf, cbsize_t desired_len);

// acknowledge read data (that can be discarded = overwritten)
void		cbuf_ack	(cbuf_t* cb, cbsize_t len);

#endif // _CBUF_H_
