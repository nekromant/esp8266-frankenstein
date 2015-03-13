#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

#include "driver/i2c_master.h"
#include "driver/i2c_pcf8591.h"

#ifdef CONFIG_CMD_BH1750_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

bool ICACHE_FLASH_ATTR 
PCF8591_Read()
{
	for(uint8_t i = 0; i < 4; i++){
		//LAST_PCF8591_A[i] = i2c_master_readRegister16(PCF8591_ADDRESS, i) >> 8;
		LAST_PCF8591_A[i] = i2c_master_readRegister8(PCF8591_ADDRESS, i);
	}
	return true;
}

bool ICACHE_FLASH_ATTR 
PCF8591_Write(uint8_t value)
{
	return i2c_master_writeRegister(PCF8591_ADDRESS, PCF8591_REG_DAC, value);
}


bool ICACHE_FLASH_ATTR 
PCF8591_Init()
{
	//TODO: maybe need some check?
	//TODO: configure for differential input
	return i2c_master_writeRegister(PCF8591_ADDRESS, 0, 0);
}

