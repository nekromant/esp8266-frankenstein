#ifndef __I2C_BH1750_H
#define	__I2C_BH1750_H

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

#define BH1750_ADDRESS 0x46 //Add Low
//#define BH1750_ADDRESS 0xB8 //Add High

#define BH1750_CONVERSION_TIME 150
#define BH1750_CONTINUOUS_HIGH_RES_MODE	0x10		// 1 lx resolution, 120ms 
#define BH1750_CONTINUOUS_HIGH_RES_MODE_2	0x11	// 0.5 lx resolution, 120ms 	
#define BH1750_CONTINUOUS_LOW_RES_MODE	0x13		// 4 lx resolution, 16ms.
#define BH1750_ONE_TIME_HIGH_RES_MODE	0x20
#define BH1750_ONE_TIME_HIGH_RES_MODE_2	0x21
#define BH1750_ONE_TIME_LOW_RES_MODE	0x23
#define BH1750_POWER_DOWN	0x00	// No active state
#define BH1750_POWER_ON	0x01		// Wating for measurment command
#define BH1750_RESET	0x07		// Reset data register value - not accepted in POWER_DOWN mode

uint16_t LAST_BH_LIGHT;
static bool IS_ALREADY_INITED = false;

bool BH1750_Init(uint8 mode);
bool BH1750_Read(void);

#endif