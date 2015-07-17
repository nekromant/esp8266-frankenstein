#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "driver/uart.h" 
#include "microrl.h"
#include "console.h"

#include <stdlib.h>
#include <stdlib.h>
#include <generic/macros.h>
#include "lib/mqtt.h"
#include "lib/config.h"

MQTT_Client mqttClient;
MQTT_Client* mqttConnectedClient;

MQTT_Client *
mqttGetConnectedClient(void)
{
	return mqttConnectedClient;
}

void mqttConnectedCb(uint32_t *args)
{
	mqttConnectedClient = (MQTT_Client *)args;
	return;
}

void mqttDisconnectedCb(uint32_t *args)
{
	mqttConnectedClient = (MQTT_Client *)NULL;
	return;
}

void mqttPublishedCb(uint32_t *args)
{
	return;
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
  char *topicBuf = (char*)os_zalloc(topic_len+1),
      *dataBuf = (char*)os_zalloc(data_len+1);

  os_memcpy(topicBuf, topic, topic_len);
  topicBuf[topic_len] = 0;

  os_memcpy(dataBuf, data, data_len);
  dataBuf[data_len] = 0;

  console_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
  MQTT_Call_Subscribe_Handler(topicBuf, dataBuf);

  os_free(topicBuf);
  os_free(dataBuf);
}




static int   do_startmqtt(int argc, const char* const* argv)
{
	CFG_Load();

	MQTT_InitConnection(&mqttClient, (uint8_t *)sysCfg.mqtt_host, sysCfg.mqtt_port, 0);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);

	MQTT_InitLWT(&mqttClient, (uint8_t *)"/lwt", (uint8_t *)"offline", 0, 0); 
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);
	MQTT_Connect(&mqttClient);

	return 0;
}

static int   do_stopmqtt(int argc, const char* const* argv)
{
	MQTT_Disconnect(&mqttClient);
	return 0;
}

static int   do_mqttpub(int argc, const char* const* argv)
{
	if (argc < 3)
		return -1;

	if (mqttConnectedClient != NULL)
		MQTT_Publish(mqttConnectedClient, argv[1], argv[2], strlen(argv[2]), 0, 0);
	return 0;
}

static int   do_mqttsub(int argc, const char* const* argv)
{
	if (argc < 2)
		return -1;

	if (mqttConnectedClient != NULL)
		MQTT_Subscribe(mqttConnectedClient, argv[1], 0);
	return 0;
}

CONSOLE_CMD(startmqtt, 1, 1, 
	    do_startmqtt, NULL, NULL, 
	    "Start MQTT Service"
);

CONSOLE_CMD(stopmqtt, 1, 1, 
	    do_stopmqtt, NULL, NULL, 
	    "Stop MQTT Service"
);

CONSOLE_CMD(mqttpub, 3, 3, 
	    do_mqttpub, NULL, NULL, 
	    "Publish MQTT"
);

CONSOLE_CMD(mqttsub, 2, 2, 
	    do_mqttsub, NULL, NULL, 
	    "Subscribe MQTT"
);
