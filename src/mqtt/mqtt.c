/* mqtt.c
*  Protocol: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "console.h"
#include <lib/mqtt_msg.h>
#include <lib/debug.h>
#include <lib/user_config.h>
#include <lib/config.h>
#include <lib/mqtt.h>
#include <lib/queue.h>
#include <lib/utils.h>

#define MQTT_TASK_PRIO        		0
#define MQTT_TASK_QUEUE_SIZE    	1
#define MQTT_SEND_TIMOUT			5

#ifndef QUEUE_BUFFER_SIZE
#define QUEUE_BUFFER_SIZE		 		2048
#endif

unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;

os_event_t mqtt_procTaskQueue[MQTT_TASK_QUEUE_SIZE];

LOCAL void ICACHE_FLASH_ATTR
mqtt_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
	struct espconn *pConn = (struct espconn *)arg;
	MQTT_Client* client = (MQTT_Client *)pConn->reverse;


	if(ipaddr == NULL)
	{
		console_printf("DNS: Found, but got no ip, try to reconnect\r\n");
		client->connState = TCP_RECONNECT_REQ;
		return;
	}

	console_printf("DNS: found ip %d.%d.%d.%d\n",
			*((uint8 *) &ipaddr->addr),
			*((uint8 *) &ipaddr->addr + 1),
			*((uint8 *) &ipaddr->addr + 2),
			*((uint8 *) &ipaddr->addr + 3));

	if(client->ip.addr == 0 && ipaddr->addr != 0)
	{
		os_memcpy(client->pCon->proto.tcp->remote_ip, &ipaddr->addr, 4);
		if(client->security){
			espconn_secure_connect(client->pCon);
		}
		else {
			espconn_connect(client->pCon);
		}

		client->connState = TCP_CONNECTING;
	}

	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
}



LOCAL void ICACHE_FLASH_ATTR
deliver_publish(MQTT_Client* client, uint8_t* message, int length)
{
	mqtt_event_data_t event_data;

	event_data.topic_length = length;
	event_data.topic = mqtt_get_publish_topic(message, &event_data.topic_length);
	event_data.data_length = length;
	event_data.data = mqtt_get_publish_data(message, &event_data.data_length);

	if(client->dataCb)
		client->dataCb((uint32_t*)client, event_data.topic, event_data.topic_length, event_data.data, event_data.data_length);

}


/**
  * @brief  Client received callback function.
  * @param  arg: contain the ip link information
  * @param  pdata: received data
  * @param  len: the lenght of received data
  * @retval None
  */
