/*
 * Adaptation of Paul Stoffregen's One wire library to the ESP8266 and
 * Necromant's Frankenstein firmware by Erland Lewin <erland@lewin.nu>
 * 
 * Paul's original library site:
 *   http://www.pjrc.com/teensy/td_libs_OneWire.html
 * 
 * See also http://playground.arduino.cc/Learning/OneWire
 * 
 */
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/i2c_master.h" 
#include "driver/i2c_bmp180.h" 
#include "driver/i2c_sht21.h" 
#include "driver/i2c_bh1750.h" 
#include "driver/i2c_hmc5883l.h" 
#include "driver/i2c_tcs3414cs.h" 
#include "driver/i2c_pcf8591.h" 
#include "microrl.h"
#include "console.h"

#include <stdlib.h>
#include <generic/macros.h>


#ifdef CONFIG_CMD_I2C_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

/*
 * i2c init [SDA] [SCL]
 * i2c scan
 */

static void i2cdevice_init(uint8 addr, bool onlyread)
{
	console_printf( "\nFound device on 0x%02x. ", addr);
	switch (addr)
	{
		case 0x3c: 
			console_printf( "HMC5883L? " );
			if(onlyread || HMC5883_Init()){
				console_printf( "yes!\n" );
				if(HMC5883_Read()){
					console_printf( "X,Y,Z: %d %d %d\n", LAST_HMC5883_VECTOR.X, LAST_HMC5883_VECTOR.Y, LAST_HMC5883_VECTOR.Z);
					console_printf( "Compas: %d degress\n", HMC5883_ReadDegrees());
				}else{
					console_printf( "failed read value\n" );
				}
			}
			break;

		case 0x72: 
			console_printf( "TCS3414CS? " );
			if(onlyread || TCS3414_Init()){
				console_printf( "yes!\n" );
				if(TCS3414_Read()){
					console_printf( "RGBW: %d %d %d %d\n", LAST_TCS3414_COLOR.R, LAST_TCS3414_COLOR.G, LAST_TCS3414_COLOR.B, LAST_TCS3414_COLOR.W);
				}else{
					console_printf( "failed read value\n" );
				}
			}
			break;

		case 0x90: 
			console_printf( "PCF8591? " );
			if(onlyread || PCF8591_Init()){
				console_printf( "yes!\n" );
				if(PCF8591_Read()){
					console_printf( "A0-A3: %d %d %d %d\n", LAST_PCF8591_A[0], LAST_PCF8591_A[1], LAST_PCF8591_A[2], LAST_PCF8591_A[3]);
				}else{
					console_printf( "failed read value\n" );
				}
			}
			break;

		case 0x78: 
			console_printf( "OLED? " );
			break;

		case 0xA0: 
			console_printf( "EEPROM? " );
			//console_printf( "DS1337? " );
			break;

		case 0xA6: 
			console_printf( "ADXL345? " );
			break;

		case 0xB4: 
			console_printf( "MPR121? " );
			//console_printf( "MLX90614? " );
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

		case 0x46: //BV1750
			console_printf( "BV1750? " );
			if(onlyread || BH1750_Init(BH1750_ONE_TIME_HIGH_RES_MODE)){
				console_printf( "yes!\n" );
				if(BH1750_Read()){
					console_printf( "Light: %d lux\n", LAST_BH_LIGHT);
				}else{
					console_printf( "failed read value\n" );
				}
			}else{
				console_printf( "no. skip.\n" );
			}
			break;

		case 0x80: //SHT21
			console_printf( "SHT21? " );
			//console_printf( "HTU21? " );
			if(onlyread || SHT21_Init()){
				console_printf( "yes!\n" );
				if(SHT21_Read()){
					console_printf( "Temperature: %d\n", LAST_SHT_TEMPERATURE);
					console_printf( "Humidity: %d\n", LAST_SHT_HUMIDITY);
				}else{
					console_printf( "failed read value\n" );
				}
			}else{
				console_printf( "no. skip.\n" );
			}
			break;

		case 0xEE: //BMP180 
		//Check full capapibility with BMP085
			console_printf( "BMP180? " );
			if(onlyread || BMP180_Init()){
				console_printf( "yes!\n" );
				if(BMP180_Read()){
					console_printf( "Temperature: %lu\n", LAST_BMP_TEMPERATURE);
					console_printf( "Pressure: %lu\n", LAST_BMP_REAL_PRESSURE);
				}else{
					console_printf( "failed read value\n" );
				}
			}else{
				console_printf( "no. skip.\n" );
			}
			break;
		
		default:
			console_printf( "Unknown device." );

	}
}

static int do_i2c(int argc, const char* const* argv)
{
	if(strcmp(argv[1], "init") == 0){
		uint8 sda = 2;
		uint8 scl = 0;
		if(argc == 4){
			sda = atoi(argv[2]);
			scl = atoi(argv[3]);
		}

		console_printf( "Init i2c bus on %d:%d pins\n", sda, scl );
		i2c_master_gpio_init(sda, scl);

	
	} else

	if(strcmp(argv[1], "scan") == 0){
		
		bool onlyread = (argc > 2) && (strcmp(argv[2], "read") == 0);
		console_printf( "Enumeration i2c bus...\n" );
		for( uint8 addr = 0; addr < 128; addr ++ ){
			i2c_master_start();  			
			i2c_master_writeByte(addr << 1);
			bool ack = i2c_master_checkAck();
			i2c_master_stop();

			if(ack){
				i2cdevice_init(addr << 1, onlyread);

				wdt_feed();
			}
		}
		console_printf( "\n" );

	} else

	if(strcmp(argv[1], "read") == 0){
		if(argc < 3){
			console_printf( "Usage i2c read ADDRESS\n");
			return 0;
		}

		i2cdevice_init(strtol(argv[2], NULL, 16), true);

	} else

	{
		console_printf( "Unknown command %s", argv[1]);
	}

	return 0;
}

CONSOLE_CMD(i2c, 2, 4, 
		do_i2c, NULL, NULL, 
		"Execure i2c commands."
		HELPSTR_NEWLINE "i2c init SDA SLC"
		HELPSTR_NEWLINE "i2c scan"
);

