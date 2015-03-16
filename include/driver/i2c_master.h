#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

#include "os_type.h"

#define I2C_MASTER_SDA_MUX (pin_mux[sda])
#define I2C_MASTER_SCL_MUX (pin_mux[scl])
#define I2C_MASTER_SDA_GPIO (pinSDA)
#define I2C_MASTER_SCL_GPIO (pinSCL)
#define I2C_MASTER_SDA_FUNC (pin_func[sda])
#define I2C_MASTER_SCL_FUNC (pin_func[scl])

#define I2C_SLEEP_TIME 10 

#if 0
#define I2C_MASTER_GPIO_SET(pin)  \
	gpio_output_set(1<<pin,0,1<<pin,0)

#define I2C_MASTER_GPIO_CLR(pin) \
	gpio_output_set(0,1<<pin,1<<pin,0)

#define I2C_MASTER_GPIO_OUT(pin,val) \
	if(val) I2C_MASTER_GPIO_SET(pin);\
	else I2C_MASTER_GPIO_CLR(pin)
#endif

#define I2C_MASTER_SDA_HIGH_SCL_HIGH()  \
	gpio_output_set(1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_HIGH_SCL_LOW()  \
	gpio_output_set(1<<I2C_MASTER_SDA_GPIO, 1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_LOW_SCL_HIGH()  \
	gpio_output_set(1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_LOW_SCL_LOW()  \
	gpio_output_set(0, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

void i2c_master_gpio_init(uint8 sda, uint8 scl);
void i2c_master_init(void);

#define i2c_master_wait	os_delay_us
void i2c_master_stop(void);
void i2c_master_start(void);
void i2c_master_setAck(uint8 level);
uint8 i2c_master_getAck(void);
uint8 i2c_master_readByte(void);
void i2c_master_writeByte(uint8 wrdata);

bool i2c_master_writeRegister(uint8 address, uint8 regaddr, uint8 wrdata);
uint16 i2c_master_readRegister16wait(uint8 address, uint8 regaddr, bool waitnack);
uint16 i2c_master_readRegister16(uint8 address, uint8 regaddr);
uint8 i2c_master_readRegister8(uint8 address, uint8 regaddr);

//TODO: bool i2c_master_writeBytes(uint8 address, uint8 value, ...);
bool i2c_master_writeBytes(uint8 address, uint8 *values, uint8 length);
bool i2c_master_writeBytes1(uint8 address, uint8 byte1);
bool i2c_master_writeBytes2(uint8 address, uint8 byte1, uint8 byte2);
bool i2c_master_readBytes(uint8 address, uint8 *values, uint8 length);
bool i2c_master_readUint8(uint8 address, uint8 regaddr, uint8 *value);
bool i2c_master_readUint16(uint8 address, uint8 regaddr, uint16 *value);
bool i2c_master_readSint16(uint8 address, uint8 regaddr, sint16 *value);

bool i2c_master_checkAck(void);
void i2c_master_send_ack(void);
void i2c_master_send_nack(void);

uint8 i2c_master_get_pinSDA();
uint8 i2c_master_get_pinSCL();

#endif
