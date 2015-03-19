/*
 * Module to sample a DHT-11 temperature / humidity sensor from the ESP8266 for the
 * Necromant's Frankenstein firmware by Andrew McDonnell <bugs@andrewmcdonnell.net>
 * 
 * See also:
 *   http://embedded-lab.com/blog/?p=4333
 *   http://www.micro4you.com/files/sensor/DHT11.pdf
 *   http://www.adafruit.com/datasheets/DHT22.pdf
 *
 * Presently hard-coded to GPIO #2 (accessible on the ESP-01 module)
 *
 * TODO: extend to cover the DHT-22 module
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

static void dht11_setup();
static int dht11_read_sensor(float* temperature, float* humidity);

static int do_dht11(int argc, const char* const* argv)
{
  float temperature, humidity; /* DHT-11 is only 8 bits decimal anyway, but allow for future extension to DHT-22 */
  int er;
  dht11_setup();
  er = dht11_read_sensor(&temperature, &humidity);
  switch (er) {
  case -1:
    console_printf( "No DHT-11 device found\n" );
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

static os_timer_t data_timer;
static volatile int dht11_data_timeout_flag;
static void ICACHE_FLASH_ATTR
dht11_data_timeout(void* unused)
{
  dht11_data_timeout_flag ++;
}

static uint8 dht11_read_byte(int gpio)
{
  /* assumes GPIO already set for reading */
  /* assumes we got the start sequence & configured the timer properly */
  uint16 bit;
  uint16 byte = 0;
  for (bit=0; bit<8; bit++)
  {
    /* wait until the bit transitions to high */
    while (!GPIO_INPUT_GET(gpio) && !dht11_data_timeout_flag);
    os_delay_us(40);
    /* Sample the next bit */
    if (GPIO_INPUT_GET(gpio)) { byte |= (1 << (7-bit)); }
    /* wait to return to low */
    /* if by some fluke the sensor is disconnected inside (40x~80us)==3.2ms... next line will hang without timer check */
    while(GPIO_INPUT_GET(gpio) && !dht11_data_timeout_flag);
  }
  return byte & 0xff;
}

static int dht11_read_sensor(float* temperature, float* humidity)
{
  /* For now, hard-coded to GPIO 2 */
  const int gpio=2;
  uint16 checksum;

  /* Starting as INPUT (thus pulled up to VCC) */
  os_delay_us(MINIMUM_POLL_DHT11);

  /* set a timeout for data from the sensor. */
  /* It actually only needs to be 80+80us but here we are using 5ms, so the same time can be used to detect a spurious disconnect */
  os_timer_disarm(&data_timer);
  os_timer_setfn(&data_timer, (os_timer_func_t *)dht11_data_timeout, NULL);
  dht11_data_timeout_flag = 0;

  /* hold low 25ms, then high 30us (see http://embedded-lab.com/blog/?p=4333 )*/
  GPIO_OUTPUT_SET(gpio, 0);
  os_delay_us(25000);
  os_timer_arm(&data_timer, 5, 0);  /* I Have tried values all the way down to 1 and the MCU still resets if no connection... */

  GPIO_OUTPUT_SET(gpio, 1);
  os_delay_us(30);

  /* now poll the input until it goes high, or 256 us (> 80 + 80us is minimum) has elasped (timeout) */
  GPIO_DIS_OUTPUT(gpio);
  while (!GPIO_INPUT_GET(gpio) && !dht11_data_timeout_flag);
  /* and we go low again... */
  while (GPIO_INPUT_GET(gpio) && !dht11_data_timeout_flag);
  if (dht11_data_timeout_flag) {
    os_timer_disarm(&data_timer);
    dbg("No sensor detected.\n");
    return -1;
  }
  /** WARNING **/
  /* WHEN NO CABLE IS CONNECTED: */
  /* PRESENTLY, THE MCU RESETS AFTER A SECOND OR SO; FOR SOME REASON THE TIMEOUT IS NOT WORKING :-( */

  uint8 b0 = dht11_read_byte(gpio);
  uint8 b1 = dht11_read_byte(gpio);
  uint8 b2 = dht11_read_byte(gpio);
  uint8 b3 = dht11_read_byte(gpio);
  uint8 b4 = dht11_read_byte(gpio);
  if (dht11_data_timeout_flag) {
    os_timer_disarm(&data_timer);
    dbg("Sensor interrupted!\n");
    return -1;
  }
  os_timer_disarm(&data_timer);

  checksum = b0 + b1 + b2 + b3;
  dbg("Data: %02x %02x %02x %02x %02x Checksum: %02x\n", b0, b1, b2, b3, b4, checksum);

  if (checksum != b4) {
    return -2;
  }

  *humidity = b0;
  *temperature = b2;
  return 0;
}

CONSOLE_CMD(dht11, 1, 1, 
      do_dht11, NULL, NULL, 
      "Read temperature and humidity from DHT11 sensor module. Presently hard-coded on GPIO #2. Warning: will reset MCU if no device present!"
      HELPSTR_NEWLINE "dht11"
  );

