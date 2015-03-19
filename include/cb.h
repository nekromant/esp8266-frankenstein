#ifndef CB_H
#define CB_H

#include <stdlib.h>

typedef size_t	cbsize_t;

// Circular buffer object
typedef struct cb_s
{
	cbsize_t	size;		// buffer size
	cbsize_t	read;		// index of oldest data
	cbsize_t	write;		// index at which to write new data
	cbsize_t	unread;		// index of next unread data
	char*		buf;		// data buffer
	char		empty;		// no data
	char		allread;	// ((read == unread) == all is swallowed)
} cb_t;

// GIVE log2() of desired size
#define CB_INIT(userbuf,sizelog2)	{ .size = 1 << (sizelog2), .buf = (userbuf), .write = 0, .read = 0, .unread = 0, .empty = 1, .allread = 1, }

//        read              unread                      write
//  ________|_________________|___________________________|_________________
// |eeeeeeee.rrrrrrrrrrrrrrrrr.uuuuuuuuuuuuuuuuuuuuuuuuuuu.eeeeeeeeeeeeeeeee|
//  \                               buffer                                 /
// r: data are given to user with cb_read_ptr(), will be freed with cb_ack()
// u: unread data, can be read with cb_read() or pointed with cb_read_ptr()
// e: empty space, filled with cb_write() or pointed to by cb_write_ptr()
// for *_ptr(): returned len is desired_len truncated to buffer's border
// so use cb_all_is_read() to check whether more data is readable
// and cb_is_full() to check whether more data is writable
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DO NOT MIX cb_read() and cb_read_ptr()+cb_ack() (cb_write*() can be mixed)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

///////////////////////////////////////////////////////////
// macro always available

// cb_is_empty() can be false while cb_all_is_read is true when using cb_read_ptr()
// cb_is_empty()=cb_all_is_read() when using cb_read()
#define cb_is_empty(cb) 		((cb)->empty)
#define cb_all_is_read(cb)		((cb)->allread)
#define cb_is_full(cb)  		(((cb)->read == (cb)->write) && !(cb)->empty)
#define cb_flush(cb)			do { (cb)->read = (cb)->unread = (cb)->write = 0; (cb)->allread = (cb)->empty = 1; } while (0)

// return available space for write
size_t cb_write_available (cb_t* cb);

void cb_init (cb_t* cb, char* userbuf, char sizelog2);

///////////////////////////////////////////////////////////
// functions with copy

// regular write/copy user data into buffer
// return effective size copied <= desired_len
cbsize_t	cb_write	(cb_t* cb, const char* data, cbsize_t desired_len);

// regular read/copy buffer into user data
// return effctive size copied <= desired_len
cbsize_t	cb_read		(cb_t* cb, char* buf, cbsize_t desired_len);

///////////////////////////////////////////////////////////
// functions with pointers

// get buffer pointer to directly write into it
// return available len <= desired_len
cbsize_t	cb_write_ptr	(cb_t* cb, char** buf, cbsize_t desired_len);

// get buffer pointer to read
// return available len <= desired_len
cbsize_t	cb_read_ptr	(cb_t* cb, char** buf, cbsize_t desired_len);

// acknowledge read data (that can be discarded = overwritten)
void		cb_ack		(cb_t* cb, cbsize_t len);

#endif
