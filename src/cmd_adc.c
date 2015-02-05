
#include <stdlib.h>

#include "c_types.h"
#include "console.h"

//#include "driver/adc.h"
#include "user_interface.h"
//uint16 system_adc_read(void);


static int do_adc (int argc, const char* argv[])
{
	console_printf("%d\n", system_adc_read());
	return 0;
}

CONSOLE_CMD(adc, 1, 1,
	    do_adc, NULL, NULL, 
	    "reads ADC"
);
