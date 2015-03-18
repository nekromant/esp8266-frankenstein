/*
 * Adaptation of Mark Ruys Arduino-DHT library to the ESP8266 and
 * Necromant's Frankenstein firmware by Andrew McDonnell <bugs@andrewmcdonnell.net>
 * 
 * Mark Ruys <mark@paracas.nl> original source code:
 *   An Arduino library for reading the DHT family of temperature and humidity sensors.
 *   https://github.com/markruys/arduino-DHT
 *
 * See also:
 *   http://www.micro4you.com/files/sensor/DHT11.pdf
 *   http://www.adafruit.com/datasheets/DHT22.pdf
 *
 * Presently hard-coded to GPIO #2 (accessible on the ESP-01 module)
 */
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h" 
#include "microrl.h"
#include "console.h"

#include <stdlib.h>
#include <math.h>
#include <generic/macros.h>

#ifdef CONFIG_CMD_DHT11_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

#define MINIMUM_POLL_DHT11 1000000   /* Minimum number of microseconds between polling the DHT11 (1Hz duty cycle) */
#define MINIMUM_POLL_DHT22 2000000   /* Minimum number of microseconds between polling the DHT22 (1/2Hz duty cycle) */

static void dht11_setup();
static int dht11_read_sensor(float* temperature, float* humidity);

static int do_dht11(int argc, const char* const* argv)
{
  float temperature, humidity;
  int er;
  dht11_setup();
  er = dht11_read_sensor(&temperature, &humidity);
  switch (er) {
  case -1:
    console_printf( "No DHT family device found\n" );
    break;
  case -2:
    console_printf( "Checksum error\n" );
    break;
  default:
    /* hmm.  seems printf(%f) doesnt work... */
    console_printf( "Temperature: %d.%d Celsius, Humidity: %d\n",
      (int)floorf(temperature), (int)(10.F*(temperature - floorf(temperature))), (int)floor(humidity));
    break;
  }
  return er;
}

/* Setup to try and read a DHT11 or DHT22 on the specified gpio pin (currently, hard-coded to 2) */
void dht11_setup(int gpio)
{
  /* set gpio2 as gpio pin, with pullup enabled. */
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  PIN_PULLDWN_DIS(PERIPHS_IO_MUX_GPIO2_U);
  PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U); /* we expect an external pullup to Vcc */
  GPIO_DIS_OUTPUT(2);

  dbg("Preparing to read a DHT-family device using gpio 2\n");
}

static int dht11_read_sensor(float* temperature, float* humidity)
{
  /* For now, hard-coded to GPIO 2 */
  const int gpio=2;

  /* Start off as input... */
  GPIO_DIS_OUTPUT(gpio);

  /* Send a start signal.
   * We try and auto-detect the device as follows:
   * (1) first, try as if it is a DHT-22
   * (2) If that times out assume we have a DHT-11 (if there is a device...)
   * (3) If still timeout then assume error
   */

  /* We need to search in this order... */
  enum { MODEL_DHT22 = 0, MODEL_DHT11, MODEL_NONE };

  uint16 readTemperature = 0;
  uint16 readHumidity = 0;

  uint8 model;
  uint16 data = (uint16)-1;
  uint8 checksum;
  for (model=0; model != MODEL_NONE; model++)
  {
    int edge;
    /* Start with output at Vcc */
    GPIO_OUTPUT_SET(gpio, 1);

    switch (model) {
    case MODEL_DHT11: 
      dbg("Trying model DTH11\n"); break;
      os_delay_us(MINIMUM_POLL_DHT11);
      /* hold low 20ms, then high 30us (see http://embedded-lab.com/blog/?p=4333 )*/
      GPIO_OUTPUT_SET(gpio, 0);
      os_delay_us(20000);
      GPIO_OUTPUT_SET(gpio, 1);
      os_delay_us(30);
      break;
    case MODEL_DHT22: 
      dbg("Trying model DTH22\n"); break;
      GPIO_OUTPUT_SET(gpio, 1);
      os_delay_us(MINIMUM_POLL_DHT22);
      GPIO_OUTPUT_SET(gpio, 0);
      os_delay_us(800);
      GPIO_OUTPUT_SET(gpio, 1);
      os_delay_us(800);
    }

    /* Read 83 edges after output was high:
     *   First a FALLING, RISING, and FALLING edge for the start bit
     *   Then 40 bits: RISING and then a FALLING edge per bit
     *   To keep things simple, we accept any HIGH or LOW reading if it's max 85 usecs (~45 x 2us retries) long
     * First 16 bits == humidity
     * Next 16 bits == temperature
     */

    GPIO_DIS_OUTPUT(gpio);
    data = 0;
    // dbg("Read loop\n");
    for (edge=-3 ; edge < 2 * 40; edge++ ) { /* LSB bit: 1 0 1 0 1 0 1 0.... */
      uint8 retries;
      // dbg("edge=%d\n", (int)edge);
      for (retries = 45; retries != 0; retries --)  {
        // For odd edges we need to read 0, for even edges 1
        if (GPIO_INPUT_GET(gpio) == ((edge & 1)?0:1)) break;
        os_delay_us(2);
      }
      //dbg("edge=%d retries=%d\n", edge, 45-(int)retries); 

      /* Sometimes you just need to use goto */
      if (retries == 0) goto read_failed;
      if (edge >= 0 && (edge & 1) ) {
        /* Shift in the bits so last read bit is LSB */
        data <<= 1;
        /* Low is max 30 us & high least 68 us */
        if (retries * 2 > 30) {
          data |= 1;
        }
      }

      switch (edge) {
      case 31: readHumidity = data; break;
      case 63: readTemperature = data; data = 0; break;
      /* On exit from loop data will contain checksum byte */
      }
    }
    dbg("All edges\n");
    break;
read_failed:
    switch (model) {
    case MODEL_DHT11: 
      dbg("It is not a DTH11. edge=%d\n", edge); break;
    case MODEL_DHT22: 
      dbg("It is not a DTH22. edge=%d\n", edge); break;
    default:
      dbg("assert!\n"); break;
    }
    dbg("Return to output mode\n");
    GPIO_OUTPUT_SET(gpio, 1);
  }
  GPIO_OUTPUT_SET(gpio, 1);

  if (model == MODEL_NONE) {
    dbg("No detect\n");
    return -1;
  }

  checksum = (uint8)(((uint8)readHumidity) + (readHumidity >> 8) + ((uint8)readTemperature) + (readTemperature >> 8));

  dbg("Data: %.4x %.4x %.2x Checksum: %2x\n", readHumidity, readTemperature, data, checksum);

  if ( checksum != data ) {
    return -2;
  }

  switch (model) {
  case MODEL_DHT11: 
    *humidity = readHumidity >> 8;
    *temperature = readTemperature >> 8;
    break;
  case MODEL_DHT22:
    /* untested: I dont have a DHT22 to try */
    *humidity = readHumidity * 0.1;
    if ( readTemperature & 0x8000 ) {
      readTemperature = -(sint16)(readTemperature & 0x7FFF);
    }
    *temperature = ((sint16)readTemperature) * 0.1;
    break;
  }
  return 0;
}

CONSOLE_CMD(dht11, 1, 1, 
      do_dht11, NULL, NULL, 
      "Read temperature and humidity from DHT11 / DHT22 sensor module."
      HELPSTR_NEWLINE "dht11 <gpio>"
  );

