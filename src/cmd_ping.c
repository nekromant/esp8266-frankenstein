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

//TODO: cmd_ping.c:(.irom0.text+0x2d): undefined reference to `ping_start'

static void ping_recv_callback(void* arg, void *pdata)
{
	struct ping_resp *pingresp = pdata;

	if(pingresp->seqno == 3 /*LAST PING PACKET*/){
		console_printf("total %d, lost %d, %d bytes, %d ms (%d)\n" , 
			pingresp->total_count, pingresp->timeout_count, pingresp->total_bytes, pingresp->total_time, pingresp->ping_err);
		console_lock(0);
	} else {
		console_printf("recv %d bytes in %d ms, seq %d (%d)\n" , pingresp->bytes, pingresp->resp_time, pingresp->seqno, pingresp->ping_err);
	}
}

static int do_ping(int argc, const char* const* argv)
{
	struct ping_option *pingopts = os_zalloc(sizeof(struct ping_option));
	ip_addr_t ipaddr;
	ipaddr.addr = ipaddr_addr(argv[1]);

	pingopts->ip = ipaddr.addr;
	pingopts->count = 3;
	pingopts->recv_function=ping_recv_callback;
	pingopts->sent_function=NULL;
	ping_start(pingopts);
	console_lock(1);
	return 0;
}

CONSOLE_CMD(ping, 2, 2, 
	do_ping, NULL, NULL, 
	"Send icmp ping to specified address"
	HELPSTR_NEWLINE "ping 8.8.8.8"
);
