
#include <string.h>

#include "cbuf.h"

#if 1
#define do_flush(cb) cbuf_flush(cb)	// flushing when empty
#else
#define do_flush(cb) do { } while (0)	// doing nothing
#endif

size_t cbuf_write_available (cbuf_t* cb)
{
	if (cb->read < cb->write || (cb->read == cb->write && cb->empty))
		return (cb->size - cb->write) + cb->read;
	else
		return cb->read - cb->write;
}

void cbuf_init (cbuf_t* cb, char* userbuf, size_t size)
{
	cb->size = userbuf? size: 0;
	cb->buf = userbuf;
	cb->write = cb->read = cb->unread = 0;
	cb->empty = cb->allread = 1;
}

static size_t cbuf_write_available_chunk (cbuf_t* cb)
{
	return (cb->read < cb->write || (cb->read == cb->write && cb->empty))?
		cb->size - cb->write:
		cb->read - cb->write;
}

cbsize_t cbuf_write (cbuf_t* cb, const void* data, cbsize_t desired_len)
{
	cbsize_t want = desired_len;
	cbsize_t ret = 0;
	while (want && !cbuf_is_full(cb))
	{
		cbsize_t writable_len = cbuf_write_available_chunk(cb);
		cbsize_t chunk = (want > writable_len)? writable_len: want;
		memcpy(cb->buf + cb->write, ((const char*)data) + ret, chunk);
		ret += chunk;
		want -= chunk;
		if ((cb->write += chunk) == cb->size)
			cb->write = 0;
		cb->empty = cb->allread = 0;
	}
	return ret;
}

cbsize_t cbuf_write_ptr (cbuf_t* cb, char** buf, cbsize_t desired_len)
{
	*buf = cb->buf + cb->write;
	cbsize_t writable_len = cbuf_write_available_chunk(cb);
	if (desired_len > writable_len)
		desired_len = writable_len;
	if (desired_len)
	{
		if ((cb->write += desired_len) == cb->size)
			cb->write = 0;
		cb->empty = cb->allread = 0;
	}
	return desired_len;
}

/////////////////////////////////////////////////////////////////////////////

cbsize_t cbuf_read (cbuf_t* cb, char* data, cbsize_t desired_len)
{
	// cb->read must always be equal when using this function
	// (do not use cbuf_*_ptr)

	cbsize_t want = desired_len;
	cbsize_t ret = 0;
	while (want && !cbuf_is_empty(cb))
	{
		cbsize_t readable_len = cb->read >= cb->write?
				cb->size - cb->read:
				cb->write - cb->read;
		cbsize_t chunk = (want > readable_len)? readable_len: want;
		memcpy(data + ret, cb->buf + cb->read, chunk);
		ret += chunk;
		want -= chunk;
		if ((cb->read += chunk) == cb->size)
			cb->read = 0;
		if ((cb->empty = cb->allread = (cb->read == cb->write)))
			do_flush(cb); // bigger_chunk, less loop
	}
	return ret;
}

/////////////////////////////////////////////////////////////////////////////

cbsize_t cbuf_read_ptr (cbuf_t* cb, char** buf, cbsize_t desired_len)
{
	*buf = cb->buf + cb->unread;
	cbsize_t readable_len = ((cb->unread > cb->write) || (cb->unread == cb->write && !cb->allread))?
			cb->size - cb->unread:
			cb->write - cb->unread;
	if (desired_len > readable_len)
		desired_len = readable_len;
	if ((cb->unread += desired_len) == cb->size)
		cb->unread = 0;
	cb->allread = (cb->unread == cb->write);
	return desired_len;
}

void cbuf_ack (cbuf_t* cb, cbsize_t len)
{
	if (len)
	{
		// len must not be 0 or cb->empty could mistakingly flip
		if ((cb->read += len) >= cb->size)
			cb->read -= cb->size;
		if ((cb->empty = (cb->read == cb->write)))
			do_flush(cb); // bigger chunk, less loop
	}
}
