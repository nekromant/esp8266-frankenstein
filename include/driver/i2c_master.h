#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

#include "os_type.h"
#include "user_interface.h"

#define I2C_SLEEP_TIME 5

bool i2c_master_gpio_init(uint8 sda, uint8 scl);
void i2c_master_init(void);

#define i2c_master_wait	os_delay_us
void i2c_master_stop(void);
void i2c_master_start(void);
void i2c_master_setAck(uint8 level);
uint8 i2c_master_getAck(void);
uint8 i2c_master_readByte(void);
void i2c_master_writeByte(uint8 wrdata);

//TODO: bool i2c_master_writeBytes(uint8 address, uint8 value, ...);
bool i2c_master_writeBytes(uint8 address, uint8 *values, uint8 length);
bool i2c_master_writeBytes1(uint8 address, uint8 byte1);
bool i2c_master_writeBytes2(uint8 address, uint8 byte1, uint8 byte2);
bool i2c_master_writeBytes3(uint8 address, uint8 byte1, uint8 byte2, uint8 byte3);
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
