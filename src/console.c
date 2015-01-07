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

extern int ets_uart_printf(const char *fmt, ...);
int (*console_printf)(const char *fmt, ...) = ets_uart_printf;


#define CONSOLE_PRIO 1

static microrl_t rl;
static microrl_t * prl = &rl;
static int console_locked = 0;

struct console {
	char name[16];
	void (*write)(char* buf, int len);	
};


void console_lock(int l)
{
	console_locked = l;
	if (!l) 
		microrl_print_prompt(prl);
}

void console_insert(char c)
{
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


void console_exec(char *str) {
	while (*str)
		microrl_insert_char (prl, (char) *str++);
}

static void  rl_print(char *str)
{
	if (!console_locked)
		console_printf(str);
}



static int do_help(int argc, const char*argv[]);

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


static int do_help(int argc, const char*argv[])
{
	struct console_cmd *cmd;
	console_printf("\n");
	FOR_EACH_CMD(cmd) {
		console_printf("%-10s - %s\n", cmd->name, cmd->help);
	}
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
		if (strcmp(cmd->name, argv[0])==0) { 
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

/*
#define COMPLETION_COUNT 8
#define COMPLETION_BUF   128

static char *compl_array[COMPLETION_COUNT+1];
static char compl_buf[COMPLETION_BUF];
static int compl_ptr; 
static int compl_buf_ptr;
 
int compl_push(char* str)
{
	if (compl_ptr >= COMPLETION_COUNT)
		return -1;
	
	char* s = &compl_buf[compl_buf_ptr];
	int sz = strlen(str);
	if (COMPLETION_BUF)
	{
		
	}
}
char ** completion(int argc, const char* const* argv)
{
	compl_ptr = 0;
	compl_buf_ptr = 0;

	if (argc == 1) {
		
	}
	// TODO: actual command completion
}
*/


#include <stdio.h>

void console_init(int qlen) {
	/* Microrl init */
	microrl_init (prl, &rl_print);
	microrl_set_execute_callback (prl, execute);
	microrl_set_sigint_callback(prl, sigint);

	char *p = env_get("hostname");
	if (p)
		microrl_set_prompt(p);

	console_printf("\n === Press enter to activate this console === \n");	
	os_event_t *queue = os_malloc(sizeof(os_event_t) * qlen);
	system_os_task(task_console, CONSOLE_PRIO, queue, qlen);
}

