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


#define is_digit(c)	((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s)
{
	int i = 0;

	while (is_digit(**s))
		i = i * 10 + *((*s)++) - '0';
	
	return i;
}


/*
static int do_iwconnect(int argc, const char*argv[])
{
	int mode, newmode; 
	mode = wifi_get_opmode();
	if ((mode != STATION_MODE) || (mode != STATIONAP_MODE)) {
		ets_uart_printf("Cannot connect while in '%s' mode", modes[mode]);
		return 0;
	} 

	
}
*/



static void print_ip_info(int iface)
{
        struct ip_info info;
	wifi_get_ip_info(iface, &info);
	console_printf("%s: %s\n", id_to_iface_name(iface), id_to_iface_description(iface));
	console_printf("    inet addr:" IPSTR " Mask:" IPSTR " Gateway:" IPSTR " \n", 
			IP2STR(&info.ip), IP2STR(&info.netmask), IP2STR(&info.gw));
}



static int do_ifconfig(int argc, const char* argv[])
{
	if (argc==1) {
		print_ip_info(STATION_IF);
		print_ip_info(SOFTAP_IF);
		return 0;
	}

	int iface = id_from_iface_name(argv[1]);
	if (iface == -1) { 
		console_printf("No such iface: %s\n", argv[1]);
	}
	
	if (argc==2) {
		print_ip_info(iface);
		return 0;
	}


	/* We can't alter these dynamically. WTF??? */
	/* Store to env */

	struct ip_info info;
	wifi_get_ip_info(iface, &info);
	uint32_t temp; 
	if (argc>2) /* IP */ 
		info.ip.addr = ipaddr_addr(argv[2]);

	if (argc>3) /* Netmask */ 
		info.ip.addr = ipaddr_addr(argv[3]);

	if (argc>4) /* Gateway */ 
		info.ip.addr = ipaddr_addr(argv[4]);

	if (!wifi_set_ip_info(iface, &info))
	{
		console_printf("Setting new IP info FAILED ;(\n");
	}

	return 0;
}

CONSOLE_CMD(ifconfig, -1, 5, do_ifconfig, NULL, 
	    "Show/setup network interfaces"
	    HELPSTR_NEWLINE "ifconfig [iface] [ipaddr] [netmask] [gateway]"
	    HELPSTR_NEWLINE "ifconfig sta0 192.168.0.1 255.255.255.0 192.168.0.8"
);

