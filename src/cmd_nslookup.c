#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "microrl.h"
#include "console.h"

#include <stdlib.h>
#include <generic/macros.h>

static void nslookup_callback(const char *name, ip_addr_t *ipaddr, void *arg) 
{
	if (ipaddr==NULL) {
		console_printf("nslookup failed\n");
	}else{
		console_printf("%d.%d.%d.%d\n" , IP2STR(&ipaddr->addr));
	}
	console_lock(0);
}

static int do_nslookup(int argc, const char* const* argv)
{
	struct espconn pespconn;
	ip_addr_t ipaddr;
	espconn_gethostbyname(&pespconn, argv[1], &ipaddr, nslookup_callback);
	console_lock(1);
	return 0;
}

CONSOLE_CMD(nslookup, 2, 2, 
	do_nslookup, NULL, NULL, 
	"Get IP address via hostname."
	HELPSTR_NEWLINE "nslookup google.com"
);
