#include <stdlib.h>

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
//#include <lwip/netif.h>
//#include <lwip/app/dhcpserver.h>
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h"
#include "microrl.h"
#include "console.h"
#include "helpers.h"
#include "flash_layout.h"
#include <generic/macros.h>
#include <cJSON.h>

static void main_init_done(void)
{
	ets_uart_printf("Init completed!");

	cJSON *root,*fmt;
	root = cJSON_CreateObject();

	cJSON_AddItemToObject(root, "deviceId", cJSON_CreateString("inst->deviceId"));
	cJSON_AddItemToObject(root, "format", fmt = cJSON_CreateObject());
	cJSON_AddStringToObject(fmt, "type", "rect");
	cJSON_AddNumberToObject(fmt, "width", 1920);
	cJSON_AddNumberToObject(fmt, "height", 1080);
	cJSON_AddFalseToObject (fmt, "interlace");
	cJSON_AddNumberToObject(fmt, "frame rate", 24);
	char * rendered = cJSON_Print(root);
	ets_uart_printf(rendered);

}

void user_init()
{
	uart_init(0, 115200);
#if defined(CONFIG_ENABLE_SECOND_UART)
	uart_init(1, 115200);
#endif
	system_init_done_cb(main_init_done);
}
