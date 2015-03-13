#ifndef __I2C_BMP180_H
#define	__I2C_BMP180_H

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

#define BMP180_ADDRESS   0xEE

#define BMP180_MAGIC_VERSION 0x55
#define BMP180_CONVERSION_TIME 5
#define BMP180_REG_VERSION 0xD0
#define	BMP180_REG_CONTROL 0xF4
#define	BMP180_REG_RESULT 0xF6
#define	BMP180_COMMAND_TEMPERATURE 0x2E // Max conversion time 4.5ms
#define	BMP180_COMMAND_PRESSURE0 0x34 // Max conversion time 4.5ms (OSS = 0)
#define	BMP180_COMMAND_PRESSURE1 0x74 // Max conversion time 7.5ms (OSS = 1)
#define	BMP180_COMMAND_PRESSURE2 0xB4 // Max conversion time 13.5ms (OSS = 2)
#define	BMP180_COMMAND_PRESSURE3 0xF4 // Max conversion time 25.5ms (OSS = 3)


int32_t LAST_BMP_TEMPERATURE;
int32_t LAST_BMP_REAL_PRESSURE;

bool BMP180_Init(void);
bool BMP180_Read(void);

#endif
