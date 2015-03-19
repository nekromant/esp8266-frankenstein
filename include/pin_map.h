
#ifndef __PIN_MAP_H__
#define __PIN_MAP_H__

#include "c_types.h"
#include "user_config.h"
#include "gpio.h"

#define GPIO_PIN_NUM 17
#define GPIO_PIN_FUNC_INVALID (uint8_t)(-1)
#define GPIO_PIN_MUX_INVALID (uint32_t)(-1)

bool is_valid_gpio_pin(uint8 gpiopin);

//extern uint8_t pin_num[GPIO_PIN_NUM];
extern uint8_t pin_func[GPIO_PIN_NUM];
extern uint32_t pin_mux[GPIO_PIN_NUM];
#ifdef GPIO_INTERRUPT_ENABLE
extern GPIO_INT_TYPE pin_int_type[GPIO_PIN_NUM];
#endif
#endif // #ifndef __PIN_MAP_H__
