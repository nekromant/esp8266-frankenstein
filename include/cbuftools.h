#ifndef _CBTOOLS_H_
#define _CBTOOLS_H_

#include <stdarg.h>

#include "cbuf.h"

// was 128 but flash_scan help line is bigger:
#define SPRINTBUFSIZE 256

extern char sprintbuf [SPRINTBUFSIZE];

void tooshortbuf (char* str, size_t size);
int cbuf_printf (cbuf_t* cb, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));
int cbuf_vprintf (cbuf_t* cb, const char* fmt, va_list ap);

#endif // _CBTOOLS_H_
