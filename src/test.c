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

extern int ets_uart_printf(const char *fmt, ...);


/* This is our blinky that toggles GPIO0 and 2. 
 * It is called by timer each second
 */
static volatile os_timer_t some_timer;
static int a = 0;
void some_timerfunc(void *arg)
{
	if (a) {
		gpio_output_set(BIT2, 0, BIT2, 0);
	}
	else
	{
		gpio_output_set(0, BIT2, BIT2, 0);
	}
	a++;
	a &= 0x1;
}


static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events);



/* Do a linear search. I doubt we'll ever have so many cmds it will 
 *  get slow. ever
 */
 


void ICACHE_FLASH_ATTR
user_init()
{

	wifi_set_opmode(0x0);
	/* Reset system timer */
	system_timer_reinit();

	/* Configure GPIO for our blinky */
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(BIT2, 0, BIT2, 0);
	gpio_output_set(BIT0, 0, BIT0, 0);

	wifi_status_led_install(0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);

	/* Reconfigure the timer to call our blinky once per second */
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);
	os_timer_arm(&some_timer, 1000, 1);
	os_delay_us(1000);
	uart_init(115200, 115200);
	console_init(32);

}
