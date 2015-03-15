#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "math.h"

#include "driver/i2c_master.h"
#include "driver/i2c_hmc5883l.h"
#include "console.h"

#ifdef CONFIG_CMD_HMC5883_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

static float mgPerDigit; 

uint16_t ICACHE_FLASH_ATTR 
HMC5883_ReadDegrees()
{
	float heading = atan2(LAST_HMC5883_VECTOR.X, LAST_HMC5883_VECTOR.Y);
	heading += (4.0 + (26.0 / 60.0)) / (180 / M_PI); 

	if (heading < 0)
	{
		heading += 2 * PI;
	} else
	if (heading > 2 * PI)
	{
		heading -= 2 * PI;
	} 

	return (heading * 180/M_PI);
}

bool ICACHE_FLASH_ATTR 
HMC5883_Read()
{
    LAST_HMC5883_VECTOR.X = ((float)i2c_master_readRegister16(HMC5883L_ADDRESS, HMC5883L_REG_OUT_X_M)) * mgPerDigit;
    LAST_HMC5883_VECTOR.Y = ((float)i2c_master_readRegister16(HMC5883L_ADDRESS, HMC5883L_REG_OUT_Y_M)) * mgPerDigit;
    LAST_HMC5883_VECTOR.Z = ((float)i2c_master_readRegister16(HMC5883L_ADDRESS, HMC5883L_REG_OUT_Z_M)) * mgPerDigit; 
	return true;
}

bool ICACHE_FLASH_ATTR 
HMC5883_SetConfig(hmc5883l_range_t range, hmc5883l_mode_t mode, hmc5883l_dataRate_t dataRate, hmc5883l_samples_t samples)
{
    switch(range)
    {
		case HMC5883L_RANGE_0_88GA:
			mgPerDigit = 0.073f;
			break;

		case HMC5883L_RANGE_1_3GA:
			mgPerDigit = 0.92f;
			break;

		case HMC5883L_RANGE_1_9GA:
			mgPerDigit = 1.22f;
			break;

		case HMC5883L_RANGE_2_5GA:
			mgPerDigit = 1.52f;
			break;

		case HMC5883L_RANGE_4GA:
			mgPerDigit = 2.27f;
			break;

		case HMC5883L_RANGE_4_7GA:
			mgPerDigit = 2.56f;
			break;

		case HMC5883L_RANGE_5_6GA:
			mgPerDigit = 3.03f;
			break;

		case HMC5883L_RANGE_8_1GA:
			mgPerDigit = 4.35f;
			break;

		default:
		    break;
   }

    if(!i2c_master_writeRegister(HMC5883L_ADDRESS, HMC5883L_REG_CONFIG_B, range << 5)){
		return false;
	}

    uint8_t value;
    value = i2c_master_readRegister8(HMC5883L_ADDRESS, HMC5883L_REG_MODE);
    value = (value & 0b11111100) | mode;
    if(!i2c_master_writeRegister(HMC5883L_ADDRESS, HMC5883L_REG_MODE, value)){
		return false;
	}

    value = i2c_master_readRegister8(HMC5883L_ADDRESS, HMC5883L_REG_CONFIG_A);
    value = (value & 0b11100011) | (dataRate << 2);
	if(!i2c_master_writeRegister(HMC5883L_ADDRESS, HMC5883L_REG_CONFIG_A, value)){
		return false;
	}

    value = i2c_master_readRegister8(HMC5883L_ADDRESS, HMC5883L_REG_CONFIG_A);
    value = (value & 0b10011111) | (samples << 5);
	if(!i2c_master_writeRegister(HMC5883L_ADDRESS, HMC5883L_REG_CONFIG_A, value)){
		return false;
	}

	return true;
}

bool ICACHE_FLASH_ATTR 
HMC5883_Init()
{
	if((i2c_master_readRegister8(HMC5883L_ADDRESS, HMC5883L_REG_IDENT_A) != 0x48)
	||(i2c_master_readRegister8(HMC5883L_ADDRESS, HMC5883L_REG_IDENT_B) != 0x34)
	||(i2c_master_readRegister8(HMC5883L_ADDRESS, HMC5883L_REG_IDENT_C) != 0x33)){
		return false;
	}

	if(!HMC5883_SetConfig(HMC5883L_RANGE_1_3GA, HMC5883L_CONTINOUS, HMC5883L_DATARATE_15HZ, HMC5883L_SAMPLES_1)){
		return false;
	}

	os_delay_us(15 * 1000);
	return true;
}

static int do_i2c_hmc5883(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if(HMC5883_Read()){
			if(argc != 1){
				console_printf( "Compas: %d degress\n", HMC5883_ReadDegrees());
				console_printf( "X,Y,Z: " );
			}
			console_printf( "%d %d %d\n", 
				(int)(LAST_HMC5883_VECTOR.X*100), (int)(LAST_HMC5883_VECTOR.Y*100), (int)(LAST_HMC5883_VECTOR.Z*100)
			);
		}else{
			console_printf( "failed read value\n" );
		}
	} else

	if(strcmp(argv[1], "init") == 0){

		console_printf( HMC5883_Init() ? "Ok\n":"Failed\n" );
	} 

	return 0;
}

CONSOLE_CMD(i2c_hmc5883, 0, 2, 
		do_i2c_hmc5883, NULL, NULL, 
		"I2C 3-Axis Digital Compass HMC5883"
		HELPSTR_NEWLINE "i2c_hmc5883 init"
		HELPSTR_NEWLINE "i2c_hmc5883 [read]"
);
