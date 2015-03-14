
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "cbtools.h"

char sprintbuf [SPRINTBUFSIZE];

void tooshortbuf (char* str, size_t size)
{
	static const char tooshortbuf [] = ">>><";
	size_t shift = (size < sizeof tooshortbuf)? sizeof tooshortbuf - 1 - size: 0;
	strcpy(str + size + shift - (sizeof tooshortbuf - 1), tooshortbuf + shift);
}

int cb_printf (cb_t* cb, const char* fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = cb_vprintf(cb, fmt, ap);
	va_end(ap);
	return ret;
}

int cb_vprintf (cb_t* cb, const char* fmt, va_list ap)
{
	size_t cbavail = cb_write_available(cb);
	if (cbavail + 1 > SPRINTBUFSIZE)
		cbavail = SPRINTBUFSIZE - 1;

	if (vsnprintf(sprintbuf, cbavail + 1, fmt, ap) >= cbavail + 1)
		tooshortbuf(sprintbuf, cbavail + 1);

	return cb_write(cb, sprintbuf, strlen(sprintbuf));
}
