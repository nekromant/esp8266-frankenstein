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



static void	hex_dump(unsigned long addr, char* data, int len)
{
	int				ii;
	int				theValue;
	int				lineCount;
	char			textString[16];
	char			asciiDump[24];
	unsigned long	myAddressPointer;

	lineCount			=	0;

	while (len >=0)
	{
		console_printf("%04X - ", addr);
		
		asciiDump[0]		=	0;
		for (ii=0; ii<16; ii++)
		{
			console_printf("%02X ", *data);

			if ((theValue >= 0x20) && (theValue < 0x7f))
			{
				asciiDump[ii % 16]	=	*data;
			}
			else
			{
				asciiDump[ii % 16]	=	'.';
			}
			
			data++;
			len--;
			addr++;
		}
		asciiDump[16]	=	0;
		console_printf("%s\n", asciiDump);
		lineCount++;
	}
}


static int do_wipe(int argc, const char*argv[])
{
	int sz = 512 * 1024;
	int i; 
	for (i=0; i<sz; i+=SPI_FLASH_SEC_SIZE) { 
		spi_flash_erase_sector( i / SPI_FLASH_SEC_SIZE);
		ets_uart_printf(".");
	}
	ets_uart_printf("We're dead now. Bye\n\n\n");
	os_delay_us(1000);
	system_restart();
}

CONSOLE_CMD(spi_wipe, 3, -1, 
	    do_wipe, NULL, NULL,
	    "Wipe the whole spi flash blank"
	    HELPSTR_NEWLINE "wipe");


//#define CHUNK 4
static int do_dump(int argc, const char*argv[])
{
	uint32_t tmp[16];
	uint32_t start = skip_atoi(&argv[1]);
	int len   = skip_atoi(&argv[2]);
	while(len > 0) { 
		spi_flash_read(start, tmp, 16);
		hex_dump(start, &tmp, 16);
		len-=16;
		start+=16;
		wdt_feed();
	}
			
}
CONSOLE_CMD(spi_dump, 3, -1, 
	    do_dump, NULL, NULL,
	    "Hexdump flash contents"
	    HELPSTR_NEWLINE "spi_dump start len");

/* wipe requires three random arguments. as a precaution */

