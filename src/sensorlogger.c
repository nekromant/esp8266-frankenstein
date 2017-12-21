#include <stdio.h>
#include <stdlib.h>
#include <base64.h>
#include <cJSON.h>
#include <sensorlogger.h>

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


static char *slogger_get_headers(struct slogger_instance *inst)
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

struct slogger_http_request *slogger_instance_rq_register(struct slogger_instance *inst)
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
		sprintf(smallBuff, "%i.0%u", val1, val2);
	else
		sprintf(smallBuff, "%i.%u", val1, val2);
}

struct slogger_http_request *slogger_instance_rq_post(struct slogger_instance *inst, int id, double value)
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
