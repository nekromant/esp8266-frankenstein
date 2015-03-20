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


void mqttConnectedCb(uint32_t *args)
{
  MQTT_Client* client = (MQTT_Client*)args;
  console_printf("MQTT: Connected\r\n");
  /*MQTT_Subscribe(client, "/mqtt/topic/0", 0);
  MQTT_Subscribe(client, "/mqtt/topic/1", 1);
  MQTT_Subscribe(client, "/mqtt/topic/2", 2);*/

  MQTT_Publish(client, "A/1", "hello0", 6, 0, 0);
  MQTT_Publish(client, "A/1", "hello1", 6, 1, 0);
  MQTT_Publish(client, "A/1", "hello2", 6, 2, 0);
}

void mqttDisconnectedCb(uint32_t *args)
{
  MQTT_Client* client = (MQTT_Client*)args;
  console_printf("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
  MQTT_Client* client = (MQTT_Client*)args;
  console_printf("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
  char *topicBuf = (char*)os_zalloc(topic_len+1),
      *dataBuf = (char*)os_zalloc(data_len+1);

  MQTT_Client* client = (MQTT_Client*)args;

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
	console_printf("In startmqtt\n");
	//MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	MQTT_InitConnection(&mqttClient, "192.168.0.104", 1883, 0);
	//MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_InitClient(&mqttClient, "client_id", "", "", 120, 1);

	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0); 
  	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
  	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
  	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
  	MQTT_OnData(&mqttClient, mqttDataCb);
	return 0;
}

static int   do_stopmqtt(int argc, const char* const* argv)
{
	console_printf("In stopmqtt\n");
	return 0;
}

static int   do_mqttpub(int argc, const char* const* argv)
{
	console_printf("In mqttpub\n");
	return 0;
}

static int   do_mqttsub(int argc, const char* const* argv)
{
	console_printf("In mqttsub\n");
	return 0;
}


CONSOLE_CMD(startmqtt, 1, 1, 
	    do_startmqtt, NULL, NULL, 
	    "Start MQTT Service"
);

CONSOLE_CMD(stopmqtt, 3, 3, 
	    do_stopmqtt, NULL, NULL, 
	    "Stop MQTT Service"
);

CONSOLE_CMD(mqttpub, 2, 2, 
	    do_mqttpub, NULL, NULL, 
	    "Publish MQTT"
);

CONSOLE_CMD(mqttsub, 1, 1, 
	    do_mqttsub, NULL, NULL, 
	    "Subscribe MQTT"
);