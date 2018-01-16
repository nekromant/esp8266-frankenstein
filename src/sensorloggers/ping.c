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
	ETSTimer periodic;
	int pending;
};

static void ping_recv_callback(void* arg, void *pdata)
{
	struct ping_resp *pingresp = pdata;
	struct pinger_instance *inst = arg;
	inst->data_type.current_value = pingresp->resp_time;
	inst->pending = 0;
}


static void dns_callback(const char * hostname, ip_addr_t * addr, void * arg)
{
	struct pinger_instance * i = arg;
	if (!addr) {
		i->pending = 0;
		i->data_type.current_value = -1;
		return;
	}

	i->pingopts.ip = addr->addr;
	i->pingopts.count = 1;
	i->pingopts.recv_function=ping_recv_callback;
	i->pingopts.sent_function=NULL;
	ping_start(&i->pingopts);
}

static void start_ping(struct pinger_instance *pinger)
{
	ip_addr_t addr;
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
	struct pinger_instance *pinger = arg;
	if (pinger->pending)
		return;
	start_ping(pinger);
}

static void create_background_pinger(const char *target, int period)
{
	struct pinger_instance *pinger = os_malloc(sizeof(*pinger));
	console_printf("    pinger: Will ping %s every %d ms\n", target, period);
	memset(pinger, 0x0, sizeof(*pinger));
	pinger->pingopts.count = 1;
	pinger->pingopts.recv_function=ping_recv_callback;
	pinger->pingopts.sent_function=NULL;
	pinger->target = strdup(target);
	pinger->pending = 0;

	pinger->data_type.unit = "ms";
	pinger->data_type.get_current_value = NULL;
	asprintf(&pinger->data_type.type, "%s", target);
	asprintf(&pinger->data_type.description, "%s response time", target);

	struct slogger_instance * inst = svclog_get_global_instance();
	sensorlogger_instance_register_data_type(inst, &pinger->data_type);

	os_timer_disarm(&pinger->periodic);
	os_timer_setfn(&pinger->periodic, (os_timer_func_t *) pinger_workhorse, pinger);
	os_timer_arm(&pinger->periodic, period, 1);
}

static void do_ping()
{
	create_background_pinger("8.8.8.8", 10000);
}

CONSOLE_CMD(ping2, 2, 2,
	do_ping, NULL, NULL,
	"Send icmp ping to specified address"
	HELPSTR_NEWLINE "ping 8.8.8.8"
);

FR_CONSTRUCTOR(sensor_ping)
{
	console_printf("senslog: Registering 'ping' sensors\n");
	int timeout = 10000;
	char *hosts = env_get("ping-hosts");
	char *timeout_str = env_get("ping-period");
	if (timeout_str)
		timeout = atoi(timeout_str);
	if (!hosts)
		return;
	hosts = strdup(hosts);
	char *host = strtok(hosts, ",");
	do {
		create_background_pinger(host, timeout);
		host = strtok(NULL, ",");
	} while (host);
	os_free(hosts);
}