void ICACHE_FLASH_ATTR
mqtt_tcpclient_recv(void *arg, char *pdata, unsigned short len)
{
	uint8_t msg_type;
	uint8_t msg_qos;
	uint16_t msg_id;

	struct espconn *pCon = (struct espconn*)arg;
	MQTT_Client *client = (MQTT_Client *)pCon->reverse;

READPACKET:
	if(len < MQTT_BUF_SIZE && len > 0){
		os_memcpy(client->mqtt_state.in_buffer, pdata, len);

		msg_type = mqtt_get_type(client->mqtt_state.in_buffer);
		msg_qos = mqtt_get_qos(client->mqtt_state.in_buffer);
		msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);
		switch(client->connState){
		case MQTT_CONNECT_SENDING:
			if(msg_type == MQTT_MSG_TYPE_CONNACK){
				if(client->mqtt_state.pending_msg_type != MQTT_MSG_TYPE_CONNECT){
					console_printf("MQTT: Invalid packet\r\n");
					if(client->security){
						espconn_secure_disconnect(client->pCon);
					}
					else {
						espconn_disconnect(client->pCon);
					}
				} else {
					console_printf("MQTT: Connected to %s:%d\r\n", client->host, client->port);
					client->connState = MQTT_DATA;
					if(client->connectedCb)
						client->connectedCb((uint32_t*)client);
				}

			}
			break;
		case MQTT_DATA:
			client->mqtt_state.message_length_read = len;
			client->mqtt_state.message_length = mqtt_get_total_length(client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);


			switch(msg_type)
			{

			  case MQTT_MSG_TYPE_SUBACK:
				if(client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_SUBSCRIBE && client->mqtt_state.pending_msg_id == msg_id)
				  console_printf("MQTT: Subscribe successful\r\n");
				break;
			  case MQTT_MSG_TYPE_UNSUBACK:
				if(client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_UNSUBSCRIBE && client->mqtt_state.pending_msg_id == msg_id)
				  console_printf("MQTT: UnSubscribe successful\r\n");
				break;
			  case MQTT_MSG_TYPE_PUBLISH:
				if(msg_qos == 1)
					client->mqtt_state.outbound_message = mqtt_msg_puback(&client->mqtt_state.mqtt_connection, msg_id);
				else if(msg_qos == 2)
					client->mqtt_state.outbound_message = mqtt_msg_pubrec(&client->mqtt_state.mqtt_connection, msg_id);
				if(msg_qos == 1 || msg_qos == 2){
					console_printf("MQTT: Queue response QoS: %d\r\n", msg_qos);
					if(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
						console_printf("MQTT: Queue full\r\n");
					}
				}

				deliver_publish(client, client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
				break;
			  case MQTT_MSG_TYPE_PUBACK:
				if(client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH && client->mqtt_state.pending_msg_id == msg_id){
				  console_printf("MQTT: received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish\r\n");
				}

				break;
			  case MQTT_MSG_TYPE_PUBREC:
				  client->mqtt_state.outbound_message = mqtt_msg_pubrel(&client->mqtt_state.mqtt_connection, msg_id);
				  if(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
				  	console_printf("MQTT: Queue full\r\n");
				  }
				break;
			  case MQTT_MSG_TYPE_PUBREL:
				  client->mqtt_state.outbound_message = mqtt_msg_pubcomp(&client->mqtt_state.mqtt_connection, msg_id);
				  if(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
					console_printf("MQTT: Queue full\r\n");
				  }
				break;
			  case MQTT_MSG_TYPE_PUBCOMP:
				if(client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH && client->mqtt_state.pending_msg_id == msg_id){
				  console_printf("MQTT: receive MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish\r\n");
				}
				break;
			  case MQTT_MSG_TYPE_PINGREQ:
				  client->mqtt_state.outbound_message = mqtt_msg_pingresp(&client->mqtt_state.mqtt_connection);
				  if(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
					console_printf("MQTT: Queue full\r\n");
				  }
				break;
			  case MQTT_MSG_TYPE_PINGRESP:
				// Ignore
				break;
			}
			// NOTE: this is done down here and not in the switch case above
			// because the PSOCK_READBUF_LEN() won't work inside a switch
			// statement due to the way protothreads resume.
			if(msg_type == MQTT_MSG_TYPE_PUBLISH)
			{
			  len = client->mqtt_state.message_length_read;

			  if(client->mqtt_state.message_length < client->mqtt_state.message_length_read)
			  {
				  //client->connState = MQTT_PUBLISH_RECV;
				  //Not Implement yet
				  len -= client->mqtt_state.message_length;
				  pdata += client->mqtt_state.message_length;

				  goto READPACKET;
			  }

			}
			break;
		default:	// -Wswitch quench.
			break;
		}
	} else {
		console_printf("ERROR: Message too long\r\n");
	}
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
}

