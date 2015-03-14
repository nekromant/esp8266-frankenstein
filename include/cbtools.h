#ifndef _CBTOOLS_H_
#define _CBTOOLS_H_

#include "cb.h"

#define SPRINTBUFSIZE 128

extern char sprintbuf [SPRINTBUFSIZE];

void tooshortbuf (char* str, size_t size);
int cb_printf (cb_t* sb, const char* fmt, ...);
int cb_vprintf (cb_t* cb, const char* fmt, va_list ap);

#endif // _CBTOOLS_H_
