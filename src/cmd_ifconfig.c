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


static void print_ip_info(int iface)
{
        struct ip_info info;
	wifi_get_ip_info(iface, &info);

	if (wifi_get_opmode() == 0x0)
		return;

	if ((iface == STATION_IF) && (wifi_get_opmode() == SOFTAP_MODE))
		return;

	if ((iface == SOFTAP_IF) && (wifi_get_opmode() == STATION_MODE))
		return;

	console_printf("%s: %s\n", id_to_iface_name(iface), id_to_iface_description(iface));
	if (iface == STATION_IF)
		console_printf("     state: %s\n", id_to_sta_state(wifi_station_get_connect_status()));
	else { /* AP */
		int state = wifi_get_opmode();
		console_printf("     state: %s\n", 
			       ((state == SOFTAP_MODE) || (state == STATIONAP_MODE)) ? 
			       "Running" : "Stopped");
	}
	console_printf("     inet addr:" IPSTR " Mask:" IPSTR " Gateway:" IPSTR " \n", 
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

CONSOLE_CMD(ifconfig, -1, 5, 
	    do_ifconfig, NULL, NULL, 
	    "Show/setup network interfaces"
	    HELPSTR_NEWLINE "ifconfig [iface] [ipaddr] [netmask] [gateway]"
	    HELPSTR_NEWLINE "ifconfig sta0 192.168.0.1 255.255.255.0 192.168.0.8"
);

