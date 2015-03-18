#ifndef _CBTOOLS_H_
#define _CBTOOLS_H_

#include <stdarg.h>

#include "cb.h"

// was 128 but flash_scan help line is bigger:
#define SPRINTBUFSIZE 256

extern char sprintbuf [SPRINTBUFSIZE];

void tooshortbuf (char* str, size_t size);
int cb_printf (cb_t* cb, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));
int cb_vprintf (cb_t* cb, const char* fmt, va_list ap);

#endif // _CBTOOLS_H_
