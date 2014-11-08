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

//TODO: Network console 

extern int ets_uart_printf(const char *fmt, ...);

#define CONSOLE_PRIO 1


#define CONSOLE_PRIO 1

static microrl_t rl;
static microrl_t * prl = &rl;
static int console_locked = 0;

void console_lock(int l)
{
	console_locked = l;
	if (!l) 
		ets_uart_printf("\nblackblade > ");
}

static void ICACHE_FLASH_ATTR task_console(os_event_t *evt)
{
	if (!console_locked)
		microrl_insert_char (prl, (char) evt->par);
}

static void ICACHE_FLASH_ATTR rl_print(char *str)
{
	if (!console_locked)
		ets_uart_printf(str);
}


static void sigint(void)
{
	ets_uart_printf("\nINTERRUPT\n");
	console_lock(0); /* Unlock console immediately */
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
	ets_uart_printf("\n");
	FOR_EACH_CMD(cmd) {
		ets_uart_printf("%-10s - %s\n", cmd->name, cmd->help);
	}
}

int execute (int argc, const char * const * argv)
{
	struct console_cmd *cmd; 
	ets_uart_printf("\n");
	FOR_EACH_CMD(cmd) {
		if (strcmp(cmd->name, argv[0])==0) { 
			if ((cmd->maximum_args != -1) && argc < cmd->required_args)
				goto err_more_args; 
			if ((cmd->maximum_args != -1) && (argc > cmd->maximum_args))
				goto err_too_many_args;
			cmd->handler(argc, argv);
			return 0;
		}
	}
	ets_uart_printf("\nCommand %s not found, type 'help' for a list\n", argv[0]);
	return 1;
err_more_args:
	ets_uart_printf("\nCommand %s requires at least %d args, %d given\n", 
			argv[0], cmd->required_args, argc);
	return 1;
err_too_many_args:
	ets_uart_printf("\nCommand %s takes a maximum of %d args, %d given\n", 
			argv[0], cmd->maximum_args, argc);
	return 1;
}

void console_init(int qlen) {
	/* Microrl init */
	microrl_init (prl, &rl_print);
	microrl_set_execute_callback (prl, execute);
	microrl_set_sigint_callback(prl, sigint);
	ets_uart_printf("\n\n\n");
	ets_uart_printf("Antares blackblade " CONFIG_VERSION_STRING " @ ESP8266. \n");	
	ets_uart_printf("(c) Andrew 'Necromant' Andrianov 2014 <andrew@ncrmnt.org>\n");	
	ets_uart_printf("\nMemory Layout:\n");	
	system_print_meminfo();
	system_set_os_print(0);
	ets_uart_printf("\n === Press enter to activate this console === \n");	
	os_event_t *queue = os_malloc(sizeof(os_event_t) * qlen);
	system_os_task(task_console, CONSOLE_PRIO, queue, qlen);
}
