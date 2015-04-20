/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: i2c_master.c
 *
 * Description: i2c master API
 *
 * Modification history:
 *	 2014/3/12, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

#include "driver/i2c_master.h"

#include "pin_map.h"

#ifdef CONFIG_I2C_MASTER_DEBUG
#include "console.h"
#endif

LOCAL uint8 m_nLastSDA;
LOCAL uint8 m_nLastSCL;

LOCAL uint8 pinSDA = 2;
LOCAL uint8 pinSCL = 0;

/******************************************************************************
 * FunctionName : i2c_master_setDC
 * Description  : Internal used function -
 *					set i2c SDA and SCL bit value for half clk cycle
 * Parameters   : uint8 SDA
 *				uint8 SCL
 * Returns	  : NONE
*******************************************************************************/
LOCAL void i2c_master_setDC(uint8 SDA, uint8 SCL)
{
	SDA	&= 0x01;
	SCL	&= 0x01;
	m_nLastSDA = SDA;
	m_nLastSCL = SCL;

	if ((0 == SDA) && (0 == SCL)) {
		gpio_output_set(0, 1<<pinSDA | 1<<pinSCL, 1<<pinSDA | 1<<pinSCL, 0);
	} else if ((0 == SDA) && (1 == SCL)) {
		gpio_output_set(1<<pinSCL, 1<<pinSDA, 1<<pinSDA | 1<<pinSCL, 0);
	} else if ((1 == SDA) && (0 == SCL)) {
		gpio_output_set(1<<pinSDA, 1<<pinSCL, 1<<pinSDA | 1<<pinSCL, 0);
	} else {
		gpio_output_set(1<<pinSDA | 1<<pinSCL, 0, 1<<pinSDA | 1<<pinSCL, 0);
	}
}

/******************************************************************************
 * FunctionName : i2c_master_getDC
 * Description  : Internal used function -
 *					get i2c SDA bit value
 * Parameters   : NONE
 * Returns	  : uint8 - SDA bit value
*******************************************************************************/
LOCAL uint8 i2c_master_getDC(void)
{
	return GPIO_INPUT_GET(GPIO_ID_PIN(pinSDA));
}

/******************************************************************************
 * FunctionName : i2c_master_getCL
 * Description  : Internal used function -
 *					get i2c SCL bit value
 * Parameters   : NONE
 * Returns	  : uint8 - SCL bit value
*******************************************************************************/
LOCAL uint8 i2c_master_getCL(void)
{
	uint8 scl_out;
	scl_out = GPIO_INPUT_GET(GPIO_ID_PIN(pinSCL));
	return scl_out;
}

/******************************************************************************
 * FunctionName : i2c_master_init
 * Description  : initilize I2C bus to enable i2c operations
 * Parameters   : NONE
 * Returns	  : NONE
*******************************************************************************/
void i2c_master_init(void)
{
	uint8 i;

	i2c_master_setDC(1, 0);
	i2c_master_wait(I2C_SLEEP_TIME);

	// when SCL = 0, toggle SDA to clear up
	i2c_master_setDC(0, 0) ;
	i2c_master_wait(I2C_SLEEP_TIME);
	i2c_master_setDC(1, 0) ;
	i2c_master_wait(I2C_SLEEP_TIME);

	// set data_cnt to max value
	for (i = 0; i < 28; i++) {
		i2c_master_setDC(1, 0);
		i2c_master_wait(I2C_SLEEP_TIME);// sda 1, scl 0
		i2c_master_setDC(1, 1);
		i2c_master_wait(I2C_SLEEP_TIME);// sda 1, scl 1
	}

	// reset all
	i2c_master_stop();
	return;
}

uint8 i2c_master_get_pinSDA(){
	return pinSDA;
}

uint8 i2c_master_get_pinSCL(){
	return pinSCL;
}

/******************************************************************************
 * FunctionName : i2c_master_gpio_init
 * Description  : config SDA and SCL gpio to open-drain output mode,
 *				mux and gpio num defined in i2c_master.h
 * Parameters   : uint8 sda and scl pin numbers
 * Returns	  : bool, true if init okay
*******************************************************************************/
bool i2c_master_gpio_init(uint8 sda, uint8 scl)
{
	if((sda > GPIO_PIN_NUM) || (pin_func[sda] == GPIO_PIN_FUNC_INVALID)){
		return false;
	}

	if((scl > GPIO_PIN_NUM) || (pin_func[scl] == GPIO_PIN_FUNC_INVALID)){
		return false;
	}

	pinSDA = sda;
	pinSCL = scl;

	ETS_GPIO_INTR_DISABLE() ;
//	ETS_INTR_LOCK();

	PIN_FUNC_SELECT(pin_mux[sda], pin_func[sda]);
	PIN_FUNC_SELECT(pin_mux[scl], pin_func[scl]);

	GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(sda)), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(sda))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)); //open drain;
	GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << sda));
	GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(scl)), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(scl))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)); //open drain;
	GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << scl));

	i2c_master_setDC(1, 1);

	ETS_GPIO_INTR_ENABLE() ;
