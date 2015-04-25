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

static const char *modes[] = {
	[0x0] = "NONE",
	[STATION_MODE] = "STA",
	[SOFTAP_MODE] = "AP",
	[STATIONAP_MODE] = "APSTA",
}; 

static const char *ciphers[] = {
	[AUTH_OPEN]         = "OPEN",
	[AUTH_WEP]          = "WEP",
	[AUTH_WPA_PSK]      = "WPA_PSK",
	[AUTH_WPA2_PSK]     = "WPA2_PSK",
	[AUTH_WPA_WPA2_PSK] = "WPA_WPA2_PSK",
};

static const char *ifacename[] = {
	[STATION_IF] = "sta0",
	[SOFTAP_IF] = "ap0",
};

static const char *ifacedsc[] = {
	[STATION_IF] = "WiFi Client Interface",
	[SOFTAP_IF] = "WiFi Access Point Interface",
};


static const char *sta_states[] = {
	[STATION_IDLE]              = "Idle",
	[STATION_CONNECTING]        = "Connecting",
	[STATION_WRONG_PASSWORD]    = "Wrong Password",
	[STATION_NO_AP_FOUND]       = "No AP Found",
	[STATION_CONNECT_FAIL]      = "Connection Failed",
	[STATION_GOT_IP]            = "Connected",
};


int  lookup_index(const char* key, const char **tbl, int count)
{
	int i; 
	for (i=0; i<count; i++) {
		if (strcmp(key, tbl[i])==0)
			return i;	
	}
	return -1;
}

const char  *lookup_string(int key, const char **tbl, int count)
{
	if (key >= count)
		return NULL;
	return tbl[key];
}


#define DECLARE_LOOKUP(_table, _func)					\
	const char  *id_to_ ## _func(int id)		\
	{								\
		return lookup_string(id, _table, ARRAY_SIZE(_table));	\
	}								\
	int  id_from_ ## _func(const char *id)		\
	{								\
		return lookup_index(id, _table, ARRAY_SIZE(_table));	\
	}								\
	

DECLARE_LOOKUP(modes, wireless_mode);
DECLARE_LOOKUP(ciphers, encryption_mode);
DECLARE_LOOKUP(ifacename, iface_name);
DECLARE_LOOKUP(ifacedsc, iface_description);
DECLARE_LOOKUP(sta_states, sta_state);

#define is_digit(c)	((c) >= '0' && (c) <= '9')

unsigned long  skip_atoul(const char **s)
{
	unsigned long i = 0;

	while (is_digit(**s))
		i = i * 10 + *((*s)++) - '0';
	
	return i;
}
