
#ifndef _MAIN_H
#define _MAIN_H

#include "console.h"

struct slogger_instance;
struct slogger_instance  *svclog_get_global_instance();

#define FR_CONSTRUCTOR(fn)                                              \
	static void fn();                                                     \
	__attribute__((__section__(".fr_init_array"))) void *fn ## _high = fn; \
	static void __attribute__((__used__)) fn()


extern printf_f console_printf; // = ets_uart_printf;

void request_default_environment(void);
void print_hello_banner(void);
const char* fr_request_hostname(void);

#endif // _MAIN_H
