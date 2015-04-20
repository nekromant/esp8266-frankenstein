#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "console.h"

#include "driver/i2c_master.h"
#include "driver/i2c_si7020.h"

#ifdef CONFIG_CMD_SI7020_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

bool
SI7020_Init()
{
	if (i2c_master_writeBytes1(SI7020_ADDRESS, SI7020_RESET))
	{
		os_delay_us(10000);
		IS_ALREADY_INITED = true;
		return true;
	}
	return false;
}

bool
SI7020_Read_Temperature(uint16_t *datum)
{
	if(!i2c_master_writeBytes1(SI7020_ADDRESS, SI7020_TEMPERATURE)){
		return false;
	}

	datum[0] = 0;
	if(i2c_master_readUint16(SI7020_ADDRESS, 0, datum)){
		return true;
	}
	return false;
}

bool
SI7020_Read_Humidity(uint16_t *datum)
{
	uint8 reg = SI7020_HUMIDITY;

	if(!i2c_master_writeBytes(SI7020_ADDRESS, &reg, 1)){
		return false;
	}

	datum[0] = 0;
	if(i2c_master_readUint16(SI7020_ADDRESS, 0, datum)){
		return true;
	}
	return false;
}

static int
do_i2c_si7020(int argc, const char* const* argv)
{
	uint16_t temp, hum;
	uint8_t datum[2];

	if (!IS_ALREADY_INITED)
	       SI7020_Init();
	if(argc == 1 || strcmp(argv[1], "gethumidity") == 0){
		if((SI7020_Read_Humidity((uint16_t *)datum))) {
			hum = ((((datum[1]<<8)|datum[0])*125.0)/65536)-6.0;
			console_printf( argc == 1 ? "%d\n" : "Humidity: %d\n", hum);
		}else{
			console_printf( "Failed to read value\n" );
		}
	}
	if(argc == 1 || strcmp(argv[1], "gettemp") == 0){
		if((SI7020_Read_Temperature((uint16_t *)datum))) {
			temp = ((((datum[1]<<8)|datum[0])*175.72)/65536)-46.85;
			console_printf( argc == 1 ? "%d\n" : "Temperature: %d\n", temp);
		}else{
			console_printf( "Failed to read value\n" );
		}
	}

	return 0;
}

CONSOLE_CMD(i2c_si7020, 0, 2, 
		do_i2c_si7020, NULL, NULL, 
		"I2C Temperature/Humidity sensor - SI7020"
		HELPSTR_NEWLINE "i2c_si7020 init"
		HELPSTR_NEWLINE "i2c_si7020 [gethumidity]"
		HELPSTR_NEWLINE "i2c_si7020 [gettemp]"
);
