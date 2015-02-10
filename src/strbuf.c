
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "console.h"

#include "strbuf.h"

#define STRBUF_DEFAULT_SIZE	512

void strbuf_init (strbuf* sb)
{
	strbuf empty = STRBUF_INIT;
	*sb = empty;
}

int strbuf_incsize (strbuf* sb, size_t grow)
{
	char* newbuf = (char*)realloc(sb->buf, sb->size + grow);
	if (newbuf == NULL)
	{
		LOG(LOG_ERR, "cannot realloc from %d by %d bytes\n", sb->size, grow);
		return -1;
	}
	sb->buf = newbuf;
	sb->size += grow;
	return 0;
}

int strbuf_printf (strbuf* sb, const char* fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = strbuf_vprintf(sb, fmt, ap);
	va_end(ap);
	return ret;
}

int strbuf_vprintf (strbuf* sb, const char* fmt, va_list ap)
{
	size_t ret, max = sb->size - sb->len; 
	
	do
	{
		ret = max? vsnprintf(sb->buf + sb->len, max, fmt, ap): max - 1;
		if (ret == max - 1 && strbuf_grow(sb) == -1)
			return -1;
	} while (ret == max - 1);

	sb->len += ret;
	return ret;
}

int strbuf_memcpy (strbuf* sb, const void* src, size_t srclen)
{
	if (   (sb->size == 0 || sb->len + srclen > sb->size)
	    && strbuf_incsize(sb, MAX(STRBUF_INC_SIZE, sb->size - sb->len + srclen)) == -1)
		return -1;
	memcpy(sb->buf + sb->len, src, srclen);
	sb->len += srclen;
	return 0;
}

void strbuf_release (strbuf* sb)
{
	if (sb->buf)
		free(sb->buf);
	sb->size = sb->len = 0;
}
