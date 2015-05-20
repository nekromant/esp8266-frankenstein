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
MQTT_Client* pub_client;


void mqttConnectedCb(uint32_t *args)
{
  console_printf("\r\nMQTT: mqttConnectedCb\r\n");
}

void mqttDisconnectedCb(uint32_t *args)
{
  console_printf("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
  console_printf("MQTT: Published\r\n");
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
  os_free(topicBuf);
  os_free(dataBuf);
}


static int   do_startmqtt(int argc, const char* const* argv)
{
	CFG_Load();
	//MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	//MQTT_InitConnection(&mqttClient, "broker.mqttdashboard.com", 1883, 0);
	//MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_InitConnection(&mqttClient, (uint8_t *)"192.168.37.30", 1883, 0);
	MQTT_InitClient(&mqttClient, (uint8_t *)"esp8266_mqtt_client", (uint8_t *)"", (uint8_t *)"", 120, 1);

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
	console_printf("In stopmqtt\n");
	MQTT_Disconnect(&mqttClient);
	return 0;
}

static int   do_mqttpub(int argc, const char* const* argv)
{
	console_printf("In mqttpub\n");
	MQTT_Publish(pub_client, "A/1", "hello0", 6, 0, 0);
	return 0;
}

static int   do_mqttsub(int argc, const char* const* argv)
{
	console_printf("In mqttsub\n");
	MQTT_Subscribe(pub_client, "/A/1", 0);
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

CONSOLE_CMD(mqttpub, 1, 1, 
	    do_mqttpub, NULL, NULL, 
	    "Publish MQTT"
);

CONSOLE_CMD(mqttsub, 1, 1, 
	    do_mqttsub, NULL, NULL, 
	    "Subscribe MQTT"
);
