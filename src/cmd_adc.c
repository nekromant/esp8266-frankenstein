
#include <stdlib.h>

#include "c_types.h"
#include "console.h"
#include "user_interface.h"

static int do_adc (int argc, const char* const* argv)
{
	console_printf("%d\n", system_adc_read());
	return 0;
}

CONSOLE_CMD(adc, 1, 1,
	    do_adc, NULL, NULL, 
	    "Reads system ADC value"
);
