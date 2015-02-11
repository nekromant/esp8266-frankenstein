
#ifndef _MAIN_H
#define _MAIN_H


/*
   !!
	DO NOT USE MALLOC
   !!
  
   USE MEM_ALLOC INSTEAD (MEM_ALLOC/MEM_REALLOC/MEM_FREE)
   which are defined in lwip/mem.h and defined as PvPort*...
   or os_*alloc()...
*/


extern int (*console_printf)(const char *fmt, ...);// = ets_uart_printf;

void request_default_environment(void);
void print_hello_banner(void);
const char* fr_request_hostname(void);

#endif // _MAIN_H
