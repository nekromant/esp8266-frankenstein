
#include <stdlib.h>
#include "console.h"
#include "user_interface.h"
#include "lwip/init.h"

#if TTDBG
int ttdbg[TTDBG] =
{
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
};
#endif

static int do_at (int argc, const char* const* argv)
{
	unsigned char stamacaddr[6];
	unsigned char apmacaddr[6];
	wifi_get_macaddr(STATION_IF, stamacaddr);
	wifi_get_macaddr(SOFTAP_IF, apmacaddr);
	console_printf("OK heap=%d sdk=%s stamac=" MACSTR " apmac=" MACSTR " chipid=0x%0x lwip=%x mss=%d tcpwnd=%d\n",
		system_get_free_heap_size(),
		system_get_sdk_version(),
		MAC2STR(stamacaddr),
		MAC2STR(apmacaddr),
		system_get_chip_id(),
		LWIP_VERSION, TCP_MSS, TCP_WND);

#if TTDBG
	int x, y = 1;
	while (argc >= y + 2)
	{
		x = atoi(argv[y]);
		if (x >= 0 && x < TTDBG)
			ttdbg[x] = atoi(argv[y + 1]);
		y += 2;
	}
	for (x = 0; x < TTDBG; x+=8)
	{
		for (y = x; y < x + 8; y++)
			console_printf("%02d:0x%08x ", y, ttdbg[y]);
		console_printf("\n");
	}
#endif
	return 0;
}

CONSOLE_CMD(AT, 1, 10, 
	    do_at, NULL, NULL, 
	    "says OK"
);
