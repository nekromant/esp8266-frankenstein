#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "console.h"

#include "driver/i2c_master.h"
#include "driver/i2c_tcs3414cs.h"

#ifdef CONFIG_CMD_BH1750_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

bool ICACHE_FLASH_ATTR 
TCS3414_Read()
{
	if(!i2c_master_writeRegister(TCS3414_ADDRESS, TCS3414_REG_BLOCK_READ, 0)){
		return false;
	}

	i2c_master_start(); 
	i2c_master_writeByte(TCS3414_ADDRESS+1);
	if(!i2c_master_checkAck()){
		i2c_master_stop();
		return false;
	}

	os_delay_us(15 * 1000);
	for(uint8_t i = 0; i < 4; i++){
		uint8_t msb = i2c_master_readByte(); 
		i2c_master_setAck(1);
		uint8_t lsb = i2c_master_readByte(); 
		i2c_master_setAck(1);
		uint16_t color = ((msb << 8) | lsb);
		switch (i)
		{
			case 0: 
				LAST_TCS3414_COLOR.G = color;
				break;

			case 1: 
				LAST_TCS3414_COLOR.R = color;
				break;

			case 2: 
				LAST_TCS3414_COLOR.B = color;
				break;

			case 3: 
				LAST_TCS3414_COLOR.W = color;
				break;
		}
	}


	i2c_master_stop();
	return true;

}

bool ICACHE_FLASH_ATTR 
TCS3414_SetTimeing(uint8_t timeing, uint8_t gain)
{
	if(!i2c_master_writeRegister(TCS3414_ADDRESS, TCS3414_REG_TIMING, timeing)
	 ||!i2c_master_writeRegister(TCS3414_ADDRESS, TCS3414_REG_GAIN, gain)
	){
		return false;
	}

	return true;
}


bool ICACHE_FLASH_ATTR 
TCS3414_SetInterrupt(uint8_t interruptSource, uint8_t interruptControl)
{

	if(!i2c_master_writeRegister(TCS3414_ADDRESS, TCS3414_REG_INT_SOURCE, interruptSource)
	 ||!i2c_master_writeRegister(TCS3414_ADDRESS, TCS3414_REG_INT, interruptControl)
	){
		return false;
	}

	return true;
}

bool ICACHE_FLASH_ATTR 
TCS3414_Init()
{
	if(!TCS3414_SetTimeing(TCS3414_INTEGRATION_TIME_12ms, TCS3414_GAIN_1|TCS3414_PRESCALER_4)){
		return false;
	}

	if(!TCS3414_SetInterrupt(TCS3414_INT_SOURCE_CLEAR, TCS3414_INTR_DISABLE)){
		return false;
	}

	if(!i2c_master_writeRegister(TCS3414_ADDRESS, TCS3414_REG_CTL, TCS3414_REG_CTL | TCS3414_CTL_DAT_INIITIATE)){
		return false;
	}

	os_delay_us(15 * 1000);
	return true;
}


static int do_i2c_tcs3414(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if(TCS3414_Read()){
			if(argc != 1){
				console_printf( "RGBW: " );
			}
			console_printf( "%d %d %d %d\n", 
				(int)LAST_TCS3414_COLOR.R, (int)LAST_TCS3414_COLOR.G, (int)LAST_TCS3414_COLOR.B, (int)LAST_TCS3414_COLOR.W
			);
		}else{
			console_printf( "failed read value\n" );
		}
	} else

	if(strcmp(argv[1], "init") == 0){

		console_printf( TCS3414_Init() ? "Ok\n":"Failed\n" );
	} 

	return 0;
}

CONSOLE_CMD(i2c_tcs3414, 0, 2, 
		do_i2c_tcs3414, NULL, NULL, 
		"I2C Digital Color Sensor TCS3414"
		HELPSTR_NEWLINE "i2c_tcs3414 init"
		HELPSTR_NEWLINE "i2c_tcs3414 [read]"
);
