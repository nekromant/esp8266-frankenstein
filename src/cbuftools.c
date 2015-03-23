
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "cbuftools.h"

char sprintbuf [SPRINTBUFSIZE];

void tooshortbuf (char* str, size_t size)
{
	static const char tooshortbuf [] = ">>><";
	size_t shift = (size < sizeof tooshortbuf)? sizeof tooshortbuf - 1 - size: 0;
	strcpy(str + size + shift - (sizeof tooshortbuf - 1), tooshortbuf + shift);
}

int cbuf_printf (cbuf_t* cb, const char* fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = cbuf_vprintf(cb, fmt, ap);
	va_end(ap);
	return ret;
}

int cbuf_vprintf (cbuf_t* cb, const char* fmt, va_list ap)
{
	size_t cbavail = cbuf_write_available(cb);
	if (cbavail + 1 > SPRINTBUFSIZE)
		cbavail = SPRINTBUFSIZE - 1;

	if (vsnprintf(sprintbuf, cbavail + 1, fmt, ap) >= cbavail + 1)
		tooshortbuf(sprintbuf, cbavail + 1);

	return cbuf_write(cb, sprintbuf, strlen(sprintbuf));
}
