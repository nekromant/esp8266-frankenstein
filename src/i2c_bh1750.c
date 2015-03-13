#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

#include "driver/i2c_master.h"
#include "driver/i2c_bh1750.h"

#ifdef CONFIG_CMD_BH1750_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

static uint8 currentmode = BH1750_ONE_TIME_HIGH_RES_MODE;

bool ICACHE_FLASH_ATTR 
BH1750_Read()
{
	if(!i2c_master_writeRegister(BH1750_ADDRESS, currentmode, 0)){
		return false;
	}

	os_delay_us(BH1750_CONVERSION_TIME*1000);

	LAST_BH_LIGHT = (i2c_master_readRegister16(BH1750_ADDRESS, 0) - 256) / 1.2;
	return true;
}


bool ICACHE_FLASH_ATTR 
BH1750_Init(uint8 mode)
{
	if(!i2c_master_writeRegister(BH1750_ADDRESS, mode, 0)){
		return false;
	}

	currentmode = mode;
	return true;
}

