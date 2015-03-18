#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "console.h"

#include "driver/i2c_master.h"
#include "driver/i2c_bh1750.h"

#ifdef CONFIG_CMD_BH1750_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

static uint8 currentmode = BH1750_ONE_TIME_HIGH_RES_MODE;

bool BH1750_Read()
{
	if(!i2c_master_writeBytes1(BH1750_ADDRESS, currentmode)){
		return false;
	}

	os_delay_us(BH1750_CONVERSION_TIME*1000);

	uint16 light;
	if(i2c_master_readUint16(BH1750_ADDRESS, 0, &light)){
		LAST_BH_LIGHT = (light - 256) / 1.2;
		return true;
	}
	return false;
}


bool BH1750_Init(uint8 mode)
{
	if(!i2c_master_writeBytes1(BH1750_ADDRESS, mode)){
		return false;
	}

	currentmode = mode;
	IS_ALREADY_INITED = true;
	return true;
}

static int do_i2c_bh1750(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if((IS_ALREADY_INITED || BH1750_Init(BH1750_ONE_TIME_HIGH_RES_MODE)) && BH1750_Read()){
			console_printf( argc == 1 ? "%d\n" : "Light: %d lux\n", LAST_BH_LIGHT);
		}else{
			console_printf( "Failed to read value\n" );
		}
	} else

	if(strcmp(argv[1], "init") == 0){

		console_printf( BH1750_Init(BH1750_ONE_TIME_HIGH_RES_MODE) ? "Ok\n":"Failed\n" );
	} 

	return 0;
}

CONSOLE_CMD(i2c_bh1750, 0, 2, 
		do_i2c_bh1750, NULL, NULL, 
		"I2C light sensor BH1750"
		HELPSTR_NEWLINE "i2c_bh1750 init"
		HELPSTR_NEWLINE "i2c_bh1750 [read]"
);
