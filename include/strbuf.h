
#ifndef _STRBUF_H_
#define _STRBUF_H_

#include <stdarg.h>

#define STRBUF_INC_SIZE		256

typedef struct 
{
	size_t size, len;
	char* buf;

	// at user discretion: (not used by strbuf_* functions)
	size_t uptr;
} strbuf;

#define STRBUF_INIT		{ .size = 0, .len = 0, .buf = NULL, .uptr = 0, }

#define MAX(a,b) ({typeof(a) aa = (a); typeof(b) bb = (b); bb > aa? bb: aa; })
#define MIN(a,b) ({typeof(a) aa = (a); typeof(b) bb = (b); bb < aa? bb: aa; })

void strbuf_init (strbuf* sb);

void strbuf_clear (strbuf* sb);
int strbuf_grow (strbuf* sb);
int strbuf_incsize (strbuf* sb, size_t grow);
int strbuf_vprintf (strbuf* sb, const char* fmt, va_list ap);
int strbuf_printf (strbuf* sb, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));
int strbuf_memcpy (strbuf* sb, const void* src, size_t len);
int strbuf_strcpy (strbuf* sb, const char* src);
void strbuf_release (strbuf* sb);

#define strbuf_endptr(sb)	((sb)->buf + (sb)->len)
#define strbuf_strcpy(sb, src)	strbuf_memcpy((sb), (src), strlen(src) + 1)
#define strbuf_grow(sb)		strbuf_incsize((sb), STRBUF_INC_SIZE)

#endif // _STRBUF_H_
