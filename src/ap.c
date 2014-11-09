#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"


void softap_start()
{
        char sofap_mac[6] = {0x16, 0x34, 0x56, 0x78, 0x90, 0xab};
        char sta_mac[6] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab};
        struct ip_info info;

        wifi_set_macaddr(SOFTAP_IF, sofap_mac);
        wifi_set_macaddr(STATION_IF, sta_mac);

        IP4_ADDR(&info.ip, 192, 168, 3, 200);
        IP4_ADDR(&info.gw, 192, 168, 3, 1);
        IP4_ADDR(&info.netmask, 255, 255, 255, 0);
        wifi_set_ip_info(STATION_IF, &info);

        IP4_ADDR(&info.ip, 10, 10, 10, 1);
        IP4_ADDR(&info.gw, 10, 10, 10, 1);
        IP4_ADDR(&info.netmask, 255, 255, 255, 0);
        wifi_set_ip_info(SOFTAP_IF, &info);

	ets_uart_printf("Starting SoftAP\n");
}
