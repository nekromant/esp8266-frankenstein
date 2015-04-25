#ifndef CONSOLE_H
#define CONSOLE_H

#define ESC_SPACE	300000
#define ESC_COUNT	3

struct console_cmd {
	const char *name; 
	const char *help;
	int required_args;
	int maximum_args;
	int (*handler)(int argc, const char * const * argv);
	char ** (*get_completion) (int argc, const char * const * argv );
	void (*interrupt)(void);
} ;

#define CONSOLE_CMD(_name, _reqargs, _maxargs, _handler, _inthandler, _completion, _help) \
	struct console_cmd  cmd_ ##_name				\
	__attribute__ ((used))						\
	__attribute__((section(".console_cmd"))) = {			\
		.name = #_name,						\
		.required_args = _reqargs,				\
		.maximum_args  = _maxargs,				\
		.handler = _handler,					\
		.interrupt = _inthandler,				\
		.get_completion = _completion,				\
		.help = _help,						\
	}


#define SERIAL_PRINTF				ets_uart_printf

#define ENABLE_PASSTHROUGH_AT_BOOT		0

#define LOG_ERR					0
#define LOG_WARN				1
#define LOG_NOTICE				2
#define LOG_DEBUG				3
#define LOG_DEBUG2				4	// very verbose
#define LOG_LEVEL_MAX				99

// LOG_LEVEL_MAX: print code will not be compiled if given level is higher
//#define LOG_LEVEL_DEFAULT	LOG_NOTICE
#define LOG_LEVEL_DEFAULT	LOG_LEVEL_MAX

extern int log_level;
#define set_log_level(lvl)	do { log_level = (lvl); } while (0)
#define __BASEFILE__		(strrchr(__FILE__, '/')?:__FILE__)
#define LOGN(level,b...)	do { if ((level) <= LOG_LEVEL_MAX && (level) <= log_level) { console_printf("%s: ", loglevnam(level)); console_printf(b); console_printf("\n"); } } while (0)
#define LOGSERIALN(level,b...)	do { if ((level) <= LOG_LEVEL_MAX && (level) <= log_level) {  SERIAL_PRINTF("%s: ", loglevnam(level));  SERIAL_PRINTF(b);  SERIAL_PRINTF("\n"); } } while (0)
#define LOG(level,b...)		do { if ((level) <= LOG_LEVEL_MAX && (level) <= log_level) { console_printf("%s: ", loglevnam(level)); console_printf(b); console_printf(" (%s:%d)\n", __BASEFILE__, __LINE__); } } while (0)
#define LOGSERIAL(level,b...)	do { if ((level) <= LOG_LEVEL_MAX && (level) <= log_level) {  SERIAL_PRINTF("%s: ", loglevnam(level));  SERIAL_PRINTF(b);  SERIAL_PRINTF(" (%s:%d)\n", __BASEFILE__, __LINE__); } } while (0)

typedef int (*printf_f)(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

// add this to antares/esp8266 missing includes
#include <stdarg.h>
#include <c_types.h>
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
       
extern printf_f console_printf;

#define HELPSTR_NEWLINE "\n             "

void console_init(int qlen);
void console_insert(char c);
void console_lock(int l);
void console_write(char *buf, int len);
void console_exec(char *str);

void enable_passthrough(int v);

const char* loglevnam (int lev);


#endif

