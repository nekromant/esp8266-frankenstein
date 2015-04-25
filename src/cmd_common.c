#include "user_interface.h"
#include "missing.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h" 
#include "microrl.h"
#include "console.h"
#include "main.h"
#include "helpers.h"
//#include "hostname.h"

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

uint32_t readvdd33(void);
static  int do_vdd(int argc, const char* const* argv)
{
	os_intr_lock();
	uint32_t vdd = readvdd33();
	os_intr_unlock();
	console_printf("VDD3V3 = %d mV\n", vdd);
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

static int do_rtcdump(int argc, const char* const* argv)
{
	/* To minimise stack we just query 16 bytes at a type */
	const int bytes = 768;
	const int words_per_row = 4;
	const int bytes_per_row = words_per_row * sizeof(uint32_t);
	uint32_t rtc_values[words_per_row];
	for (int row=0; row < bytes/bytes_per_row; row++) {
		system_rtc_mem_read(row * words_per_row, rtc_values, bytes_per_row);
		/* print address on start of line */
		console_printf("%04x ", row*bytes_per_row);
		for (int w=0; w < words_per_row; w++) {
			console_printf("%08x ", rtc_values[w]);
		}
		const char *chars = (const char*)&rtc_values[0];
		for (int c=0; c < bytes_per_row; c++) {
			char ch = chars[c];
			console_printf("%c", ch < 32 || ch > 95 ? '.' : ch);
		}
		console_printf("\n");
	}
	return 0;
}

CONSOLE_CMD(vdd, -1, -1, 
	    do_vdd, NULL, NULL, 
	    "Get VDD voltage"
);


CONSOLE_CMD(meminfo, -1, -1, 
	    do_meminfo, NULL, NULL, 
	    "Display memory information (serial line only)"
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

CONSOLE_CMD(rtcdump, -1, -1,
	    do_rtcdump, NULL, NULL,
	    "Dump content of RTC memory"
);

