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
#include <generic/macros.h>


static int do_wipe(int argc, const char*argv[])
{
	int sz = 512 * 1024;
	int i; 
	for (i=0; i<sz; i+=SPI_FLASH_SEC_SIZE) { 
		spi_flash_erase_sector( i / SPI_FLASH_SEC_SIZE);
		ets_uart_printf(".");
	}
	ets_uart_printf("We're dead now. Bye");
	system_restart();
}

CONSOLE_CMD(spi_wipe, 3, -1, 
	    do_wipe, NULL, NULL,
	    "Wipe the whole spi flash blank"
	    HELPSTR_NEWLINE "wipe");
