#ifndef __I2C_PCF8591_H
#define	__I2C_PCF8591_H

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

#define PCF8591_ADDRESS	0x90

#define PCF8591_REG_READ_ALL	0x04
#define PCF8591_REG_DAC	0x40

uint8 LAST_PCF8591_A[4];
static bool IS_ALREADY_INITED = false;

bool PCF8591_Init(void);
bool PCF8591_Read(void);
bool PCF8591_Write(uint8_t value);

#endif
