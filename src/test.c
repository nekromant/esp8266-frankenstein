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
#include <generic/macros.h>

extern int ets_uart_printf(const char *fmt, ...);



const char *initcmds[] = {
//	"iwmode AP\r",
//	"apconfig dummy OPEN none\r",
};




void ICACHE_FLASH_ATTR
user_init()
{
	wifi_set_opmode(0x1);
	/* Reset system timer */
	/* Configure GPIO for our blinky */
//	wifi_status_led_install(0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);

	/* Reconfigure the timer to call our blinky once per second */
//	os_timer_disarm(&some_timer);
//	os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);
//	os_timer_arm(&some_timer, 1000, 1);
//	os_delay_us(1000);
	uart_init(115200, 115200);
	console_init(32);
	
	int i; 
	for (i=0; i<ARRAY_SIZE(initcmds); i++)
		console_exec(initcmds[i]);
	
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, BIT2, BIT2, 0);
	gpio_output_set(0, BIT0, BIT0, 0);

}
