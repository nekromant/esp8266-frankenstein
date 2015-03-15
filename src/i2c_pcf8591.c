#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "console.h"

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
	for(uint8 i = 0; i < 4; i++){
		//LAST_PCF8591_A[i] = i2c_master_readRegister16(PCF8591_ADDRESS, i) >> 8;
		LAST_PCF8591_A[i] = i2c_master_readRegister8(PCF8591_ADDRESS, i);
	}
	return true;
}

bool ICACHE_FLASH_ATTR 
PCF8591_Write(uint8 value)
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

static int do_i2c_pcf8591(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if(PCF8591_Read()){
			if(argc != 1){
				console_printf( "A0-A3: " );
			}
			console_printf( "%d %d %d %d\n", LAST_PCF8591_A[0], LAST_PCF8591_A[1], LAST_PCF8591_A[2], LAST_PCF8591_A[3]);
		}else{
			console_printf( "Failed to read value\n" );
		}
	} else

	if(strcmp(argv[1], "write") == 0){

		if( argc < 3 ){
			console_printf( "Value not specified\n" );
			return 0;
		}

		console_printf(PCF8591_Write(atoi(argv[2]))?"Ok":"Failed to write value");

	} else

	if(strcmp(argv[1], "init") == 0){

		console_printf( PCF8591_Init() ? "Ok\n":"Failed\n" );
	} 

	return 0;
}

CONSOLE_CMD(i2c_pcf8591, 0, 3, 
		do_i2c_pcf8591, NULL, NULL, 
		"I2C 8-bit ADC DAC PCF8591"
		HELPSTR_NEWLINE "i2c_pcf8591 init"
		HELPSTR_NEWLINE "i2c_pcf8591 [read]"
		HELPSTR_NEWLINE "i2c_pcf8591 write <value>"
);
