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

extern int ets_uart_printf(const char *fmt, ...);


struct envpair {
	char *key, *value;
};

struct envpair defaultenv[] = {
	{ "sta-mode", "dhcp" },
	{ "sta-ip", "192.168.0.123" },
	{ "sta-mask", "255.255.255.0" },
	{ "sta-gw", "192.168.0.1" },

	{ "ap-ip", "192.168.1.1" },
	{ "ap-mask", "255.255.255.0" },
	{ "ap-gw", "192.168.1.1" },

	{ "bootdelay", "5" },
};

void request_default_environment()
{
	int i;
	for (i=0; i<ARRAY_SIZE(defaultenv); i++)
		env_insert(defaultenv[i].key, defaultenv[i].value);
}

void print_hello_banner()
{
	console_printf("\n\n\nFrankenstein ESP8266 Firmware\n");
	console_printf("Powered by Antares " CONFIG_VERSION_STRING "\n");	
	console_printf("(c) Andrew 'Necromant' Andrianov 2014 <andrew@ncrmnt.org>\n");	
	console_printf("This is free software (where possible), published under the terms of GPLv2\n");	
	console_printf("\nMemory Layout:\n");	
	system_print_meminfo();
	system_set_os_print(0);

}


extern void env_init(uint32_t flashaddr, uint32_t envsize);

void ICACHE_FLASH_ATTR
network_init()
{
	struct ip_info info;
	wifi_get_ip_info(STATION_IF, &info);

	char *dhcp = env_get("sta-mode"); 
	
	if (dhcp && strcmp(dhcp, "dhcp") == 0)
		return;

	char *ip = env_get("sta-ip"); 
	char *mask = env_get("sta-mask");
	char *gw = env_get("sta-gw");
	if (ip)
		info.ip.addr = ipaddr_addr(ip);
	if (mask)
		info.netmask.addr = ipaddr_addr(mask);
	if (gw)
		info.gw.addr = ipaddr_addr(gw);

	wifi_set_ip_info(STATION_IF, &info);


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

	wifi_set_ip_info(SOFTAP_IF, &info);


}

void ICACHE_FLASH_ATTR
user_init()
{
	/* Reset system timer */
	/* Configure GPIO for our blinky */
//	wifi_status_led_install(0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);

	/* Reconfigure the timer to call our blinky once per second */
//	os_timer_disarm(&some_timer);
//	os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);
//	os_timer_arm(&some_timer, 1000, 1);
//	os_delay_us(1000);
	uart_init(115200, 115200);
	/* TODO: Kconfig */
	print_hello_banner();
	env_init((512 - 16) * 1024, 4*1024);
	network_init();
	console_init(32);


	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, BIT2, BIT2, 0);
	gpio_output_set(0, BIT0, BIT0, 0);

}
