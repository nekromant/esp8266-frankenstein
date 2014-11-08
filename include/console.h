#ifndef CONSOLE_H
#define CONSOLE_H

struct console_cmd {
	const char *name; 
	const char *help;
	int required_args;
	int maximum_args;
	int (*handler)(int argc, const char * const * argv);
	char ** (*get_completion) (int argc, const char * const * argv );
} ;

#define CONSOLE_CMD(_name, _reqargs, _maxargs, _handler, _completion, _help) \
	struct console_cmd  cmd_ ##_name  __attribute__((section(".console_cmd"))) = { \
		.name = #_name,						\
		.required_args = _reqargs,				\
		.maximum_args  = _maxargs,				\
		.handler = _handler,					\
		.get_completion = _completion,				\
		.help = _help,						\
	}


#endif