/**
  * @brief  Client send over callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
void ICACHE_FLASH_ATTR
mqtt_tcpclient_sent_cb(void *arg)
{
	struct espconn *pCon = (struct espconn *)arg;
	MQTT_Client* client = (MQTT_Client *)pCon->reverse;
	
	client->sendTimeout = 0;
	if(client->connState == MQTT_DATA && client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH){
		if(client->publishedCb)
			client->publishedCb((uint32_t*)client);
	}
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
}

void ICACHE_FLASH_ATTR mqtt_timer(void *arg)
{
	MQTT_Client* client = (MQTT_Client*)arg;

	if(client->connState == MQTT_DATA){
		client->keepAliveTick ++;
		if(client->keepAliveTick > client->mqtt_state.connect_info->keepalive){

			console_printf("\r\nMQTT: Send keepalive packet to %s:%d!\r\n", client->host, client->port);
			client->mqtt_state.outbound_message = mqtt_msg_pingreq(&client->mqtt_state.mqtt_connection);
			client->mqtt_state.pending_msg_type = MQTT_MSG_TYPE_PINGREQ;
			client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
			client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);


			client->sendTimeout = MQTT_SEND_TIMOUT;
			console_printf("MQTT: Sending, type: %d, id: %04X\r\n",client->mqtt_state.pending_msg_type, client->mqtt_state.pending_msg_id);
			if(client->security){
				espconn_secure_sent(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);
			}
			else{
				espconn_sent(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);
			}

			client->mqtt_state.outbound_message = NULL;

			client->keepAliveTick = 0;
			system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
		}

	} else if(client->connState == TCP_RECONNECT_REQ){
		client->reconnectTick ++;
		if(client->reconnectTick > MQTT_RECONNECT_TIMEOUT) {
			client->reconnectTick = 0;
			client->connState = TCP_RECONNECT;
			system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
		}
	}
	if(client->sendTimeout > 0)
		client->sendTimeout --;
}

void ICACHE_FLASH_ATTR
mqtt_tcpclient_discon_cb(void *arg)
{

	struct espconn *pespconn = (struct espconn *)arg;
	MQTT_Client* client = (MQTT_Client *)pespconn->reverse;
	client->connState = TCP_RECONNECT_REQ;
	if(client->disconnectedCb)
		client->disconnectedCb((uint32_t*)client);

	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
}



/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
void ICACHE_FLASH_ATTR
mqtt_tcpclient_connect_cb(void *arg)
{
	struct espconn *pCon = (struct espconn *)arg;
	MQTT_Client* client = (MQTT_Client *)pCon->reverse;

	espconn_regist_disconcb(client->pCon, mqtt_tcpclient_discon_cb);
	espconn_regist_recvcb(client->pCon, mqtt_tcpclient_recv);////////
	espconn_regist_sentcb(client->pCon, mqtt_tcpclient_sent_cb);///////
	console_printf("MQTT: Connected to broker %s:%d\r\n", client->host, client->port);

	mqtt_msg_init(&client->mqtt_state.mqtt_connection, client->mqtt_state.out_buffer, client->mqtt_state.out_buffer_length);
	client->mqtt_state.outbound_message = mqtt_msg_connect(&client->mqtt_state.mqtt_connection, client->mqtt_state.connect_info);
	client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
	client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);


	client->sendTimeout = MQTT_SEND_TIMOUT;
	console_printf("MQTT: Sending, type: %d, id: %04X\r\n",client->mqtt_state.pending_msg_type, client->mqtt_state.pending_msg_id);
	if(client->security){
		espconn_secure_sent(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);
	}
	else{
		espconn_sent(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);
	}

	client->mqtt_state.outbound_message = NULL;
	client->connState = MQTT_CONNECT_SENDING;
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
}

/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
void ICACHE_FLASH_ATTR
mqtt_tcpclient_recon_cb(void *arg, sint8 errType)
{
	struct espconn *pCon = (struct espconn *)arg;
	MQTT_Client* client = (MQTT_Client *)pCon->reverse;

	client->connState = TCP_RECONNECT_REQ;

	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);

}

/**
  * @brief  MQTT publish function.
  * @param  client: 	MQTT_Client reference
  * @param  topic: 		string topic will publish to
  * @param  data: 		buffer data send point to
  * @param  data_length: length of data
  * @param  qos:		qos
  * @param  retain:		retain
  * @retval TRUE if success queue
  */
BOOL ICACHE_FLASH_ATTR
MQTT_Publish(MQTT_Client *client, const char* topic, const char* data, int data_length, int qos, int retain)
{
	uint8_t dataBuffer[MQTT_BUF_SIZE];
	uint16_t dataLen;

	client->mqtt_state.outbound_message = mqtt_msg_publish(&client->mqtt_state.mqtt_connection,
										 topic, data, data_length,
										 qos, retain,
										 &client->mqtt_state.pending_msg_id);
	if(client->mqtt_state.outbound_message->length == 0){
		console_printf("MQTT: Queuing publish failed\r\n");
		return FALSE;
	}
	console_printf("MQTT: queuing publish, length: %d, queue size(%ld/%ld)\r\n", client->mqtt_state.outbound_message->length, client->msgQueue.rb.fill_cnt, client->msgQueue.rb.size);
	while(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
		console_printf("MQTT: Queue full\r\n");
		if(QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1) {
			console_printf("MQTT: Serious buffer error\r\n");
			return FALSE;
		}
	}
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
	return TRUE;
}

/**
  * @brief  MQTT subscibe function.
  * @param  client: 	MQTT_Client reference
  * @param  topic: 		string topic will subscribe
  * @param  qos:		qos
  * @retval TRUE if success queue
  */
BOOL ICACHE_FLASH_ATTR
MQTT_Subscribe(MQTT_Client *client, const char* topic, uint8_t qos)
{
	uint8_t dataBuffer[MQTT_BUF_SIZE];
	uint16_t dataLen;

	client->mqtt_state.outbound_message = mqtt_msg_subscribe(&client->mqtt_state.mqtt_connection,
											topic, 0,
											&client->mqtt_state.pending_msg_id);
	console_printf("MQTT: queue subscribe, topic\"%s\", id: %d\r\n",topic, client->mqtt_state.pending_msg_id);
	while(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
		console_printf("MQTT: Queue full\r\n");
		if(QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1) {
			console_printf("MQTT: Serious buffer error\r\n");
			return FALSE;
		}
	}
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
	return TRUE;
}

