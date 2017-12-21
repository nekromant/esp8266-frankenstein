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


static double get_something(struct slogger_data_type *tp)
{
	return system_get_time() / 1000000.0;
}

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
            .get_current_value = get_something
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


static void request_finalize() {
	console_lock(0);
	char *tmp = cur_rq->userdata;
	slogger_http_request_release(cur_rq);
	cur_rq = NULL;
	if (tmp)
		console_write(tmp, strlen(tmp));
}

void http_cb(char *response_body, int http_status, char *response_headers, int body_size)
{
	console_printf("http status: %d\n", http_status);
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
		console_printf("server response: %s\n", response_body);
	}
	request_finalize();
}

void http_data_type_cb(char *response_body, int http_status, char *response_headers, int body_size)
{
	console_printf("http status: %d\n", http_status);
	if (http_status != HTTP_STATUS_GENERIC_ERROR)
		slogger_instance_populate_data_type_ids(&this, response_body);
	request_finalize();
}

static int   do_svclog(int argc, const char *const *argv)
{
	struct slogger_http_request *rq;
	http_callback cb = http_cb;
	svclog_load_env();


	if (strcmp(argv[1], "register") == 0) {
		rq = slogger_instance_rq_register(&this);
		rq->userdata = "senslog get_dt\n\r\n\r";
		console_printf("Registering device at %s\n", rq->url);
	} else if (strcmp(argv[1], "post") == 0) {
		rq = slogger_instance_rq_post(&this);
		console_printf("Posting data to %s\n", rq->url);
	} else if (strcmp(argv[1], "get_dt") == 0) {
		rq = slogger_instance_rq_get_data_types(&this);
		cb = http_data_type_cb;
		console_printf("Requesting info at %s\n", rq->url);
	} else {
		console_printf("Unknown op %s\n", argv[1]);
		return 0;
	}

	cur_rq = rq;
	console_printf("%s\n", rq->data);
	http_post(rq->url, rq->data, rq->headers, cb);
	console_lock(1);
	return 0;
}

static void do_svclog_interrupt(void)
{
}


CONSOLE_CMD(senslog, 2, -1,
	    do_svclog, do_svclog_interrupt, NULL,
	    "Register and send data to a nextcloud sensorlogger."
	    HELPSTR_NEWLINE "senslog register       - registers this device with logger"
	    HELPSTR_NEWLINE "senslog get_data_types - obtains data type ids from server"
	    HELPSTR_NEWLINE "senslog post           - posts data from all configured sensors"
	    );
