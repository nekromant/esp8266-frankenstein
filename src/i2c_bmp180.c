/* reaper7 */

#include <math.h>
#include "driver/i2c_master.h"
#include "driver/i2c_bmp180.h"

#ifdef CONFIG_CMD_BMP180_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

#include "console.h"


static int16_t ac1, ac2, ac3;
static uint16_t ac4, ac5, ac6;
static int16_t b1, b2;
static int16_t mb, mc, md; 

static int16_t ICACHE_FLASH_ATTR
BMP180_readRawValue(uint8_t cmd) 
{
	i2c_master_writeRegister(BMP180_ADDRESS, BMP180_REG_CONTROL, cmd);
	os_delay_us(BMP180_CONVERSION_TIME*1000);
	return i2c_master_readRegister16(BMP180_ADDRESS, BMP180_REG_RESULT);
}

bool ICACHE_FLASH_ATTR
BMP180_Read()
{
	int32_t UT;
	uint16_t UP;
	int32_t B3, B5, B6;
	uint32_t B4, B7;
	int32_t X1, X2, X3;
	int32_t T, P;
	
	UT = BMP180_readRawValue(BMP180_COMMAND_TEMPERATURE);
	X1 = (UT - (int32_t)ac6) * ((int32_t)ac5) >> 15;
	X2 = ((int32_t)mc << 11) / (X1 + (int32_t)md); 
	B5 = X1 + X2;
	T  = (B5+8) >> 4;
	LAST_BMP_TEMPERATURE = T; //div by 10

	UP = BMP180_readRawValue(BMP180_COMMAND_PRESSURE0);
	B6 = B5 - 4000;
	X1 = ((int32_t)b2 * ((B6 * B6) >> 12)) >> 11;
	X2 = ((int32_t)ac2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = (((int32_t)ac1 * 4 + X3) + 2) / 4;
	X1 = ((int32_t)ac3 * B6) >> 13;
	X2 = ((int32_t)b1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = ((uint32_t)ac4 * (uint32_t)(X3 + 32768)) >> 15;
	B7 = ((uint32_t)UP - B3) * (uint32_t)(50000UL);
	
	if (B7 < 0x80000000) {
		P = (B7 * 2) / B4;
	} else {
		P = (B7 / B4) * 2;
	}

	X1 = (P >> 8) * (P >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * P) >> 16;
	P  = P + ((X1 + X2 + (int32_t)3791) >> 4);
	LAST_BMP_REAL_PRESSURE = P * 0.75; //div by 100
	return true;
}

bool ICACHE_FLASH_ATTR
BMP180_Init()
{
	if (i2c_master_readRegister8(BMP180_ADDRESS, BMP180_REG_VERSION) != BMP180_MAGIC_VERSION)
		return 0;

	ac1 = i2c_master_readRegister16(BMP180_ADDRESS, 0xAA);				 
	ac2 = i2c_master_readRegister16(BMP180_ADDRESS, 0xAC);
	ac3 = i2c_master_readRegister16(BMP180_ADDRESS, 0xAE);
	ac4 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB0);
	ac5 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB2);
	ac6 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB4);
	b1  = i2c_master_readRegister16(BMP180_ADDRESS, 0xB6);
	b2  = i2c_master_readRegister16(BMP180_ADDRESS, 0xB8);
	mb  = i2c_master_readRegister16(BMP180_ADDRESS, 0xBA);
	mc  = i2c_master_readRegister16(BMP180_ADDRESS, 0xBC);
	md  = i2c_master_readRegister16(BMP180_ADDRESS, 0xBE);
	
	return 1;
}