#define MAX_SUBSCRIBE_HANDLERS 5
#define MAX_SUBSCRIBE_DRIVER_LEN 8
#define MAX_SUBSCRIBE_ARG_LEN 32

static struct MQTT_Subscribe_Handler {
	char driver[MAX_SUBSCRIBE_DRIVER_LEN];
	void (*handler)(const char*, const char*);
} MQTT_Subscribe_Handlers[MAX_SUBSCRIBE_HANDLERS];

static int nMQTT_Subscribe_Handlers;

static int ICACHE_FLASH_ATTR
MQTT_Find_Subscribe_Handler(const char *driver)
{
	int i;

	for (i=0; i<nMQTT_Subscribe_Handlers; i++) {
		if (strncmp(driver, MQTT_Subscribe_Handlers[i].driver, MAX_SUBSCRIBE_DRIVER_LEN) == 0)
			return i;
	}
	return -1;
}

static int ICACHE_FLASH_ATTR
MQTT_Add_Subscribe_Handler(const char *driver, int driverlen, void (*handler)(const char*, const char*))
{

	if (driverlen > MAX_SUBSCRIBE_DRIVER_LEN)
		driverlen = MAX_SUBSCRIBE_DRIVER_LEN;
	/*
	 * Check if this driver has already registered a handler.  We only need
	 * one handler per driver
	 */
	if (nMQTT_Subscribe_Handlers >= MAX_SUBSCRIBE_HANDLERS)
		return 0;
	MQTT_Subscribe_Handlers[nMQTT_Subscribe_Handlers].handler = handler;
	memcpy(MQTT_Subscribe_Handlers[nMQTT_Subscribe_Handlers].driver, driver, driverlen);
	return ++nMQTT_Subscribe_Handlers;	// pre-increment to handle 0th handler
}

/*
 * Topic should be /deviceid/driver/arg
 * (This is kind of brute force but theoretically we are being passed
 * a well formatted topic string that we supplied at Subscribe-time.)
 */

static const char * ICACHE_FLASH_ATTR
MQTT_find_slash(const char *p)
{
	if (p) {
		while (*p)
		{
			if (*p == '/') {
				break;
			}
			p++;
		}
	}
	return p;

}

void ICACHE_FLASH_ATTR
MQTT_Call_Subscribe_Handler(const char *topic, const char *data)
{
	int i;
	const char *p;
	char driver[MAX_SUBSCRIBE_DRIVER_LEN];
	char arg[MAX_SUBSCRIBE_ARG_LEN];

	// skip over Device_ID in topic
	p = MQTT_find_slash(topic);

	// Now copy 'driver'
	// p should be '/'
	if (*p++ != '/')
		return; // Invalid topic format
	i = 0;
	memset(driver, 0, MAX_SUBSCRIBE_DRIVER_LEN);
	while (*p && *p != '/') {
		driver[i++] = *p++;
		if (i == MAX_SUBSCRIBE_DRIVER_LEN)
			break;
	}

	p = MQTT_find_slash(p);	// Should be at /ctl now
	if (*p++ != '/')
		return; // Invalid topic format. 
	p = MQTT_find_slash(p);	// Should be at /<arg> now

	// Now copy 'arg'
	if (*p++ != '/')
		return; // Invalid topic format.  Driverlen > MAX_SUBSCRIBE_DRIVER_LEN or we encountered a NULL

	i = 0;
	memset(arg, 0, MAX_SUBSCRIBE_ARG_LEN);
	while (*p && *p != '/') {
		arg[i++] = *p++;
		if (i == MAX_SUBSCRIBE_ARG_LEN)
			break;
	}

	if ((i = MQTT_Find_Subscribe_Handler(driver)) >= 0)
		MQTT_Subscribe_Handlers[i].handler(arg, data);

	return;
}

int ICACHE_FLASH_ATTR
MQTT_Do_Subscribe(const char *driver, const char *arg, void (*handler)())
{
	MQTT_Client *client = mqttGetConnectedClient();
	char topic[128];

	if (client == NULL) {
		console_printf("MQTT Client not bound to broker\r\n");
		return -1;
	}

	// TODO: bounds check on topic
	os_sprintf(topic, "%s/%s/ctl/%s", client->connect_info.client_id, driver, arg);

	if (MQTT_Add_Subscribe_Handler(driver, strlen(driver), handler))
		MQTT_Subscribe(client, topic, 0);
	else
		return -1;
	return 0;
}

