#include "driver/i2c_master.h"
#include "driver/i2c_ina219.h"
#include "console.h"

bool INA219_writereg16(uint8 reg, uint16 data)
{
	return i2c_master_writeBytes3(INA219_ADDRESS, reg, data, data >> 8);
}

bool INA219_Init()
{
	if (INA219_writereg16(INA219_REG_CALIBRATION, INA219_CALIBRATION_VALUE)){

		IS_ALREADY_INITED = true;
		return 1;
	}
	return 0;
}

bool INA219_Read()
{
	if (!INA219_writereg16(INA219_REG_CONFIG, (INA219_CONFIG_VALUE | INA219_MODE_SHUNT_BUS_TRIG)))
		return 0;

	i2c_master_readUint16(INA219_ADDRESS, INA219_REG_BUSVOLTAGE, &LAST_INA219_VOLTAGE);
	LAST_INA219_VOLTAGE = (LAST_INA219_VOLTAGE >> 3) * 4;

	i2c_master_readUint16(INA219_ADDRESS, INA219_REG_CURRENT, &LAST_INA219_CURRENT);
	if (((LAST_INA219_CURRENT) >> (15)) & 0x01) { //nagative
		LAST_INA219_CURRENT = 0;
		LAST_INA219_SHUNT_VOLTAGE = 0;
		LAST_INA219_POWER = 0;
	} else {
		LAST_INA219_CURRENT = LAST_INA219_CURRENT / 100;

		i2c_master_readUint16(INA219_ADDRESS, INA219_REG_SHUNTVOLTAGE, &LAST_INA219_SHUNT_VOLTAGE);
		LAST_INA219_SHUNT_VOLTAGE /= 1000;

		i2c_master_readUint16(INA219_ADDRESS, INA219_REG_POWER, &LAST_INA219_POWER);
		LAST_INA219_POWER /= 5;
	}

	if (!INA219_writereg16(INA219_REG_CONFIG, (INA219_CONFIG_VALUE | INA219_MODE_POWER_DOWN)))
		return 0;

	return 1;
}

static int do_i2c_ina219(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if((IS_ALREADY_INITED || INA219_Init()) && INA219_Read()){
			console_printf( argc == 1 ? "%d %d %d %d\n" : "Voltage: %d\nCurrent: %d\nShuntV: %d\nPower: %d\n", 
				LAST_INA219_VOLTAGE, LAST_INA219_CURRENT, LAST_INA219_SHUNT_VOLTAGE, LAST_INA219_POWER);
		}else{
			console_printf( "Failed to read value\n" );
		}
	} else

	if(strcmp(argv[1], "init") == 0){

		console_printf( INA219_Init() ? "Ok\n":"Failed\n" );
	} 

	return 0;
}

CONSOLE_CMD(i2c_ina219, 0, 2, 
		do_i2c_ina219, NULL, NULL, 
		"I2C current sensor INA219"
		HELPSTR_NEWLINE "i2c_ina219 init"
		HELPSTR_NEWLINE "i2c_ina219 [read]"
);
