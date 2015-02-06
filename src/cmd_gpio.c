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


static int  do_gpio(int argc, const char* const* argv)
{
	int gpio = atoi(argv[2]);
	
	if (strcmp(argv[1], "in") == 0) {
		GPIO_DIS_OUTPUT(gpio);
		console_printf("GP%d==%d\n", gpio, GPIO_INPUT_GET(gpio));
	} else 	if (strcmp(argv[1], "out") == 0) {
		if (argc < 4)
			return -1;
		int v = atoi(argv[3]);
		GPIO_OUTPUT_SET(gpio, v);
	}
	return 0;
}

CONSOLE_CMD(gpio, 3, 4, 
	    do_gpio, NULL, NULL, 
	    "Control gpio lines. gpio mode line [value] "
	    HELPSTR_NEWLINE "gpio in 0"
	    HELPSTR_NEWLINE "gpio out 0 1"
);
