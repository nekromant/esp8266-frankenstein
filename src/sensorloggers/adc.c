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


static double get_adc(struct slogger_data_type *tp)
{
    return (double) system_adc_read() / 1024.0 * 1000.0;
}

FR_CONSTRUCTOR(register_adc_logger)
{
    console_printf("senslogger: Registering adc sensor\n");
    struct slogger_data_type *dt = malloc(sizeof(*dt));

    dt->type		= "Voltage";
    dt->description	= "ADC Voltage";
    dt->unit		= "mV";
    dt->get_current_value = get_adc;
    sensorlogger_instance_register_data_type(svclog_get_global_instance(), dt);
}
