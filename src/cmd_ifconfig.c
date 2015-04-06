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
#include "helpers.h"

#include <stdlib.h>
#include <stdlib.h>
#include <generic/macros.h>

#include "missing.h"

static void print_ip_info_real(int iface, int rssi, int channel)
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
	if (iface == STATION_IF) {
		console_printf("     state   : %s\n", id_to_sta_state(wifi_station_get_connect_status()));
		console_printf("     rssi    : %d\n", rssi);
		console_printf("     channel : %d\n", channel);
	} else { /* AP */
		int state = wifi_get_opmode();
		console_printf("     state: %s\n", 
			       ((state == SOFTAP_MODE) || (state == STATIONAP_MODE)) ? 
			       "Running" : "Stopped");
	}
	console_printf("     inet addr:" IPSTR " Mask:" IPSTR " Gateway:" IPSTR " \n", 
			IP2STR(&info.ip), IP2STR(&info.netmask), IP2STR(&info.gw));
}

static void  scan_done_cb(void *arg, STATUS status)
{
	scaninfo *c = arg; 
	struct bss_info *inf; 
	STAILQ_FOREACH(inf, c->pbss, next) {
		print_ip_info_real(STATION_IF, inf->rssi, inf->channel); 
		inf = (struct bss_info *) &inf->next;
	}
	console_lock(0);	
}

struct scan_config conf;
struct station_config sconf;
static void print_ip_info(int iface)
{
	if (iface == SOFTAP_IF) {
		print_ip_info_real(iface, 0, 0);
		return;
	}
	
	if (!wifi_get_opmode() || (wifi_get_opmode() == SOFTAP_MODE))
		return;
	
	
	int state = wifi_station_get_connect_status();
	if ((iface != STATION_IF) || 
	    (state != STATION_GOT_IP)) {
		print_ip_info_real(iface, 0, 0);
		return;
	}

	memset(&conf, 0x0, sizeof(conf)); 
	memset(&sconf, 0x0, sizeof(sconf));
	if (wifi_station_get_config(&sconf)) {
		conf.ssid = sconf.ssid;
		wifi_station_scan(&conf, &scan_done_cb);
		console_lock(1); /* Lock till we've finished scanning */
		return;
	} else 
		print_ip_info_real(iface, 0, 0);
}

static int do_ifconfig(int argc, const char* const* argv)
{
	if (argc==1) {
		print_ip_info_real(STATION_IF, 0, 0);
		print_ip_info_real(SOFTAP_IF, 0, 0);		
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

	return 0;
}

CONSOLE_CMD(ifconfig, -1, 5, 
	    do_ifconfig, NULL, NULL, 
	    "Show network interfaces and their status"
	    HELPSTR_NEWLINE "ifconfig [iface]"
	    HELPSTR_NEWLINE "ifconfig sta0"
);

