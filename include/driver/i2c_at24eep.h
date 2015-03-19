#ifndef __I2C_AT24EEP_H
#define	__I2C_AT24EEP_H

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

#define AT24EEP_ADDRESS 0x50
// may change from 0x50 to 0x57 depend on A0 A1

static bool IS_ALREADY_INITED = false;

bool AT24EEP_Init(void);
bool AT24EEP_Read(void);
bool AT24EEP_Write(void);

#endif