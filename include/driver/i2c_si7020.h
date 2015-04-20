#ifndef __I2C_SI7020_H
#define	__I2C_SI7020_H

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

#define SI7020_RESET	(0xfe)
#define SI7020_ADDRESS   (0x80)
#define SI7020_HUMIDITY (0xe5)
/*
 * 0xe0 re-uses the measurement made during the humidity measurement
 * 0xe3 is a real temperature measurement.
 */
#define SI7020_TEMPERATURE_STORED (0xe0)
#define SI7020_TEMPERATURE (0xe3)

bool  SI7020_Init(void);
bool SI7020_Read_Temperature(uint16_t *);
bool SI7020_Read_Humidity(uint16_t *);

static bool IS_ALREADY_INITED = false;
#endif
