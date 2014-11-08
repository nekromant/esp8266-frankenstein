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

static void ICACHE_FLASH_ATTR scan_done_cb(void *arg, STATUS status)
{
	scaninfo *c = arg; 
	struct bss_info *inf; 
	STAILQ_FOREACH(inf, c->pbss, next) {
		ets_uart_printf("BSSID %x:%x:%x:%x:%x:%x channel %.02d rssi %.02d auth %d %s\n", 
				MAC2STR(inf->bssid),
				inf->channel, 
				inf->rssi, 
				inf->authmode,
				inf->ssid
			);
		inf = (struct bss_info *) &inf->next;
	}
	console_lock(0);
}


static int do_scan(int argc, const char*argv[])
{
	wifi_set_opmode(STATION_MODE);
	wifi_station_scan(NULL, &scan_done_cb);
	console_lock(1); /* Lock till we've finished scanning */
}


const char *modes[] = {
	[0x0] = "NONE",
	[STATION_MODE] = "STA",
	[SOFTAP_MODE] = "AP",
	[STATIONAP_MODE] = "APSTA",
}; 

int str2mode(const char *str)
{
	int i; 
	for (i=0; i<=STATIONAP_MODE; i++) { 
		if (strcmp(str, modes[i])==0)
			return i;
	}
	return -1;
}
static int do_iwmode(int argc, const char*argv[])
{
	int mode, newmode; 
	mode = wifi_get_opmode();
	if (argc == 1) {
		ets_uart_printf("Wireless mode: %s", modes[mode]);
		return 0;
	} else {
		newmode = str2mode(argv[1]);
		if (-1 == newmode) { 
			ets_uart_printf("Invalid mode specified: %s\n", argv[1]);
			return 1;
		}
		ets_uart_printf("Wireless mode change: %s -> %s", modes[mode], modes[newmode]);
		wifi_set_opmode(newmode);
	}	
}

static int do_iwconnect(int argc, const char*argv[])
{
	int mode, newmode; 
	mode = wifi_get_opmode();
	if ((mode != STATION_MODE) || (mode != STATIONAP_MODE)) {
		ets_uart_printf("Cannot connect while in '%s' mode", modes[mode]);
		return 0;
	} 

	
}


static void print_ip_info(int iface)
{
        struct ip_info info;
	wifi_get_ip_info(STATION_IF, &info);
	
}

static int do_ifconfig(int argc, const char*argv[])
{
	print_ip_info(STATION_IF);
}


CONSOLE_CMD(iwscan, -1, -1, do_scan, NULL, 
	    "Scan for available stations");

CONSOLE_CMD(iwmode, -1, 2, do_iwmode, NULL, 
	    "Get/set wireless mode. Available modes: NONE, STA, AP, APSTA");
