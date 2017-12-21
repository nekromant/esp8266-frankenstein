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
#include "httpclient.h"
#include <sensorlogger.h>
#include <stdlib.h>
#include <generic/macros.h>




//struct slogger_instace *slogger_instance_create(struct slogger_opts *opts);
//int slogger_instance_register_datatype(struct slogger_instace *i, const char *type, const char* desc, const char* unit);
//struct slogger_report *slogger_instance_create_report(struct slogger_instance *i);
//int slogger_report_add_data(int id, )
//void slogger_report_add_data(struct slogger_instance *i);



static struct slogger_instance this = {
	.deviceId			= "dev-arven-001",
	.deviceType			= "dummy",
	.deviceGroup		= "development",
	.deviceName			= "arven-green",
	.userName			= "andrew",
	.password			= "JMjpL-7o5dP-ez8DK-jQzSq-8aiqM",
	.nextCloudUrl		= "https://cloud.ncrmnt.org",
	.deviceDataTypes	= {
		{
			.type		= "mana",
			.description	= "Mana regen rate",
			.unit		= "pts/sec",
		},
		{
		}
	}
};

#define load_param(key) \
	tmp = env_get("slog-" #key); \
	if (tmp) { \
		this.key = tmp; \
	}

static void svclog_load_env()
{
	char *tmp;
	load_param(deviceId);
	load_param(deviceType);
	load_param(deviceGroup);
	load_param(deviceParentGroup);
	load_param(deviceName);
	load_param(userName);
	load_param(password);
	load_param(nextCloudUrl);
}

static struct slogger_http_request *cur_rq;

void http_cb(char *response_body, int http_status, char *response_headers, int body_size)
{
	console_printf("http status: %d\n", http_status);
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
		console_printf("server response: %s\n", response_body);
	}
	slogger_http_request_release(cur_rq);
	cur_rq = NULL;
	console_lock(0);
}

static int   do_svclog(int argc, const char *const *argv)
{
	struct slogger_http_request *rq;
	svclog_load_env();
	if (strcmp(argv[1], "register") == 0) {
		rq = slogger_instance_rq_register(&this);
		console_printf("Registering device at %s\n", rq->url);
	} else if (strcmp(argv[1], "post") == 0) {
		rq = slogger_instance_rq_post(&this, 0, system_get_time() / 1000000.0);
		console_printf("Posting data to %s\n", rq->url);
	} else {
		console_printf("Unknown op %s\n", argv[1]);
		return 0;
	}

	cur_rq = rq;
	console_printf("%s\n", rq->data);
	http_post(rq->url, rq->data, rq->headers, http_cb);
	console_lock(1);
	return 0;
}

static void do_svclog_interrupt(void)
{
}


CONSOLE_CMD(senslog, 1, -1,
	    do_svclog, do_svclog_interrupt, NULL,
	    "Send data to a remote host."
	    HELPSTR_NEWLINE "Send svclog request"
	    );
