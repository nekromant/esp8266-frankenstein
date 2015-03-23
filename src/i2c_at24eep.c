#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "console.h"

#include "driver/i2c_master.h"
#include "driver/i2c_at24eep.h"

#ifdef CONFIG_CMD_AT24EEP_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

bool AT24EEP_Write(/*uint8 address, uint8 length, uint8 *values*/)
{
	//TODO push address to values head
	//return i2c_master_writeBytes(AT24EEP_ADDRESS, values, length+1);
	return true;
}

bool AT24EEP_Read(/*uint8 address, uint8 length, uint8 *values*/)
{
	//TODO push address to values head
	//return i2c_master_readBytes(AT24EEP_ADDRESS, values, length+1);
	return true;
}

bool AT24EEP_Init()
{
	//TODO detect flash size by reading max address
	uint8 addr = 0x80;
	uint8 temp;
	while(i2c_master_readUint8(AT24EEP_ADDRESS, addr, &temp)){
		console_printf( "Addr 0x%02x == %d \n", addr, temp);
		addr *= 2;
	}
	console_printf( "Failed to read addr 0x%02x\n", addr);

	IS_ALREADY_INITED = true;
	return true;
}

static int do_i2c_at24eep(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if(!IS_ALREADY_INITED && !AT24EEP_Init()){
			console_printf( "Failed\n" );
			return 0;
		}

		if(AT24EEP_Read()){
			//console_printf( argc == 1 ? "%d\n" : "Light: %d lux\n", LAST_BH_LIGHT);
		}else{
			console_printf( "Failed to read value\n" );
		}

	} else

	if(strcmp(argv[1], "write") == 0){

		if(AT24EEP_Write()){
		}else{
			console_printf( "Failed to write value\n" );
		}

	} else

	if(strcmp(argv[1], "init") == 0){

		console_printf( AT24EEP_Init() ? "Ok\n":"Failed\n" );
	} 

	return 0;
}

CONSOLE_CMD(i2c_at24eep, 0, 2, 
		do_i2c_at24eep, NULL, NULL, 
		"I2C EEPROM serial memory"
		HELPSTR_NEWLINE "i2c_at24eep init"
		HELPSTR_NEWLINE "i2c_at24eep [read]"
);
