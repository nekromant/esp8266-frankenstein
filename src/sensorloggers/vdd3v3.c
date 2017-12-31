#include "user_interface.h"
#include "missing.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h"
#include "microrl.h"
#include "console.h"
#include "main.h"
#include "helpers.h"
#include "lwip/sys.h"

#include <stdlib.h>
#include <stdlib.h>
#include <generic/macros.h>
#include <sensorlogger.h>


static double get_vdd3v3(struct slogger_data_type *tp)
{
    return (double) readvdd33();
}

FR_CONSTRUCTOR(register_vdd3v3_logger)
{
    console_printf("senslogger: Registering vdd3v3 sensor\n");
    struct slogger_data_type *dt = malloc(sizeof(*dt));

    dt->type		= "Voltage";
    dt->description	= "VDD3V3 Voltage";
    dt->unit		= "mV";
    dt->get_current_value = get_vdd3v3;
    sensorlogger_instance_register_data_type(svclog_get_global_instance(), dt);
}
