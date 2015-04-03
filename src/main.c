
#include <stdlib.h>

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
#include "flash_layout.h"
#include <generic/macros.h>
#include <lwip/netif.h>
#include <lwip/app/dhcpserver.h>

#include "env.h"
#if defined(CONFIG_SERVICE_TELNET)
#include "svc_telnet.h"
#endif

#include "iwconnect.h"

struct envpair {
	char *key, *value;
};

struct envpair defaultenv[] = {
	{ "sta-mode",          "dhcp" },
	{ "default-mode",
/* ideally, this should somehow be tied into helpers.c, id_from_wireless_mode... */
#if defined(CONFIG_WIFI_MODE_AP)
                         "AP" },
#elif defined(CONFIG_WIFI_MODE_STATION)
                         "STA" },
#elif defined(CONFIG_WIFI_MODE_SOFTAP)
                         "APSTA" },
#else
                         "" },
#endif

	{ "sta-ip",            CONFIG_ENV_DEFAULT_STATION_IP },
	{ "sta-mask",          CONFIG_ENV_DEFAULT_STATION_MASK },
	{ "sta-gw",            CONFIG_ENV_DEFAULT_STATION_GW },
	{ "log-level",         "3", },
	{ "sta-auto",
#if defined(CONFIG_ENV_DEFAULT_STATION_AUTO_CONNECT)
	                       "1" },
	{ "sta-auto-ssid",     CONFIG_ENV_DEFAULT_STATION_AUTO_SSID },
	{ "sta-auto-password", CONFIG_ENV_DEFAULT_STATION_AUTO_PASSWORD },
#else
	                       "0" },
#endif


	{ "ap-ip",             CONFIG_ENV_DEFAULT_AP_IP },
	{ "ap-mask",           CONFIG_ENV_DEFAULT_AP_MASK },
	{ "ap-gw",             CONFIG_ENV_DEFAULT_AP_GW },

	{ "hostname",          CONFIG_ENV_DEFAULT_HOSTNAME },
	{ "bootdelay",         "5" },
#if defined(CONFIG_SERVICE_DHCPS)
	{ "dhcps-enable",      "1" },
#endif
#if defined(CONFIG_SERVICE_TELNET)
  /* Note: modules requiring their own config might be able to have a 'registration' struct that adds to here */
	{ "telnet-port",       "23" },
	{ "telnet-autostart",  "1" },
	{ "telnet-drop",       "60" },
#endif
#if defined(CONFIG_CMD_TFTP)
	{ "tftp-server",       CONFIG_ENV_DEFAULT_TFTP_SERVER_IP}, 
	{ "tftp-dir",          CONFIG_ENV_DEFAULT_TFTP_SERVER_DIR}, 
	{ "tftp-file",         CONFIG_ENV_DEFAULT_TFTP_SERVER_FILE},
#endif
};

void request_default_environment(void)
{
	int i;
	for (i=0; i<ARRAY_SIZE(defaultenv); i++)
		env_insert(defaultenv[i].key, defaultenv[i].value);
}

#if defined(CONFIG_ENABLE_BANNER)
void print_hello_banner(void)
{
	console_printf("\n\n\nFrankenstein ESP8266 Firmware\n");
	console_printf("Powered by Antares " CONFIG_VERSION_STRING "\n");	
	console_printf("(c) Andrew 'Necromant' Andrianov 2014 <andrew@ncrmnt.org>\n");	
	console_printf("This is free software (where possible), published under the terms of GPLv2\n");	
	console_printf("\nMemory Layout:\n");	
	system_print_meminfo();
	system_set_os_print(0);
	console_printf("\n Flash layout:\n"
		       "Firmware size is     %d KiB (0x%x)\n"
		       "Filesystem starts at %d KiB (0x%x)\n",
		       fr_get_firmware_size()/1024, fr_get_firmware_size(),
		       fr_fs_flash_offset()/1024,   fr_fs_flash_offset());
	console_printf("\nAvailable services:\n");
  int have_features = 0;
#if defined(CONFIG_SERVICE_DHCPS)
	console_printf("DHCP server\n"); have_features ++;
#endif
#if defined(CONFIG_SERVICE_TELNET)
	console_printf("Telnet server\n"); have_features ++;
#endif
  if (have_features == 0) { console_printf("None.\n"); }
}
#endif

void network_init()
{
	struct ip_info info;
	wifi_set_opmode(id_from_wireless_mode(env_get("default-mode")));
	wifi_get_ip_info(STATION_IF, &info);
	const char *dhcp = env_get("sta-mode"); 
	const char *ip, *mask, *gw;
	if (!dhcp || strcmp(dhcp, "dhcp") != 0)
	{
		ip = env_get("sta-ip"); 
		mask = env_get("sta-mask");
		gw = env_get("sta-gw");
		if (ip)
			info.ip.addr = ipaddr_addr(ip);
		if (mask)
			info.netmask.addr = ipaddr_addr(mask);
		if (gw)
			info.gw.addr = ipaddr_addr(gw);
		
		wifi_set_ip_info(STATION_IF, &info);
	}

	wifi_get_ip_info(SOFTAP_IF, &info);
	ip = env_get("ap-ip"); 
	mask = env_get("ap-mask");
	gw = env_get("ap-gw");
	if (ip)
		info.ip.addr = ipaddr_addr(ip);
	if (mask)
		info.netmask.addr = ipaddr_addr(mask);
	if (gw)
		info.gw.addr = ipaddr_addr(gw);
	
	if (wifi_get_opmode() != STATION_MODE)
		wifi_set_ip_info(SOFTAP_IF, &info);

#if defined(CONFIG_SERVICE_DHCPS)
	const char *dhcps = env_get("dhcps-enable"); 
	if (dhcps && (*dhcps == '1')) {
		dhcps_start(&info);
		console_printf("dhcpserver: started\n");
	} else
		console_printf("dhcpserver: disabled\n");
#endif
}

#include <stdio.h>


const char* fr_request_hostname(void) {

	return env_get("hostname");
}

/* By experimentation we discovered that certain things cant be done from inside user_init... */
static void main_init_done(void)
{
	if (wifi_get_opmode() == STATION_MODE) {
	  const char *sta_auto = env_get("sta-auto");
	  if (sta_auto && atoi(sta_auto)) {
	    const char *ssid = env_get("sta-auto-ssid");
	    const char *pass = env_get("sta-auto-password");
	    if (ssid) {
	      console_printf("STA mode: automatically attempting to connect to %s\n", ssid);
	      exec_iwconnect(ssid, pass);
	    }
	  }
	}
}

void user_init()
{
	uart_init(0, 115200);
#if defined(CONFIG_ENABLE_SECOND_UART)
	uart_init(1, 115200);
#endif
	uart_init_io();

	env_init(CONFIG_ENV_OFFSET, CONFIG_ENV_LEN);
#if defined(CONFIG_ENABLE_BANNER)
	print_hello_banner();
#endif
	network_init();

#if defined(CONFIG_SERVICE_TELNET)
	const char *enabled = env_get("telnet-autostart"); 
	if (enabled && (*enabled=='1'))
		telnet_start(-1); // use env or 23
#endif
	console_init(32);

	const char* loglevel = env_get("log-level");
	if (loglevel) { set_log_level(atoi(loglevel)); }
	
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, BIT2, BIT2, 0);
	gpio_output_set(0, BIT0, BIT0, 0);

	system_init_done_cb(main_init_done);
}
