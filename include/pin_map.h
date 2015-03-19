/*
 * There is not a 1:1 mapping between physical pin numbers numbers and the
 * various constants uses to control GPIO functionality.
 * The following globals are used to make this easy without adding masses of
 * overhead that a fully fledged abstraction library (such as that provided by
 * Arduino) might add:
 *
 * pin_mux[pin]    ->> returns the correct constant for pin 'pin' to use
 *                     as the first argument to PIN_FUNC_SELECT() and related
 *                     functions
 *
 * pin_func[pin]   ->> returns the correct constant for pin 'pin' to use
 *                     as the second argument to PIN_FUNC_SELECT()
 *
 * The above should suffice for the simplest GPIO use cases, that only need the
 * following functions:
 *
 * PIN_FUNC_SELECT(), PIN_PULLDWN_DIS(), PIN_PULLUP_DIS(),
 * PIN_PULLDWN_EN(), PIN_PULLUP_EN()
 *
 * The functions GPIO_DIS_OUTPUT(), GPIO_INPUT_GET(), GPIO_OUTPUT_SET() work
 * directly with the gpio number (e.g, 0, 2 on the ESP-01)
 *
 * pin_num[pin]    ->> Used to convert from a physical pin number 'pin' to the
 *                     common gpio number, if required.
 *
 * pin_int_type[pin] is used to return the GPIO_PIN_INTR_DISABLE state when
 * GPIO interrupts are enabled.
 *
 * The actual mappings are described in the file pin_map.c
 *
 * IMPORTANT
 * ---------
 * These mappings are indexed by physical PIN number (on the chip itself) not
 * by the commonly referred GPIO number.
 * In particular, the ESP01 which has GPIO0 and GPIO2 actually requires
 * pin==3 for GPIO0 and pin==4 for GPIO2.
 *
 * Note that the ESP SDK provides an additional macro, GPIO_ID_PIN() which in
 * theory should be wrapped around all 'gpio' numbers passed to the register
 * functions, however, this macro essentially resolves to a NO-OP so code will
 * work fine without it. It is not inconceivable however that there could be
 * a future hardware version increment that breaks this, so I guess omit this
 * at your own risk.
 *
 * To go in reverse, from a gpio number to a pin, use the macro XXXX
 * This would be needed when prompting a user for an actual gpio number,
 * which is the thing they are actually going to know...
 *
 * WARNING!
 * --------
 *
 * Check the user entered gpio number is in range...
 *
 * EXAMPLE
 * -------
 *
 *   uint8_t pin = XXX(GPIO_ID_PIN(gpio));
 *   if (pin >= GPIO_PIN_NUM) { printf("error...\n"); return; }
 *   PIN_FUNC_SELECT(pin_mux[pin], pin_func[pin]);
 *   PIN_PULLDWN_DIS(pin_mux[pin]);
 *   GPIO_DIS_OUTPUT(GPIO_ID_PIN(gpio));
 *
 */

#ifndef __PIN_MAP_H__
#define __PIN_MAP_H__

#include "c_types.h"
#include "user_config.h"
#include "gpio.h"

/* Total number of GPIOs */
#define GPIO_PIN_NUM 13

/* Max number of gpio numbers E 0..16, although there are gaps:
 * there is no gpio6, gpio7, gpio8 or gpio11, so the return 0xff from gpio_pin
 * Also, gpio0/pin0 may need to be treated separately...
 */
#define MAX_GPIO_USER_NUMBER 16

extern uint8_t gpio_pin[MAX_GPIO_USER_NUMBER+1];
extern uint8_t pin_num[GPIO_PIN_NUM];
extern uint8_t pin_func[GPIO_PIN_NUM];
extern uint32_t pin_mux[GPIO_PIN_NUM];
#ifdef GPIO_INTERRUPT_ENABLE
extern GPIO_INT_TYPE pin_int_type[GPIO_PIN_NUM];
#endif
#endif // #ifndef __PIN_MAP_H__