//	ETS_INTR_UNLOCK();

	i2c_master_init();
	return true;
}

/******************************************************************************
 * FunctionName : i2c_master_start
 * Description  : set i2c to send state
 * Parameters   : NONE
 * Returns	  : NONE
*******************************************************************************/
void i2c_master_start(void)
{
	i2c_master_setDC(1, m_nLastSCL);
	i2c_master_wait(I2C_SLEEP_TIME);
	i2c_master_setDC(1, 1);
	i2c_master_wait(I2C_SLEEP_TIME);// sda 1, scl 1
	i2c_master_setDC(0, 1);
	i2c_master_wait(I2C_SLEEP_TIME);// sda 0, scl 1
}

/******************************************************************************
 * FunctionName : i2c_master_stop
 * Description  : set i2c to stop sending state
 * Parameters   : NONE
 * Returns	  : NONE
*******************************************************************************/
void i2c_master_stop(void)
{
	i2c_master_wait(I2C_SLEEP_TIME);

	i2c_master_setDC(0, m_nLastSCL);
	i2c_master_wait(I2C_SLEEP_TIME);// sda 0
	i2c_master_setDC(0, 1);
	i2c_master_wait(I2C_SLEEP_TIME);// sda 0, scl 1
	i2c_master_setDC(1, 1);
	i2c_master_wait(I2C_SLEEP_TIME);// sda 1, scl 1
}

/******************************************************************************
 * FunctionName : i2c_master_setAck
 * Description  : set ack to i2c bus as level value
 * Parameters   : uint8 level - 0 or 1
 * Returns	  : NONE
*******************************************************************************/
void i2c_master_setAck(uint8 level)
{
	i2c_master_setDC(m_nLastSDA, 0);
	i2c_master_wait(I2C_SLEEP_TIME);
	i2c_master_setDC(level, 0);
	i2c_master_wait(I2C_SLEEP_TIME);// sda level, scl 0
	i2c_master_setDC(level, 1);
	i2c_master_wait(8);// sda level, scl 1
	i2c_master_setDC(level, 0);
	i2c_master_wait(I2C_SLEEP_TIME);// sda level, scl 0
	i2c_master_setDC(1, 0);
	i2c_master_wait(I2C_SLEEP_TIME);
}

/******************************************************************************
 * FunctionName : i2c_master_getAck
 * Description  : confirm if peer send ack
 * Parameters   : NONE
 * Returns	  : uint8 - ack value, 0 or 1
*******************************************************************************/
uint8 i2c_master_getAck(void)
{
	uint8 retVal;
	i2c_master_setDC(m_nLastSDA, 0);
	i2c_master_wait(I2C_SLEEP_TIME);
	i2c_master_setDC(1, 0);
	i2c_master_wait(I2C_SLEEP_TIME);
	i2c_master_setDC(1, 1);
	i2c_master_wait(I2C_SLEEP_TIME);

	retVal = i2c_master_getDC();
	i2c_master_wait(I2C_SLEEP_TIME);
	i2c_master_setDC(1, 0);
	i2c_master_wait(I2C_SLEEP_TIME);

	return retVal;
}

/******************************************************************************
* FunctionName : i2c_master_checkAck
* Description  : get dev response
* Parameters   : NONE
* Returns	  : true : get ack ; false : get nack
*******************************************************************************/
bool i2c_master_checkAck(void)
{
	if(i2c_master_getAck()){
		return FALSE;
	}else{
		return TRUE;
	}
}

/******************************************************************************
* FunctionName : i2c_master_send_ack
* Description  : response ack
* Parameters   : NONE
* Returns	  : NONE
*******************************************************************************/
void i2c_master_send_ack(void)
{
	i2c_master_setAck(0x0);
}
/******************************************************************************
* FunctionName : i2c_master_send_nack
* Description  : response nack
* Parameters   : NONE
* Returns	  : NONE
*******************************************************************************/
void i2c_master_send_nack(void)
{
	i2c_master_setAck(0x1);
}

/******************************************************************************
 * FunctionName : i2c_master_readByte
 * Description  : read Byte from i2c bus
 * Parameters   : NONE
 * Returns	  : uint8 - readed value
*******************************************************************************/
uint8 i2c_master_readByte(void)
{
	uint8 retVal = 0;
	uint8 k, i;

	i2c_master_setDC(1, m_nLastSCL);

	for (i = 0; i < 8; i++) {
		i2c_master_wait(I2C_SLEEP_TIME);
		i2c_master_setDC(1, 0);
		i2c_master_wait(I2C_SLEEP_TIME);// sda 1, scl 0
		i2c_master_setDC(1, 1);
		while(i2c_master_getCL()==0)
			;		// clock stretch
		i2c_master_wait(I2C_SLEEP_TIME);// sda 1, scl 1

		k = i2c_master_getDC();
		i2c_master_wait(I2C_SLEEP_TIME);

		//if (i == 7) {
		//	i2c_master_wait(5);   ////
		//}

		k <<= (7 - i);
		retVal |= k;
	}

	i2c_master_setDC(m_nLastSDA, 0);
	i2c_master_wait(I2C_SLEEP_TIME);// sda 1, scl 0

	return retVal;
}

