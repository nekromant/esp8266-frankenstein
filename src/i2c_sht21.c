#include "driver/i2c_master.h"
#include "driver/i2c_sht21.h"
#include "console.h"

#ifdef CONFIG_CMD_SHT21_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

bool SHT21_Init()
{
	
	if(i2c_master_writeBytes1(SHT21_ADDRESS, SHT21_SOFT_RESET))
	{
		// The soft reset takes less than 15ms.
		os_delay_us(SHT21_SOFT_RESET_TIME*1000);
		IS_ALREADY_INITED = true;
		return true;
	}
	return false;
}

bool SHT21_Read()
{
#ifdef CONFIG_USEFLOAT
#define PRECISION_MULTI 1.0
	float temp;
#else
#define PRECISION_MULTI 100
	uint16 temp;
#endif

	if(i2c_master_readUint16(SHT21_ADDRESS, SHT21_TRIGGER_TEMP_MEASURE_NOHOLD, &temp)){
		LAST_SHT_TEMPERATURE = (-46.85 + 175.72 / 65536.0 * (temp & ~3)) * PRECISION_MULTI;
	} else return false;
	
	if(i2c_master_readUint16(SHT21_ADDRESS, SHT21_TRIGGER_HUMD_MEASURE_NOHOLD, &temp)){
		LAST_SHT_HUMIDITY = (-6.0 + 125.0 / 65536.0 * (temp & ~3)) * PRECISION_MULTI;
	} else return false;

	//TODO read data in hold mode
	//LAST_SHT_TEMPERATURE = i2c_master_readRegister16(SHT21_ADDRESS, SHT21_TRIGGER_TEMP_MEASURE_HOLD) & ~3;
	//LAST_SHT_HUMIDITY = i2c_master_readRegister16(SHT21_ADDRESS, SHT21_TRIGGER_TEMP_MEASURE_NOHOLD) & ~3;

	return true;
}

static int do_i2c_sht21(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if((IS_ALREADY_INITED || SHT21_Init()) && SHT21_Read()){
			console_printf( argc == 1 ? "%d %d\n" : "Temperature: %d C\nHumidity: %d %%\n", 
#ifdef CONFIG_USEFLOAT
				(int)(LAST_SHT_TEMPERATURE*100),
				(int)(LAST_SHT_HUMIDITY*100)
#else
				LAST_SHT_TEMPERATURE,
				LAST_SHT_HUMIDITY
#endif
			);
		}else{
			console_printf( "Failed to read value\n" );
		}
	} else

	if(strcmp(argv[1], "init") == 0){

		console_printf( SHT21_Init() ? "Ok\n":"Failed\n" );
	} 

	return 0;
}

CONSOLE_CMD(i2c_sht21, 0, 2, 
		do_i2c_sht21, NULL, NULL, 
		"I2C humidity sensor SHT21"
		HELPSTR_NEWLINE "i2c_sht21 init"
		HELPSTR_NEWLINE "i2c_sht21 [read]"
);
