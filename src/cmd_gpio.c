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

#include <stdlib.h>
#include <stdlib.h>
#include <generic/macros.h>


static int  do_gpio(int argc, const char* const* argv)
{
	int gpio = atoi(argv[2]);
	
	if (strcmp(argv[1], "in") == 0) {
		GPIO_DIS_OUTPUT(gpio);
		console_printf("GP%d==%d\n", gpio, GPIO_INPUT_GET(gpio));
	} else 	if (strcmp(argv[1], "out") == 0) {
		if (argc < 4)
			return -1;
		int v = atoi(argv[3]);
		GPIO_OUTPUT_SET(gpio, v);
	}
	return 0;
}

CONSOLE_CMD(gpio, 3, 4, 
	    do_gpio, NULL, NULL, 
	    "Control gpio lines. gpio mode line [value] "
	    HELPSTR_NEWLINE "gpio in 0"
	    HELPSTR_NEWLINE "gpio out 0 1"
);

#ifdef CONFIG_ENABLE_MQTT
#include "lib/mqtt.h"

/*
 * Is this a reasonable limit?
 */
#define TOPIC_LEN 128

void ICACHE_FLASH_ATTR
gpio_sub_handler(const char *gpiostr, const char *arg)
{
	int gpio = atoi(gpiostr);
	int data = atoi(arg);

	/* TODO: Range check GPIO here */
	if (data == 0 || data == 1)
		GPIO_OUTPUT_SET(gpio, data);
}

static int ICACHE_FLASH_ATTR
do_gpio_sub(int argc, const char* const* argv)
{
	return MQTT_Do_Subscribe("gpio", argv[1], gpio_sub_handler);
}

static int ICACHE_FLASH_ATTR
do_gpio_pub(int argc, const char* const* argv)
{
	MQTT_Client *client = mqttGetConnectedClient();
	int gpio = atoi(argv[1]);
	char buf[6];
	int buflen;
	char topic[TOPIC_LEN];

	if (client == NULL) {
		console_printf("MQTT Client not bound to broker\r\n");
		return -1;
	}

	os_sprintf(topic, "%s/gpio/status/%d", client->connect_info.client_id, gpio);

	buflen = os_sprintf(buf, "%d", GPIO_INPUT_GET(gpio));
	MQTT_Publish(client, topic, buf, buflen, 0, 0);
	return 0;
}

CONSOLE_CMD(gpio_sub, 2,2,
		do_gpio_sub, NULL, NULL,
		"Subscribe to GPIO"
		HELPSTR_NEWLINE
		"gpio_sub <gpio>"
	   );

CONSOLE_CMD(gpio_pub, 2,2,
		do_gpio_pub, NULL, NULL,
		"Publish GPIO state"
		HELPSTR_NEWLINE
		"gpio_pub <gpio>"
	);

#endif // CONFIG_ENABLE_MQTT