void ICACHE_FLASH_ATTR
MQTT_Task(os_event_t *e)
{
	MQTT_Client* client = (MQTT_Client*)e->par;
	uint8_t dataBuffer[MQTT_BUF_SIZE];
	uint16_t dataLen;
	switch(client->connState){

	case TCP_RECONNECT_REQ:
		console_printf("TCP_RECONNECT_REQ\r\n");
		break;
	case TCP_RECONNECT:
		console_printf("TCP_RECONNECT\r\n");
		MQTT_Connect(client);
		client->connState = TCP_CONNECTING;
		break;
	case TCP_CONNECTING:
		console_printf("TCP_CONNECTING\r\n");
		break;
	case TCP_CONNECTING_ERROR:
		console_printf("TCP_CONNECTING_ERR\r\n");
		break;
	case MQTT_DATA:
		if(QUEUE_IsEmpty(&client->msgQueue) || client->sendTimeout != 0) {
			break;
		}
		if(QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == 0){
			client->mqtt_state.pending_msg_type = mqtt_get_type(dataBuffer);
			client->mqtt_state.pending_msg_id = mqtt_get_id(dataBuffer, dataLen);


			client->sendTimeout = MQTT_SEND_TIMOUT;
			console_printf("MQTT: Sending, type: %d, id: %04X\r\n",client->mqtt_state.pending_msg_type, client->mqtt_state.pending_msg_id);
			if(client->security){
				espconn_secure_sent(client->pCon, dataBuffer, dataLen);
			}
			else{
				espconn_sent(client->pCon, dataBuffer, dataLen);
			}

			client->mqtt_state.outbound_message = NULL;
			break;
		}
		break;
	default:	// -Wswitch quench.
		break;
	}
}

/**
  * @brief  MQTT initialization connection function
  * @param  client: 	MQTT_Client reference
  * @param  host: 	Domain or IP string
  * @param  port: 	Port to connect
  * @param  security:		1 for ssl, 0 for none
  * @retval None
  */
void ICACHE_FLASH_ATTR
MQTT_InitConnection(MQTT_Client *mqttClient, uint8_t* host, uint32 port, uint8_t security)
{
	uint32_t temp;
	os_memset(mqttClient, 0, sizeof(MQTT_Client));
	temp = os_strlen((const char *)host);
	mqttClient->host = (uint8_t*)os_zalloc(temp + 1);
	os_strcpy((char *)mqttClient->host, (const char *)host);
	mqttClient->host[temp] = 0;
	mqttClient->port = port;
	mqttClient->security = security;
	nMQTT_Subscribe_Handlers = 0;

}

/**
  * @brief  MQTT initialization mqtt client function
  * @param  client: 	MQTT_Client reference
  * @param  clientid: 	MQTT client id
  * @param  client_user:MQTT client user
  * @param  client_pass:MQTT client password
  * @param  client_pass:MQTT keep alive timer, in second
  * @retval None
  */
