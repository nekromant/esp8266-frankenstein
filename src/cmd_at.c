
#include <stdlib.h>
#include "console.h"
#include "user_interface.h"

static int do_at (int argc, const char* const* argv)
{
        unsigned char macaddr[6];
        wifi_get_macaddr(SOFTAP_IF, macaddr);
	console_printf("OK sdk=%s chipid=0x%x mac=" MACSTR "\n", system_get_sdk_version(), system_get_chip_id(), MAC2STR(macaddr));
	return 0;
}

CONSOLE_CMD(AT, 1, 10, 
	    do_at, NULL, NULL, 
	    "says OK"
);
