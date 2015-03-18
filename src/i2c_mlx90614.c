#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "console.h"

#include "driver/i2c_master.h"
#include "driver/i2c_mlx90614.h"

#ifdef CONFIG_CMD_MLX90614_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

#ifdef CONFIG_USEFLOAT
float MLX90614_ReadTempFrom(uint8 reg)
{
	float result = i2c_master_readRegister16(MLX90614_ADDRESS, reg);
	return ((result / 50) - 273.15);
}
#else
sint16 MLX90614_ReadTempFrom(uint8 reg)
{

	uint8 data[3];
	data[0] = reg;
	if(i2c_master_readBytes(MLX90614_ADDRESS, data, 3)){
		uint16 temp = (data[1] << 8) | data[0];
		return ((temp / 50.0) - 273.15)*100;
	}
	return 0;
}
#endif

bool MLX90614_Read()
{
	LAST_MLX90614_AMBIENT_TEMPERATURE = MLX90614_ReadTempFrom(MLX90614_TA);
	LAST_MLX90614_OBJECT_TEMPERATURE = MLX90614_ReadTempFrom(MLX90614_TOBJ1);
	return true;
}


bool MLX90614_Init()
{
#ifdef CONFIG_CMD_MLX90614_DEBUG
	console_printf("Sensor ID: %02x %02x %02x %02x\n",
		i2c_master_readRegister8(MLX90614_ADDRESS, MLX90614_ID1),
		i2c_master_readRegister8(MLX90614_ADDRESS, MLX90614_ID2),
		i2c_master_readRegister8(MLX90614_ADDRESS, MLX90614_ID3),
		i2c_master_readRegister8(MLX90614_ADDRESS, MLX90614_ID4)
	);
#endif

	IS_ALREADY_INITED = true;
	return true;
}

static int do_i2c_mlx90614(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if((IS_ALREADY_INITED || MLX90614_Init()) && MLX90614_Read()){
			console_printf( argc == 1 ? "%d %d\n" : "Ambient: %d C\nObject: %d C\n", 
#ifdef CONFIG_USEFLOAT
				(int)(LAST_MLX90614_AMBIENT_TEMPERATURE*100), 
				(int)(LAST_MLX90614_OBJECT_TEMPERATURE*100)
#else
				LAST_MLX90614_AMBIENT_TEMPERATURE,
				LAST_MLX90614_OBJECT_TEMPERATURE
#endif
			);
		}else{
			console_printf( "Failed to read value\n" );
		}
	} else

	if(strcmp(argv[1], "init") == 0){

		console_printf( MLX90614_Init() ? "Ok\n":"Failed\n" );
	} 

	return 0;
}

CONSOLE_CMD(i2c_mlx90614, 0, 2, 
		do_i2c_mlx90614, NULL, NULL, 
		"I2C non-contact thermopile sensor"
		HELPSTR_NEWLINE "i2c_mlx90614 init"
		HELPSTR_NEWLINE "i2c_mlx90614 [read]"
);
