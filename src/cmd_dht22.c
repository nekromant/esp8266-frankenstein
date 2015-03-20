#include "driver/dht22.h"
#include "console.h"
#include "pin_map.h"

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

#ifdef CONFIG_DHT22_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

static DHT_Type current_dht_type;
static uint8 DHT_Pin;

bool DHT_Read(void)
{
	uint8 data[5];
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	
	GPIO_OUTPUT_SET(DHT_Pin, 1);	// high for 250ms
	os_delay_us(250*1000);
	GPIO_OUTPUT_SET(DHT_Pin, 0);	// low for 20ms
	os_delay_us(20*1000);
	GPIO_OUTPUT_SET(DHT_Pin, 1);	// high for 40ns
	os_delay_us(40);
	GPIO_DIS_OUTPUT(DHT_Pin);

	uint8 i = 0;
	uint8 bits = 0;
	bool laststate = 1;
	do{
		uint8 counter = 0;
		while (GPIO_INPUT_GET(DHT_Pin) == laststate)
		{
			os_delay_us(1);
			if (++counter > DHT_READTIMEOUT){
				dbg("DHT reading timeout on bit %d.\n", bits);
				return false;
			}
		}

		laststate = GPIO_INPUT_GET(DHT_Pin);

		// skip first 3 bits
		if ((i >= 4) && (i%2 == 0)) {
			data[bits/8] <<= 1;
			if (counter > DHT_BREAKTIME)
				data[bits/8] |= 1;
			if(++bits >= 40) break;
		}

		wdt_feed();
	}while(++i);

	if (bits >= 40) {
		uint8 checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
		if (data[4] == checksum) {
			if(current_dht_type == DHT11) {
				LAST_DHT_HUMIDITY = data[0];
				LAST_DHT_TEMPERATURE = data[2];
			} else {
				LAST_DHT_HUMIDITY = data[0] * 256 + data[1];
				LAST_DHT_TEMPERATURE = (data[2] & 0x7f) * 256 + data[3];
				if(data[2] & 0x80) LAST_DHT_TEMPERATURE = -LAST_DHT_TEMPERATURE;
			}
			return true;
		}

		dbg("Checksum mismatch. Expected %d got %d. Data: %02x %02x %02x %02x\n", data[4], checksum, data[0], data[1], data[2], data[3]);
	} else {
		dbg("Want to read 40 bits, got: %d\n", bits);
	}

	return false;
}


bool DHT_Init(uint8 gpiopin, DHT_Type dht_type)
{
	if(!is_valid_gpio_pin(gpiopin)){
		console_printf( "Invalid GPIO pin number provided\n" );
		return false;
	}

	DHT_Pin = gpiopin;
	current_dht_type = dht_type;

	PIN_FUNC_SELECT(pin_mux[DHT_Pin], pin_func[DHT_Pin]);
	PIN_PULLUP_EN(pin_mux[DHT_Pin]);
	IS_ALREADY_INITED = true;

	return true;
}

static int do_dht22(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if(!IS_ALREADY_INITED){
			console_printf( "Sensor does not inited\n" );
			return false;
		}

		if(DHT_Read()){
			console_printf( argc == 1 ? "%d %d\n" : "Temperature: %d C\nHumidity: %d %%\n", 
				LAST_DHT_TEMPERATURE,
				LAST_DHT_HUMIDITY
			);
		}else{
			console_printf( "Failed to read value\n" );
		}
	} else

	if(strcmp(argv[1], "init") == 0){

		if(argc < 3){
			console_printf( "Usage: dht22 init <gpio> [11|22]\n" );
			return false;
		}

		uint8 pinnum = atoi(argv[2]);
		uint8 sensortype = (argc == 3)?DHT22:(argv[3][0]=='1'?DHT11:DHT22);
		
		console_printf( DHT_Init(pinnum, sensortype) ? "Ok\n":"Failed\n" );
	} 

	return 0;
}

CONSOLE_CMD(dht22, 0, 4, 
		do_dht22, NULL, NULL, 
		"DHT22 humidity sensor"
		HELPSTR_NEWLINE "dht22 init <gpio> [11|22]"
		HELPSTR_NEWLINE "dht22 [read]"
);
