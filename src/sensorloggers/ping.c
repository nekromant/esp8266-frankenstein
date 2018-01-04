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
#include <main.h>

struct pinger_instance {
	struct ping_option pingopts;
	struct slogger_data_type data_type;
	char *target;
	uint32_t current_ms;
	ETSTimer periodic;
	int pending;
};

static void ping_recv_callback(void* arg, void *pdata)
{
	struct ping_resp *pingresp = pdata;

	struct pinger_instance *inst = arg;
	inst->current_ms = pingresp->total_time;
	console_printf("ping %d ms\n", inst->current_ms);
	inst->pending = 0;
//	if(pingresp->seqno == 3 /*LAST PING PACKET*/){
//		console_printf("total %d, lost %d, %d bytes, %d ms (%d)\n" ,
//			pingresp->total_count, pingresp->timeout_count, pingresp->total_bytes, , pingresp->ping_err);
//		console_lock(0);
//	} else {
//		console_printf("recv %d bytes in %d ms, seq %d (%d)\n" , pingresp->bytes, pingresp->resp_time, pingresp->seqno, pingresp->ping_err);
//	}
}


static void dns_callback(const char * hostname, ip_addr_t * addr, void * arg)
{
	struct pinger_instance * i = arg;
	i->pingopts.ip = addr->addr;
	i->pingopts.count = 1;
	i->pingopts.recv_function=ping_recv_callback;
	i->pingopts.sent_function=NULL;
	ping_start(&i->pingopts);
}

static void start_ping(struct pinger_instance *pinger)
{
	ip_addr_t addr;
	console_printf("%p \n", pinger);
	pinger->pending = 1;
	err_t error = espconn_gethostbyname(pinger, /* UNDOCUMENTED API: This one will get into dns_callback as arg*/
										pinger->target, &addr, dns_callback);

	if (error == ESPCONN_INPROGRESS) {

	} else if (error == ESPCONN_OK) {
		// Already in the local names table (or hostname was an IP address), execute the callback ourselves.
		dns_callback(pinger->target, &addr, pinger);
	} else {
		if (error == ESPCONN_ARG) {
			console_printf("DNS arg error %s\n", pinger->target);
		}
		else {
			console_printf("DNS error code %d\n", error);
		}
	}
}

static void pinger_workhorse(void *arg)
{
	console_printf("Koo!\n");
	//struct pinger_instance *pinger = arg;
	//if (pinger->pending)
	//	return;
	//start_ping(pinger);
}

static void create_background_pinger(const char *target, int period)
{
	console_printf("  pinger: Will ping %s every %d seconds\n", target, period);
	struct pinger_instance *pinger = os_zalloc(sizeof(struct ping_option));
	memset(pinger, 0x0, sizeof(*pinger));
	pinger->pingopts.count = 1;
	pinger->pingopts.recv_function=ping_recv_callback;
	pinger->pingopts.sent_function=NULL;
	pinger->target = strdup(target);
	pinger->pending = 0;

	//os_timer_disarm(&pinger->periodic);
	os_timer_setfn(&pinger->periodic, (os_timer_func_t *) pinger_workhorse, pinger);
	os_timer_arm(&pinger->periodic, period, 0);
}

static void do_ping()
{
	create_background_pinger("silverblade", 10000);
}

CONSOLE_CMD(ping2, 2, 2,
	do_ping, NULL, NULL,
	"Send icmp ping to specified address"
	HELPSTR_NEWLINE "ping 8.8.8.8"
);

FR_CONSTRUCTOR(sensor_ping)
{
	console_printf("senslog: Registering 'ping' sensor\n");
//	create_background_pinger("silverblade", 10000);
//	ip_addr_t ipaddr;
}
