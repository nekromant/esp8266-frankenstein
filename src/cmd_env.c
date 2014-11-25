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



static int   do_printenv(int argc, const char* argv[])
{
	env_dump();
}

static int   do_reset(int argc, const char* argv[])
{
	console_printf("Reverting to default env..");
	env_reset();
	console_printf(".DONE!\n");
}
static int   do_setenv(int argc, const char* argv[])
{
	switch (argc)
	{
	case 1:
		env_dump();
		break;
	case 2:
		env_delete(argv[1]);
		break;
	case 3:
		env_insert(argv[1], argv[2]);
		break;
	}
}

static int do_getenv(int argc, const char* argv[])
{
	char *v = env_get(argv[1]);
	if (v)
		console_printf(v);
	else
		console_printf("\n");
}

static int   do_saveenv(int argc, const char* argv[])
{
	console_printf("Writing environment to flash...");
	env_save();
	console_printf("DONE\n");
}

CONSOLE_CMD(printenv, -1, -1, 
	    do_printenv, NULL, NULL, 
	    "Print all environment variables"
);

CONSOLE_CMD(setenv, -1, -1, 
	    do_setenv, NULL, NULL, 
	    "Set an environment variable" 
	    HELPSTR_NEWLINE "setenv var value"
);

CONSOLE_CMD(getenv, -1, -1, 
	    do_getenv, NULL, NULL, 
	    "Get an environment variable" 
	    HELPSTR_NEWLINE "setenv var value"
);

CONSOLE_CMD(saveenv, -1, -1, 
	    do_saveenv, NULL, NULL, 
	    "Write environment to flash" 
	    HELPSTR_NEWLINE "setenv var value"
);

CONSOLE_CMD(envreset, -1, -1, 
	    do_reset, NULL, NULL, 
	    "Reset environment to defaults" 
	    HELPSTR_NEWLINE "resetenv"
);
