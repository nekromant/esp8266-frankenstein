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


static int   do_baud(int argc, const char* const* argv)
{
	int port = atoi(argv[1]);
	int speed = atoi(argv[2]);
	if (port > 1) {
		console_printf("We only have UART0 and UART1, sorry\n");
		return -1;
	}
	
	console_printf("Setting UART%d speed to %d bps\n", port, speed);
	uart_init(port, speed);
	
	return 0;
}

CONSOLE_CMD(baud, 3, 3, 
	    do_baud, NULL, NULL, 
	    "Configure serial port speed" 
	    HELPSTR_NEWLINE "baudrate {port:0/1} {speed}"
	    HELPSTR_NEWLINE "baudrate 0 57600"
);
