#ifndef __DHT22_H__
#define __DHT22_H__

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"

typedef enum{
	DHT11 = 0,
	DHT22
} DHT_Type;

#define DHT_MAXTIMINGS	40
#define DHT_READTIMEOUT	250
#define DHT_BREAKTIME	20

static bool IS_ALREADY_INITED;

sint16 LAST_DHT_TEMPERATURE;
uint16 LAST_DHT_HUMIDITY;

bool DHT_Init(uint8 gpiopin, DHT_Type dht_type);
bool DHT_Read(void);

#endif
