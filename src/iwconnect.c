#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "helpers.h"
#include "iwconnect.h"

static int conntimes = 0;
static /*volatile*/ os_timer_t conn_checker;
static void conn_checker_handler(void *arg)
{
	int state = wifi_station_get_connect_status();
	switch (state) {
	case STATION_CONNECT_FAIL:
		console_printf("Failed\n");
		goto bailout;		
	case STATION_NO_AP_FOUND:
		console_printf("NotFound\n");
		goto bailout;		
	case STATION_GOT_IP:
		console_printf("Connected\n");
		goto bailout;
	}

	conntimes++;
	if (conntimes > 20) {
		console_printf("Timeout, still trying\n");
		goto bailout;
	}

	return;
bailout:
	os_timer_disarm(&conn_checker);
	console_lock(0);
	return;
}

int exec_iwconnect(const char *ssid, const char *password)
{
	struct station_config sta_conf;
	os_strncpy((char*)sta_conf.ssid, ssid, sizeof sta_conf.ssid);

	sta_conf.password[0] = 0x0;
	if (password != NULL) {
		os_strncpy((char*)&sta_conf.password, password, 32);
  }

	wifi_station_set_config(&sta_conf);		
	wifi_station_disconnect();
	wifi_station_connect();

	os_timer_disarm(&conn_checker);
	os_timer_setfn(&conn_checker, (os_timer_func_t *)conn_checker_handler, NULL);
	os_timer_arm(&conn_checker, 300, 1);
	conntimes = 0;
	console_lock(1);

	return 0;
}

