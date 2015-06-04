#include "user_interface.h"
#include "missing.h"

#include "espconn.h"
#include "gpio.h"
#include "main.h"
#include "helpers.h"
#include "sched.h"

#include <stdlib.h>
#include <stdlib.h>
#include <generic/macros.h>

#define MAX_SCHED_ENTRIES 5

static uint8_t last;
static uint16_t mod;
ETSTimer schedTimer;

static void sched_timer_cb(void *);

struct {
	int modulus;
	const char *cmd;
} schedule[MAX_SCHED_ENTRIES];

void ICACHE_FLASH_ATTR
sched_init(void)
{
	last = 0;
	mod = 0;
	os_timer_disarm(&schedTimer);
	os_timer_setfn(&schedTimer, (os_timer_func_t *)sched_timer_cb, NULL);
}

int ICACHE_FLASH_ATTR
sched_add(const char *cmd, int mod)
{
	if (last < MAX_SCHED_ENTRIES) {
		if (cmd != NULL && mod) {
			schedule[last].cmd = cmd;
			schedule[last].modulus = mod;
		}
		last++;
	}
	else
		return -1;
	if (last == 1) // First entry, arm the timer
	{
		os_timer_arm(&schedTimer, 1000, 1);
		console_printf("Timer armed, 1000ms\r\n");
	}

	return last;
}

static void ICACHE_FLASH_ATTR
sched_timer_cb(void *arg)
{
	int i;

	if (last == 0) {	// Sanity check.
		os_timer_disarm(&schedTimer);
		return;
	}

	for (i=0;i<last;i++)
	{
		int j=0;
		if (mod % schedule[i].modulus == 0) {
			console_insert('\r');
			console_insert('\n');
			while(schedule[i].cmd[j] != '\0')
				console_insert(schedule[i].cmd[j++]);
			console_insert('\r');
			console_insert('\n');
		}
	}
	mod++;	// rolls at uint16_t
}

static int
do_every(int argc, const char* const* argv)
{
	uint16_t mod;
	char *cmd;
	int l;

	if (argc < 3) {
		console_printf("Usage: \r\n\t every <seconds> <command>\r\n");
		return 0;
	}
	mod = (uint16_t)skip_atoul((const char **)&argv[1]);

	if (mod == 0)
		return 0;
	l = strlen(argv[2]);	// Assume cmd is a single arg in ''s

	cmd = os_malloc(l);
	if (cmd == NULL) {
		console_printf("Out of memory?\r\n");
		return 0;
	}
	strcpy(cmd, argv[2]);

	console_printf("Scheduling [%s] to execute every %d seconds\r\n", cmd, mod);

	return sched_add(cmd, mod);
}

CONSOLE_CMD(every, -1, 3,
		do_every, NULL, NULL,
		"Execute console cmd every N seconds"
		HELPSTR_NEWLINE "every 10 'gpio out 5 1'"
	   );
