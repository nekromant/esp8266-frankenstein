
#include <stdlib.h>
#include "console.h"

static int do_at (int argc, const char* argv[])
{
	console_printf("OK\n");
}

CONSOLE_CMD(at, 1, 10, 
	    do_at, NULL, NULL, 
	    "says OK"
);
