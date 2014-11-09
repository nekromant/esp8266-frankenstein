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

#include <stdlib.h>
#include <stdlib.h>
#include <generic/macros.h>





static int do_version(int argc, const char* argv[])
{
	console_print_verinfo();
	return 0;
}

static int do_meminfo(int argc, const char* argv[])
{
	system_set_os_print(1);
	system_print_meminfo();
	system_set_os_print(0);
	return 0;
}

static int do_reboot(int argc, const char* argv[])
{
	system_restart();
}

CONSOLE_CMD(version, -1, -1, do_version, NULL, 
	    "Display version information and copyright"
);

CONSOLE_CMD(meminfo, -1, -1, do_meminfo, NULL, 
	    "Display memory information"
);

CONSOLE_CMD(reset, -1, -1, do_reboot, NULL, 
	    "Soft-reboot the device "
);
