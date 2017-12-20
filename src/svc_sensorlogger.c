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

#include <stdlib.h>
#include <generic/macros.h>
#include <cJSON.h>
#include <base64.h>

struct slogger_data_type {
	const char *	type;
	const char *	description;
	const char *	unit;
};

struct slogger_instance {
	char *				deviceId;
	char *				deviceName;
	char *				deviceType;
	char *				deviceGroup;
	char *				deviceParentGroup;
	char *				nextCloudUrl;
	char *				userName;
	char *				password;
	struct slogger_data_type	deviceDataTypes[];
};

struct slogger_http_request {
	char *	data;
	char *	url;
	char *	headers;
};

void slogger_http_request_release(struct slogger_http_request *rq)
{
	free(rq->url);
	free(rq->data);
	free(rq->headers);
	free(rq);
}

#define APPEND(r, object, field) \
	if (object->field) \
		cJSON_AddItemToObject(r, #field, cJSON_CreateString(object->field)); \
	else \
		cJSON_AddItemToObject(r, #field, cJSON_CreateString(""));


char *slogger_get_headers(struct slogger_instance *inst)
{
	char *ret;
	char *cred;
	char *base64_cred;

	int len = asprintf(&cred, "%s:%s", inst->userName, inst->password);

	base64_cred = malloc(b64e_size(len) + 1);
	b64_encode(cred, len, base64_cred);

	asprintf(&ret,
		 "Content-Type: application/json\r\n"
		 "Authorization: Basic %s\r\n",
		 base64_cred
		 );
	free(cred);
	free(base64_cred);
	return ret;
}

struct slogger_http_request *slogger_instance_register(struct slogger_instance *inst)
{
	struct slogger_http_request *ret;
	cJSON *root, *dt;

	root = cJSON_CreateObject();

	APPEND(root, inst, deviceId);
	APPEND(root, inst, deviceName);
	APPEND(root, inst, deviceType);
	APPEND(root, inst, deviceGroup);
	APPEND(root, inst, deviceParentGroup);

	cJSON_AddItemToObject(root, "deviceDataTypes", dt = cJSON_CreateObject());
	struct slogger_data_type *tp = inst->deviceDataTypes;
	while (tp->type) {
		APPEND(dt, tp, type);
		APPEND(dt, tp, description);
		APPEND(dt, tp, unit);
		tp++;
	}
	ret = malloc(sizeof(*ret));
	ret->data = cJSON_Print(root);
	ret->headers = slogger_get_headers(inst);
	asprintf(&ret->url, "%s/index.php/apps/sensorlogger/api/v1/registerdevice/", inst->nextCloudUrl);
	cJSON_Delete(root);
	return ret;
}


void float2char(float val, char *smallBuff)
{
	int val1 = (int)val;
	unsigned int val2;

	if (val < 0)
		val2 = (int)(-100.0 * val) % 100;
	else
		val2 = (int)(100.0 * val) % 100;
	if (val2 < 10)
		os_sprintf(smallBuff, "%i.0%u", val1, val2);
	else
		os_sprintf(smallBuff, "%i.%u", val1, val2);
}

struct slogger_http_request *slogger_instance_post(struct slogger_instance *inst, int id, double value)
{
	struct slogger_http_request *ret;
	cJSON *root;

	root = cJSON_CreateObject();
	APPEND(root, inst, deviceId);
	char tmp[64];
	float2char(value, tmp);
	cJSON_AddStringToObject(root, inst->deviceDataTypes[id].type, tmp);

	ret = malloc(sizeof(*ret));
	ret->data = cJSON_Print(root);
	ret->headers = slogger_get_headers(inst);
	asprintf(&ret->url, "%s/index.php/apps/sensorlogger/api/v1/createlog/", inst->nextCloudUrl);
	cJSON_Delete(root);

	return ret;
}

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
	.nextCloudUrl			= "https://cloud.ncrmnt.org",
	.deviceDataTypes		= {
		{
			.type		= "mana",
			.description	= "Mana regen rate",
			.unit		= "pts/sec",
		},
		{
		}
	}
};

static struct slogger_http_request *cur_rq;

void http_cb(char *response_body, int http_status, char *response_headers, int body_size)
{
	console_printf("http status: %d\n", http_status);
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
		console_printf("server response: %s\n", response_body); // FIXME: this does not handle binary data.
	}
	slogger_http_request_release(cur_rq);
	cur_rq = NULL;
	console_lock(0);
}

static int   do_svclog(int argc, const char *const *argv)
{
	struct slogger_http_request *rq;
	if (strcmp(argv[1], "register") == 0) {
		rq = slogger_instance_register(&this);
		console_printf("Registering device at %s\n", rq->url);
	} else if (strcmp(argv[1], "post") == 0) {
		rq = slogger_instance_post(&this, 0, system_get_time() / 1000000.0);
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


CONSOLE_CMD(svclog, 1, -1,
	    do_svclog, do_svclog_interrupt, NULL,
	    "Send data to a remote host."
	    HELPSTR_NEWLINE "Send svclog request"
	    );
