#include <stdlib.h>

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h" 
#include "microrl.h"
#include "console.h"
#include "env.h"
#include "lwip/mem.h" // mem_realloc

printf_f console_printf = SERIAL_PRINTF;

#define CONSOLE_PRIO 1

int log_level = LOG_LEVEL_DEFAULT;

static microrl_t rl;
#define prl (&rl)
static int console_locked = 0;
static int passthrough = ENABLE_PASSTHROUGH_AT_BOOT;

const char* loglevnam (int lev)
{
	switch (lev)
	{
	case LOG_ERR: return "ERR";
	case LOG_WARN: return "WARN";
	case LOG_NOTICE: return "notice";
	default: return "dbg";
	}
}

#if 0
struct console {
	char name[16];
	void (*write)(char* buf, int len);	
};
#endif

void console_lock(int l)
{
	console_locked = l;
	if (!l) 
		microrl_print_prompt(prl);
}

void enable_passthrough(int v)
{
	passthrough = v;
	console_lock(v);
}

void console_insert(char c)
{
#if 0
WIP direct link serial<->wifi
	static uint32 esc_time = 0;
	static int esc_count = 0;
	
	if (passthrough)
	{
		//console_printf("%c", c);
		ets_uart_printf("@%c,%d", c,c);
		
		if (c != KEY_ESC)
			esc_count = 0;
		else
		{
			uint32 now = system_get_time();
			if (++esc_count > 1)
			{
				if (now - esc_time < ESC_SPACE)
					esc_count = 0;
				else if (esc_count == ESC_COUNT)
				{
					// disable passthrough
					enable_passthrough(0);
					console_printf("console on serial line\n");
					esc_count = 0;
				}
			}
			esc_time = now;
		}
	}
	else
#endif
		if (!console_locked || (c) == KEY_ETX)
			microrl_insert_char (prl, c);
}

void console_write(char *buf, int len)
{
	while (len--)
		console_insert(*buf++);
}

static void  task_console(os_event_t *evt)
{
	console_insert(evt->par);
}

static void  rl_print(const char *str)
{
	if (!console_locked)
		console_printf(str);
}

static int do_help(int argc, const char* const* argv);

static struct console_cmd  cmd_first  __attribute__((section(".console_firstcmd"))) = {
	.name = "help",
	.handler = do_help,
	.help = "Show this message",
	.required_args = -1,
	.maximum_args = -1,
} ;

static struct console_cmd  cmd_last  __attribute__((section(".console_lastcmd"))) = {
};


#define FOR_EACH_CMD(iterator) for (iterator=&cmd_first; iterator<&cmd_last; iterator++) 

static int do_help(int argc, const char* const* argv)
{
	struct console_cmd *cmd;
	console_printf("\n");
	FOR_EACH_CMD(cmd) {
		console_printf("%-10s - %s\n", cmd->name, cmd->help);
	}
	return 0;
}

static void sigint(void)
{
	struct console_cmd *cmd;
	console_printf("\nINTERRUPT\n");
	FOR_EACH_CMD(cmd) {
		if (cmd->interrupt)
			cmd->interrupt();
	}
	console_lock(0); /* Unlock console immediately */
}


int execute (int argc, const char * const * argv)
{
	struct console_cmd *cmd; 
	console_printf("\n");
	FOR_EACH_CMD(cmd) {
		if (strcasecmp(cmd->name, argv[0])==0) { 
			if ((cmd->required_args != -1) && argc < cmd->required_args)
				goto err_more_args; 
			if ((cmd->maximum_args != -1) && (argc > cmd->maximum_args))
				goto err_too_many_args;
			cmd->handler(argc, argv);
			return 0;
		}
	}
	console_printf("\nCommand %s not found, type 'help' for a list\n", argv[0]);
	return 1;
err_more_args:
	console_printf("\nCommand %s requires at least %d args, %d given\n", 
			argv[0], cmd->required_args, argc);
	return 1;
err_too_many_args:
	console_printf("\nCommand %s takes a maximum of %d args, %d given\n", 
			argv[0], cmd->maximum_args, argc);
	return 1;
}

const char ** completion(int argc, const char* const* argv)
{
	#define COMPL_MAX_RESULTS	15
	#define COMPL_BUF		200
	static const char* noroom = "(completion: not enough memory)";
	static char complbuf [COMPL_BUF];
	static const char* compl [COMPL_MAX_RESULTS + 1];
	static const char* nocompl [] = { NULL };
	
	if (argc == 1)
	{
		struct console_cmd *cmd;
		const char* part = argv[0];
		size_t partlen = strlen(part);

		int ncompl = 0;
		FOR_EACH_CMD(cmd)
			if (strncasecmp(cmd->name, part, partlen) == 0)
				ncompl++;
		int i = 0;
		size_t pos = 0;
		FOR_EACH_CMD(cmd)
			if (strncasecmp(cmd->name, part, partlen) == 0)
			{
				if (i == COMPL_MAX_RESULTS)
					compl[i++] = noroom;
				else if (i < COMPL_MAX_RESULTS)
				{
					const char* src = cmd->name + (i == 0 && ncompl > 1? partlen: 0);
					size_t srcsize = strlen(src) + 1;
					if (pos + srcsize <= COMPL_BUF)
					{
						compl[i++] = complbuf + pos;
						memcpy(complbuf + pos, src, srcsize);
						pos += srcsize;
					}
					else
						compl[i++] = noroom;
				}
			}
		compl[i] = NULL;
		
		return (const char**)compl;
	}
	
	return nocompl;
}

void console_init(int qlen) {
	/* Microrl init */
	microrl_init (prl, &rl_print);
	microrl_set_execute_callback (prl, execute);
	microrl_set_sigint_callback(prl, sigint);
	microrl_set_complete_callback(prl, completion);

	const char *p = env_get("hostname");
	if (p)
		microrl_set_prompt(p);

	console_printf("\n === Press enter to activate this console === \n");	
	os_event_t *queue = os_malloc(sizeof(os_event_t) * qlen);
	system_os_task(task_console, CONSOLE_PRIO, queue, qlen);
}

