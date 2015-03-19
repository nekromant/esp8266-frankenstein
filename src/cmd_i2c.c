#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/i2c_master.h" 
#include "microrl.h"
#include "console.h"

#include <stdlib.h>
#include <generic/macros.h>


#ifdef CONFIG_CMD_I2C_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

static void i2cdevice_enum(uint8 addr)
{
	console_printf( "\nFound device on 0x%02x. ", addr);
	switch (addr)
	{
		case 0x3c: 
			console_printf( "HMC5883L? " );
			break;

		case 0x72: 
			console_printf( "TCS3414CS? " );
			break;

		case 0x82: 
			console_printf( "INA219? " );
			break;

		case 0x90: 
			console_printf( "PCF8591? " );
			break;

		case 0x78: 
		//case 0x7A: 
			console_printf( "SSD1306 OLED? " );
			break;

		case 0xA0: 
			console_printf( "EEPROM? " );
			console_printf( "DS1337? " );
			break;

		case 0xA6: 
			console_printf( "ADXL345? " );
			break;

		case 0xB4: 
			console_printf( "MPR121? " );
			console_printf( "MLX90614? " );
			break;

		case 0xAE: 
			console_printf( "DS3231SN? " );
			break;

		case 0xD2: 
			console_printf( "L3G4200D? " );
			break;

		case 0xD0: 
			console_printf( "DS1337? " );
			break;

		case 0x46: 
			console_printf( "BH1750? " );
			break;

		case 0x80: 
			console_printf( "SHT21? " );
			console_printf( "HTU21? " );
			break;

		case 0xEE: 
			console_printf( "BMP180? " );
			console_printf( "BMP085? " );
			break;
		
		default:
			console_printf( "Unknown device." );

	}
}

static int do_i2c(int argc, const char* const* argv)
{
	if(strcmp(argv[1], "init") == 0){

		uint8 sda = 0;
		uint8 scl = 2;
		if(argc == 4){
			sda = atoi(argv[2]);
			scl = atoi(argv[3]);
		}

		console_printf( "Init i2c bus on GPIO%d:GPIO%d ", sda, scl );
		if(i2c_master_gpio_init(sda, scl)){
			console_printf( "ok\n" );
		}else{
			console_printf( "failed\n" );
		}

	} else

	if(strcmp(argv[1], "scan") == 0){
		
		console_printf( "Enumeration i2c bus...\n" );
		for( uint8 addr = 0; addr < 128; addr ++ ){
			i2c_master_start();  			
			i2c_master_writeByte(addr << 1);
			bool ack = i2c_master_checkAck();
			i2c_master_stop();

			if(ack){
				i2cdevice_enum(addr << 1);
				wdt_feed();
			}
		}
		console_printf( "\n" );

	} else

	{
		console_printf( "Unknown command %s", argv[1]);
	}

	return 0;
}

CONSOLE_CMD(i2c, 2, 4, 
		do_i2c, NULL, NULL, 
		"I2C bus config. Default SDA and SCL is GPIO0 and 2"
		HELPSTR_NEWLINE "i2c init [SDA] [SCL]"
		HELPSTR_NEWLINE "i2c scan"
);

