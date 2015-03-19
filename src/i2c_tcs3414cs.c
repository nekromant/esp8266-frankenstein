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

bool TCS3414_Read()
{
	
	if(!i2c_master_writeBytes1(TCS3414_ADDRESS, TCS3414_REG_BLOCK_READ)){
		console_printf( "Failed to read colors\n" );
		return false;
	}

	uint8 GRBW[8];
	GRBW[0] = TCS3414_REG_BLOCK_READ;
	if(i2c_master_readBytes(TCS3414_ADDRESS, GRBW, 8)){
		for(uint8_t i = 0; i < 4; i++){
			uint16_t color = ((GRBW[i*2] << 8) | GRBW[i*2+1]);
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
		return true;
	}

	return false;
}

bool TCS3414_SetTimeing(uint8_t timeing, uint8_t gain)
{
	return i2c_master_writeBytes2(TCS3414_ADDRESS, TCS3414_REG_TIMING, timeing)
		&& i2c_master_writeBytes2(TCS3414_ADDRESS, TCS3414_REG_GAIN, gain);
}


bool TCS3414_SetInterrupt(uint8_t interruptSource, uint8_t interruptControl)
{

	return i2c_master_writeBytes2(TCS3414_ADDRESS, TCS3414_REG_INT_SOURCE, interruptSource)
		&& i2c_master_writeBytes2(TCS3414_ADDRESS, TCS3414_REG_INT, interruptControl);
}

bool TCS3414_Init()
{
	if(!TCS3414_SetTimeing(TCS3414_INTEGRATION_TIME_12ms, TCS3414_GAIN_1|TCS3414_PRESCALER_4)){
		console_printf( "Failed to set timeings\n" );
		return false;
	}

	if(!TCS3414_SetInterrupt(TCS3414_INT_SOURCE_CLEAR, TCS3414_INTR_DISABLE)){
		console_printf( "Failed to set interrupt\n" );
		return false;
	}

	if(!i2c_master_writeBytes2(TCS3414_ADDRESS, TCS3414_REG_CTL, TCS3414_REG_CTL | TCS3414_CTL_DAT_INIITIATE)){
		console_printf( "Failed to enable ADC\n" );
		return false;
	}

	os_delay_us(15 * 1000);
	IS_ALREADY_INITED = true;
	return true;
}


static int do_i2c_tcs3414(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if((IS_ALREADY_INITED || TCS3414_Init()) && TCS3414_Read()){
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
