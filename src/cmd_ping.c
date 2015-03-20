#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "microrl.h"
#include "console.h"
#include "ping.h"

#include <stdlib.h>
#include <generic/macros.h>


static void ping_recv_callback(void* arg, void *pdata)
{
	struct ping_resp *pingresp = pdata;
	console_printf("recv %d %d %d %d %d %d %d %d\n" , 
		&pingresp->total_count, &pingresp->resp_time, &pingresp->seqno, &pingresp->timeout_count, 
		&pingresp->bytes, &pingresp->total_bytes, &pingresp->total_time, &pingresp->ping_err);
}

TODO: undefined reference to `ping_start'

static int do_ping(int argc, const char* const* argv)
{
	struct ping_option *pingopts;
	ip_addr_t ipaddr;
	IP4_ADDR(&ipaddr, 8, 8, 8, 8);
	pingopts->ip = ipaddr.addr;
	pingopts->count = 3;
	pingopts->recv_function=ping_recv_callback;
	pingopts->sent_function=NULL;
	ping_start(pingopts);
	return 0;
}

CONSOLE_CMD(ping, 2, 2, 
	do_ping, NULL, NULL, 
	"Send icmp ping to specified address"
	HELPSTR_NEWLINE "ping 8.8.8.8"
);