/******************************************************************************
 * FunctionName : i2c_master_writeByte
 * Description  : write wrdata value(one byte) into i2c
 * Parameters   : uint8 wrdata - write value
 * Returns	  : NONE
*******************************************************************************/
void i2c_master_writeByte(uint8 wrdata)
{
	uint8 dat;
	sint8 i;
/*
	i2c_master_wait(I2C_SLEEP_TIME);
	i2c_master_setDC(m_nLastSDA, 0);
*/
	i2c_master_wait(I2C_SLEEP_TIME);

	for (i = 7; i >= 0; i--) {
		dat = wrdata >> i;
		i2c_master_setDC(dat, 0);
		i2c_master_wait(I2C_SLEEP_TIME);
		i2c_master_setDC(dat, 1);
		i2c_master_wait(I2C_SLEEP_TIME);
/*
		if (i == 0) {
			i2c_master_wait(5);   ////
		}
*/
		i2c_master_setDC(dat, 0);
		i2c_master_wait(I2C_SLEEP_TIME);
	}
}

bool i2c_master_writeBytes(uint8 address, uint8 *values, uint8 length)
{
	i2c_master_start();

	i2c_master_writeByte(address);
	if (!i2c_master_checkAck())
	{
		i2c_master_stop();
#ifdef CONFIG_I2C_MASTER_DEBUG
		console_printf( "Device not ACKed on address\n" );
#endif
		return false;
	}

	for(uint8 i = 0; i < length; i++){

		i2c_master_writeByte(values[i]);
		if (!i2c_master_checkAck())
		{
#ifdef CONFIG_I2C_MASTER_DEBUG
			console_printf( "Device not ACKed on write\n" );
#endif
			i2c_master_stop();
			return false;
		}
	}

	i2c_master_stop();
	return true;
}


bool i2c_master_writeBytes1(uint8 address, uint8 byte1)
{
	uint8 data[1];
	data[0] = byte1;
	return i2c_master_writeBytes(address, data, 1);
}

bool i2c_master_writeBytes2(uint8 address, uint8 byte1, uint8 byte2)
{
	uint8 data[2];
	data[0] = byte1;
	data[1] = byte2;
	return i2c_master_writeBytes(address, data, 2);
}

bool i2c_master_writeBytes3(uint8 address, uint8 byte1, uint8 byte2, uint8 byte3)
{
	uint8 data[3];
	data[0] = byte1;
	data[1] = byte2;
	data[2] = byte3;
	return i2c_master_writeBytes(address, data, 3);
}

bool i2c_master_readBytes(uint8 address, uint8 *values, uint8 length)
{
	if(values[0] > 0){
		if(!i2c_master_writeBytes(address, values, 1)){
			return false;
		}
	}

	uint8 timeout = 100;
	do{
		i2c_master_start();
		i2c_master_writeByte(address+1);
		if(!i2c_master_checkAck()){
			i2c_master_stop();
			i2c_master_wait(1000);
			continue;
		}
		break;
	}while(--timeout>0);

	if(timeout == 0){
		return false;
	}

#ifdef CONFIG_I2C_MASTER_DEBUG
	console_printf("Read: ");
#endif
	uint8 readed = 0;
	while((readed < length) && (--timeout>0)){
		uint8 byte = i2c_master_readByte();
		values[readed++] = byte;
		i2c_master_setAck((readed == length));	// send the ACK or NAK as applicable
		i2c_master_setDC(1, 0); // release SDA
#ifdef CONFIG_I2C_MASTER_DEBUG
		console_printf("%d ", byte);
#endif
	}
#ifdef CONFIG_I2C_MASTER_DEBUG
	console_printf("\n");
#endif
	i2c_master_stop();
	return true;
}

bool i2c_master_readSint16(uint8 address, uint8 regaddr, sint16 *value)
{
	uint8 data[2];
	data[0] = regaddr;
	if(i2c_master_readBytes(address, data, 2)){
		*value = (data[0] << 8) | data[1];
		return true;
	}
	value = 0;
	return false;
}

bool i2c_master_readUint16(uint8 address, uint8 regaddr, uint16 *value)
{
	uint8 data[2];
	data[0] = regaddr;
	if(i2c_master_readBytes(address, data, 2)){
		*value = (data[0] << 8) | data[1];
		return true;
	}
	value = 0;
	return false;
}

bool i2c_master_readUint8(uint8 address, uint8 regaddr, uint8 *value)
{
	uint8 data[1];
	data[0] = regaddr;
	if(i2c_master_readBytes(address, data, 1)){
		*value = data[0];
		return true;
	}
	value = 0;
	return false;
}
