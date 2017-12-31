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
#include <sensorlogger.h>

struct pinger_instance {
	struct ping_option *pingopts;
	struct slogger_data_type data_type;
	uint32_t current_ms;
};

static void ping_recv_callback(void* arg, void *pdata)
{
	struct ping_resp *pingresp = pdata;

	struct pinger_instance *inst = arg;
	inst->current_ms = pingresp->total_time;

	if(pingresp->seqno == 3 /*LAST PING PACKET*/){
		console_printf("total %d, lost %d, %d bytes, %d ms (%d)\n" ,
			pingresp->total_count, pingresp->timeout_count, pingresp->total_bytes, , pingresp->ping_err);
		console_lock(0);
	} else {
		console_printf("recv %d bytes in %d ms, seq %d (%d)\n" , pingresp->bytes, pingresp->resp_time, pingresp->seqno, pingresp->ping_err);
	}
}


static void dns_callback(const char * hostname, ip_addr_t * addr, void * arg)
{

	pingopts->ip = addr->.addr;
	pingopts->count = 1;
	pingopts->recv_function=ping_recv_callback;
	pingopts->sent_function=NULL;
	ping_start(pingopts);
}

static void start_ping_by_hostname(const char *hostname, void *arg)
{
	ip_addr_t addr;
	err_t error = espconn_gethostbyname(NULL, // It seems we don't need a real espconn pointer here.
										hostname, &addr, dns_callback);

	if (error == ESPCONN_INPROGRESS) {
		console_printf("DNS pending\n");
	}
	else if (error == ESPCONN_OK) {
		// Already in the local names table (or hostname was an IP address), execute the callback ourselves.
		dns_callback(hostname, &addr, req);
	}
	else {
		if (error == ESPCONN_ARG) {
			console_printf("DNS arg error %s\n", hostname);
		}
		else {
			console_printf("DNS error code %d\n", error);
		}
	}
}




static int do_ping(int argc, const char* const* argv)
{
	struct pinger_instance *pingopts = os_zalloc(sizeof(struct ping_option));
	ip_addr_t ipaddr;
	console_lock(1);
	return 0;
}
