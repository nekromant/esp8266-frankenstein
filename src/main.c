
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
#include <generic/macros.h>
#include <lwip/netif.h>
#include <lwip/app/dhcpserver.h>

#include "env.h"
#include "telnet.h"

struct envpair {
	char *key, *value;
};

struct envpair defaultenv[] = {
	{ "sta-mode",          "dhcp" },
	{ "default-mode",      "STA" },
	{ "sta-ip",            "192.168.0.123" },
	{ "sta-mask",          "255.255.255.0" },
	{ "sta-gw",            "192.168.0.1" },

	{ "ap-ip",             "192.168.1.1" },
	{ "ap-mask",           "255.255.255.0" },
	{ "ap-gw",             "192.168.1.1" },

	{ "hostname",          "frankenstein" },
	{ "bootdelay",         "5" },
	{ "dhcps-enable",      "1" },
	{ "telnet-port",       "23" },
	{ "telnet-autostart",  "1" },
	{ "telnet-drop",       "60" },
	{ "tftp-server",       "192.168.1.215"}, 
	{ "tftp-dir",          "/"}, 
	{ "tftp-file",         "antares.rom"}    
};

void request_default_environment(void)
{
	int i;
	for (i=0; i<ARRAY_SIZE(defaultenv); i++)
		env_insert(defaultenv[i].key, defaultenv[i].value);
}

void print_hello_banner(void)
{
	console_printf("\n\n\nFrankenstein ESP8266 Firmware\n");
	console_printf("Powered by Antares " CONFIG_VERSION_STRING "\n");	
	console_printf("(c) Andrew 'Necromant' Andrianov 2014 <andrew@ncrmnt.org>\n");	
	console_printf("This is free software (where possible), published under the terms of GPLv2\n");	
	console_printf("\nMemory Layout:\n");	
	system_print_meminfo();
	system_set_os_print(0);

}


void network_init()
{
	struct ip_info info;

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

	const char *dhcps = env_get("dhcps-enable"); 
	if (dhcps && (*dhcps == '1')) {
		dhcps_start(&info);
		console_printf("dhcpserver: started\n");
	} else
		console_printf("dhcpserver: disabled\n");
}

#include <stdio.h>


const char* fr_request_hostname(void) {

	return env_get("hostname");
}

void user_init()
{
	uart_init(0, 115200);
	uart_init(1, 115200);
	uart_init_io();

	env_init(CONFIG_ENV_OFFSET, CONFIG_ENV_LEN);
	print_hello_banner();

	network_init();

	const char *enabled = env_get("telnet-autostart"); 
	if (enabled && (*enabled=='1')) { 
		int port = 23; 
		const char *tmp = env_get("telnet-port");
		if (tmp)
			port = atoi(tmp);
		telnet_start(port);
	}

	console_init(32);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, BIT2, BIT2, 0);
	gpio_output_set(0, BIT0, BIT0, 0);

}
