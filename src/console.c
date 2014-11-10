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


#define CONSOLE_PRIO 1

static microrl_t rl;
static microrl_t * prl = &rl;
static int console_locked = 0;

void console_lock(int l)
{
	console_locked = l;
	if (!l) 
		console_printf("\nblackblade > ");
}

static void ICACHE_FLASH_ATTR task_console(os_event_t *evt)
{
	if (!console_locked)
		microrl_insert_char (prl, (char) evt->par);
}


void console_exec(char *str) {
	while (*str)
		microrl_insert_char (prl, (char) *str++);
}

static void ICACHE_FLASH_ATTR rl_print(char *str)
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


void console_print_verinfo()
{
	console_printf("\n\n\nFrankenstein ESP8266 Firmware\n");
	console_printf("Powered by Antares " CONFIG_VERSION_STRING "\n");	
	console_printf("(c) Andrew 'Necromant' Andrianov 2014 <andrew@ncrmnt.org>\n");	
	console_printf("This is free software (where possible), published under the terms of GPLv2\n");	
}

void console_init(int qlen) {
	/* Microrl init */
	microrl_init (prl, &rl_print);
	microrl_set_execute_callback (prl, execute);
	microrl_set_sigint_callback(prl, sigint);
	console_print_verinfo();
	console_printf("\nMemory Layout:\n");	
	system_print_meminfo();
	system_set_os_print(0);
	console_printf("\n === Press enter to activate this console === \n");	
	os_event_t *queue = os_malloc(sizeof(os_event_t) * qlen);
	system_os_task(task_console, CONSOLE_PRIO, queue, qlen);
}

