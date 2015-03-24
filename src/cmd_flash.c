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


static void 	hex_dump(uint32 addr, const char* data, int len)
{
	int				ii;
	char			asciiDump[24];

	while (len >0)
	{
		console_printf("%04X - ", addr);
		
		asciiDump[0]		=	0;
		for (ii=0; ii<16; ii++)
		{
			console_printf("%02X ", *data);

			if ((*data >= 0x20) && (*data < 0x7f))
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
	}
}



extern void ets_wdt_enable(void);
extern void ets_wdt_disable(void);

static  __attribute__ ((section(".iram0.text"))) int do_wipe(int argc, const char* const* argv)
{
	int sz = 512 * 1024;
	int i; 
	ets_wdt_disable();
	for (i=0; i < (sz / SPI_FLASH_SEC_SIZE); i++) { 
		spi_flash_erase_sector( i );
		if (0 ==(i % 16)) 
			console_printf(".");
	}
	console_printf("\nSuicide complete (%d records), rebooting\n", i);
	system_restart();
	return 0;
}

CONSOLE_CMD(spi_wipe, 3, -1, 
	    do_wipe, NULL, NULL,
	    "Wipe the whole spi flash blank"
	    HELPSTR_NEWLINE "wipe any three arguments");

static  int do_wipeparams(int argc, const char* const* argv)
{
	console_printf("\nWiping configuration area: ");

	spi_flash_erase_sector(CONFIG_ADDR_BLOBSETTING1 / SPI_FLASH_SEC_SIZE);	
	console_printf(" %p ", (void*)(CONFIG_ADDR_BLOBSETTING1 / SPI_FLASH_SEC_SIZE));

	spi_flash_erase_sector(CONFIG_ADDR_BLOBSETTING2 / SPI_FLASH_SEC_SIZE);	
	console_printf(" %p ", (void*)(CONFIG_ADDR_BLOBSETTING2 / SPI_FLASH_SEC_SIZE));

	console_printf("\nDone, rebooting\n\n\n\n\n\n\n");

	system_restart();
	
	return 0;
}

CONSOLE_CMD(wipeparams, 3, -1, 
	    do_wipeparams, NULL, NULL,
	    "Wipe blob configuration section of flash"
	    HELPSTR_NEWLINE "wipeparams any three arguments");

static int  do_dump(int argc, const char* const* argv)
{
	uint32 tmp[16];
	uint32 start = atoi(argv[1]);
	int len   = atoi(argv[2]);
	while(len > 0) { 
		spi_flash_read(start, tmp, 16);
		hex_dump(start, (char*)tmp, 16);
		len-=16;
		start+=16;
		wdt_feed();
	}	
	return 0;		
}

CONSOLE_CMD(spi_dump, 3, -1, 
	    do_dump, NULL, NULL,
	    "Hexdump flash contents"
	    HELPSTR_NEWLINE "spi_dump start len");




static int  do_scan(int argc, const char* const* argv)
{
	ets_wdt_disable();
	uint32_t off = 256 * 1024; 
	char tmp[4096];
	int i; 
	int altered; 
	while(off <= 512 * 1024 - 4096) { 
		altered = 0;
		spi_flash_read(off, (uint32*)tmp, 4096);
		for (i=0; i<4096; i++)
			if (tmp[i]!=0xff) 
				altered++;

		if (altered) {
			console_printf("Sector %d (0x%x) has been tampered by blobs\n",
				       off / 4096, off);
		}
		off += 4096;
	}			
	return 0;
}

CONSOLE_CMD(flash_scan, -1, -1, 
	    do_scan, NULL, NULL,
	    "Scan the upper portion of FLASH for dirty blocks"
	    HELPSTR_NEWLINE "Used to find out where the blobs store config"
	    HELPSTR_NEWLINE "flash_scan");
