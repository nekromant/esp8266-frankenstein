
#include <string.h>

#include "cb.h"

#if 0
#define do_flush(cb) cb_flush(cb)	// flushing when empty
#else
#define do_flush(cb) do { } while (0)	// doing nothing
#endif

/////////////////////////////////////////////////////////////////////////////

cbsize_t cb_write (cb_t* cb, const char* data, cbsize_t desired_len)
{
	cbsize_t want = desired_len;
	cbsize_t ret = 0;
	while (want && !cb_is_full(cb))
	{
		cbsize_t writable_len = cb->read <= cb->write?
				cb->size - cb->write:
				cb->read - cb->write;
		cbsize_t chunk = (want > writable_len)? writable_len: want;
		memcpy(cb->buf + cb->write, data + ret, chunk);
		ret += chunk;
		want -= chunk;
		cb->write = (cb->write + chunk) & (cb->size - 1);
		cb->empty = cb->allread = 0;
	}
	return ret;
}

cbsize_t cb_read (cb_t* cb, char* data, cbsize_t desired_len)
{
	// cb->read must always be equal when using this function
	// (do not use cb_*_ptr)

	cbsize_t want = desired_len;
	cbsize_t ret = 0;
	while (want && !cb_is_empty(cb))
	{
		cbsize_t readable_len = cb->write <= cb->read?
				cb->size - cb->read:
				cb->write - cb->read;
		cbsize_t chunk = (want > readable_len)? readable_len: want;
		memcpy(data + ret, cb->buf + cb->read, chunk);
		ret += chunk;
		want -= chunk;
		cb->read = (cb->read + chunk) & (cb->size - 1);
		if ((cb->empty = cb->allread = (cb->read == cb->write)))
			do_flush(cb); // bigger_chunk, less loop
	}
	return ret;
}

/////////////////////////////////////////////////////////////////////////////

cbsize_t cb_write_ptr (cb_t* cb, char** buf, cbsize_t desired_len)
{
	*buf = cb->buf + cb->write;
	cbsize_t writable_len = (cb->read < cb->write || (cb->read == cb->write && cb->empty))?
			cb->size - cb->write:
			cb->read - cb->write;
	if (desired_len > writable_len)
		desired_len = writable_len;
	if (desired_len)
	{
		cb->write = (cb->write + desired_len) & (cb->size - 1);
		cb->empty = cb->allread = 0;
	}
	return desired_len;
}

cbsize_t cb_read_ptr (cb_t* cb, char** buf, cbsize_t desired_len)
{
	*buf = cb->buf + cb->unread;
	cbsize_t readable_len = ((cb->unread > cb->write) || (cb->unread == cb->write && !cb->allread))?
			cb->size - cb->unread:
			cb->write - cb->unread;
	if (desired_len > readable_len)
		desired_len = readable_len;
	cb->unread = (cb->unread + desired_len) & (cb->size - 1);
	cb->allread = (cb->unread == cb->write);
	return desired_len;
}

void cb_ack (cb_t* cb, cbsize_t len)
{
	if (len)
	{
		// this must not be called with 0
		// (cb->empty could mistakingly flip)
		cb->read = (cb->read + len) & (cb->size - 1);
		if ((cb->empty = (cb->read == cb->write)))
			do_flush(cb); // bigger chunk, less loop
	}
}
