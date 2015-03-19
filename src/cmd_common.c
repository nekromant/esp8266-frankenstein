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
#include "main.h"
#include "helpers.h"
#include "hostname.h"

#include <stdlib.h>
#include <stdlib.h>
#include <generic/macros.h>




static  int do_meminfo(int argc, const char* const* argv)
{
	system_set_os_print(1);
	system_print_meminfo();
	system_set_os_print(0);
	return 0;
}

static  int do_chipinfo(int argc, const char* const* argv)
{
	console_printf("id=0x%x\n", system_get_chip_id());
	return 0;
}

static  int do_reboot(int argc, const char* const* argv)
{
	system_restart();
	return 0;
}

static  int do_argtest(int argc, const char* const* argv)
{
	int i;
	console_printf("argc == %d\n", argc); 
	for (i=0; i<argc; i++) 
	{
		console_printf("argv[%d] == %s\n", i, argv[i]);
	}
	return 0;
}

static  int do_deepsleep(int argc, const char* const* argv)
{
	const char *tmp = argv[1];
	unsigned long n = skip_atoul(&tmp); 
	console_printf("Deep sleep mode for %lu microseconds\n", n);
	system_deep_sleep(n);
	return 0;
}

static  int do_hname(int argc, const char* const* argv)
{
	set_dhcp_hostname("aura");
	return 0;
}


CONSOLE_CMD(hname, -1, -1, 
	    do_hname, NULL, NULL, 
	    "Set dhcp hostname"
);


CONSOLE_CMD(meminfo, -1, -1, 
	    do_meminfo, NULL, NULL, 
	    "Display memory information"
);

CONSOLE_CMD(chipinfo, -1, -1,
	    do_chipinfo, NULL, NULL,
	    "Display chip information"
);

CONSOLE_CMD(reset, -1, -1, 
	    do_reboot, NULL, NULL, 
	    "Soft-reboot the device "
);

CONSOLE_CMD(deepsleep, 2, 2, 
	    do_deepsleep, NULL, NULL, 
	    "Enter deep sleep for some microseconds"
	    HELPSTR_NEWLINE "deepsleep 10000"
);

CONSOLE_CMD(argtest, -1, -1, 
	    do_argtest, NULL, NULL, 
	    "Print out argc/argv"
);