void ICACHE_FLASH_ATTR
MQTT_InitClient(MQTT_Client *mqttClient, uint8_t* client_id, uint8_t* client_user, uint8_t* client_pass, uint32_t keepAliveTime, uint8_t cleanSession)
{
	uint32_t temp;

	os_memset(&mqttClient->connect_info, 0, sizeof(mqtt_connect_info_t));

	temp = os_strlen((const char *)client_id);
	mqttClient->connect_info.client_id = (char *)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.client_id, (const char *)client_id);
	mqttClient->connect_info.client_id[temp] = 0;

	temp = os_strlen((const char *)client_user);
	mqttClient->connect_info.username = (char *)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.username, (const char *)client_user);
	mqttClient->connect_info.username[temp] = 0;

	temp = os_strlen((const char *)client_pass);
	mqttClient->connect_info.password = (char *)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.password, (const char *)client_pass);
	mqttClient->connect_info.password[temp] = 0;


	mqttClient->connect_info.keepalive = keepAliveTime;
	mqttClient->connect_info.clean_session = cleanSession;

	mqttClient->mqtt_state.in_buffer = (uint8_t *)os_zalloc(MQTT_BUF_SIZE);
	mqttClient->mqtt_state.in_buffer_length = MQTT_BUF_SIZE;
	mqttClient->mqtt_state.out_buffer =  (uint8_t *)os_zalloc(MQTT_BUF_SIZE);
	mqttClient->mqtt_state.out_buffer_length = MQTT_BUF_SIZE;
	mqttClient->mqtt_state.connect_info = &mqttClient->connect_info;

	mqtt_msg_init(&mqttClient->mqtt_state.mqtt_connection, mqttClient->mqtt_state.out_buffer, mqttClient->mqtt_state.out_buffer_length);

	QUEUE_Init(&mqttClient->msgQueue, QUEUE_BUFFER_SIZE);

	system_os_task(MQTT_Task, MQTT_TASK_PRIO, mqtt_procTaskQueue, MQTT_TASK_QUEUE_SIZE);
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)mqttClient);
}
void ICACHE_FLASH_ATTR
MQTT_InitLWT(MQTT_Client *mqttClient, uint8_t* will_topic, uint8_t* will_msg, uint8_t will_qos, uint8_t will_retain)
{
	uint32_t temp;

	temp = os_strlen((const char *)will_topic);
	mqttClient->connect_info.will_topic = (char *)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.will_topic, (const char *)will_topic);
	mqttClient->connect_info.will_topic[temp] = 0;

	temp = os_strlen((const char *)will_msg);
	mqttClient->connect_info.will_message = (char *)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.will_message, (const char *)will_msg);
	mqttClient->connect_info.will_message[temp] = 0;


	mqttClient->connect_info.will_qos = will_qos;
	mqttClient->connect_info.will_retain = will_retain;
}
/**
  * @brief  Begin connect to MQTT broker
  * @param  client: MQTT_Client reference
  * @retval None
  */
void ICACHE_FLASH_ATTR
MQTT_Connect(MQTT_Client *mqttClient)
{
	MQTT_Disconnect(mqttClient);
	mqttClient->pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
	mqttClient->pCon->type = ESPCONN_TCP;
	mqttClient->pCon->state = ESPCONN_NONE;
	mqttClient->pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	mqttClient->pCon->proto.tcp->local_port = espconn_port();
	mqttClient->pCon->proto.tcp->remote_port = mqttClient->port;
	mqttClient->pCon->reverse = mqttClient;
	espconn_regist_connectcb(mqttClient->pCon, mqtt_tcpclient_connect_cb);
	espconn_regist_reconcb(mqttClient->pCon, mqtt_tcpclient_recon_cb);

	mqttClient->keepAliveTick = 0;
	mqttClient->reconnectTick = 0;


	os_timer_disarm(&mqttClient->mqttTimer);
	os_timer_setfn(&mqttClient->mqttTimer, (os_timer_func_t *)mqtt_timer, mqttClient);
	os_timer_arm(&mqttClient->mqttTimer, 1000, 1);

	if(UTILS_StrToIP((const int8_t *)mqttClient->host, &mqttClient->pCon->proto.tcp->remote_ip)) {
		if(mqttClient->security){
			espconn_secure_connect(mqttClient->pCon);
		}
		else {
			espconn_connect(mqttClient->pCon);
		}
	}
	else {
		espconn_gethostbyname(mqttClient->pCon, (const char *)mqttClient->host, &mqttClient->ip, mqtt_dns_found);
	}
	mqttClient->connState = TCP_CONNECTING;
}

void ICACHE_FLASH_ATTR
MQTT_Disconnect(MQTT_Client *mqttClient)
{
	if(mqttClient->pCon){
		if(mqttClient->pCon->proto.tcp)
			os_free(mqttClient->pCon->proto.tcp);
		os_free(mqttClient->pCon);
		mqttClient->pCon = NULL;
	}

	os_timer_disarm(&mqttClient->mqttTimer);
}
void ICACHE_FLASH_ATTR
MQTT_OnConnected(MQTT_Client *mqttClient, MqttCallback connectedCb)
{
	mqttClient->connectedCb = connectedCb;
}

void ICACHE_FLASH_ATTR
MQTT_OnDisconnected(MQTT_Client *mqttClient, MqttCallback disconnectedCb)
{
	mqttClient->disconnectedCb = disconnectedCb;
}

void ICACHE_FLASH_ATTR
MQTT_OnData(MQTT_Client *mqttClient, MqttDataCallback dataCb)
{
	mqttClient->dataCb = dataCb;
}

void ICACHE_FLASH_ATTR
MQTT_OnPublished(MQTT_Client *mqttClient, MqttCallback publishedCb)
{
	mqttClient->publishedCb = publishedCb;
}
