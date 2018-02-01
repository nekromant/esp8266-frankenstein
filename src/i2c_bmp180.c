/* based on Adafruit code */
#include <math.h>
#include "console.h"

#include "driver/i2c_master.h"
#include "driver/i2c_bmp180.h"
#include <sensorlogger.h>
#include <main.h>
#include <osapi.h>
#include <mem.h>

//#define CONFIG_USEFLOAT
//#define CONFIG_CMD_BMP180_DEBUG
#ifdef CONFIG_CMD_BMP180_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ## __VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

#include "console.h"

struct bmp180_inst {
#ifdef CONFIG_USEFLOAT
	sint16				AC1, AC2, AC3, VB1, VB2, MB, MC, MD;
	uint16				AC4, AC5, AC6;
	float				c5, c6, mc, md, x0, x1, x2, ay0, ay1, ay2, p0, p1, p2;
	float				LAST_BMP_TEMPERATURE;
	float				LAST_BMP_REAL_PRESSURE;
#else
	sint16				ac1, ac2, ac3;
	uint16				ac4, ac5, ac6;
	sint16				b1, b2;
	sint16				mb, mc, md;
	sint32				LAST_BMP_TEMPERATURE;
	sint32				LAST_BMP_REAL_PRESSURE;
#endif
	struct slogger_data_type	sld[2];
};

static uint16 BMP180_readRawValue(uint8 cmd)
{
	i2c_master_writeBytes2(BMP180_ADDRESS, BMP180_REG_CONTROL, cmd);
	switch (cmd) {
	case BMP180_COMMAND_TEMPERATURE:
	case BMP180_COMMAND_PRESSURE0:
		os_delay_us(BMP180_CONVERSION_TIME * 1000);
		break;
/*Unsupported yet. Need to read 3 bytes from device.
 *              case BMP180_COMMAND_PRESSURE1:
 *                      os_delay_us(8*1000);
 *                      break;
 *
 *              case BMP180_COMMAND_PRESSURE2:
 *                      os_delay_us(14*1000);
 *                      break;
 *
 *              case BMP180_COMMAND_PRESSURE3:
 *                      os_delay_us(26*1000);
 *                      break;
 */
	}

	uint16 result;
	if (i2c_master_readUint16(BMP180_ADDRESS, BMP180_REG_RESULT, &result))
		return result;

	return false;
}

bool BMP180_Read(struct bmp180_inst *this)
{
#ifdef CONFIG_USEFLOAT
	float tu, pu, a, s, x, y, z;

	tu = BMP180_readRawValue(BMP180_COMMAND_TEMPERATURE);
	a = this->c5 * (tu - this->c6);
	this->LAST_BMP_TEMPERATURE = a + (this->mc / (a + this->md));

	pu = BMP180_readRawValue(BMP180_COMMAND_PRESSURE0);
	s = this->LAST_BMP_TEMPERATURE - 25.0;
	x = (this->x2 * pow(s, 2)) + (this->x1 * s) + this->x0;
	y = (this->ay2 * pow(s, 2)) + (this->ay1 * s) + this->ay0;
	z = (pu - x) / y;
	this->LAST_BMP_REAL_PRESSURE = (((this->p2 * pow(z, 2)) + (this->p1 * z) + this->p0) * 0.75);
#else
	int32 UT;
	uint16 UP;
	int32 B3, B5, B6;
	uint32 B4, B7;
	int32 X1, X2, X3;
	int32 T, P;

	UT = BMP180_readRawValue(BMP180_COMMAND_TEMPERATURE);
	X1 = (UT - (sint32)this->ac6) * ((sint32)this->ac5) >> 15;
	X2 = ((sint32)this->mc << 11) / (X1 + (sint32)this->md);
	B5 = X1 + X2;
	T = (B5 + 8) >> 4;
	this->LAST_BMP_TEMPERATURE = T;

	dbg("UT: %d\nX1: %d\nX2: %d\nB5: %d\n", UT, X1, X2, B5);

	UP = BMP180_readRawValue(BMP180_COMMAND_PRESSURE0);
	B6 = B5 - 4000;
	X1 = ((sint32)this->b2 * ((B6 * B6) >> 12)) >> 11;
	X2 = ((sint32)this->ac2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = (((sint32)this->ac1 * 4 + X3) + 2) >> 2;

	dbg("UP: %d\nB5: %d\nB6: %d\nX1: %d\nX2: %d\nX3: %d\n", UP, B5, B6, X1, X2, X3);

	X1 = ((sint32)this->ac3 * B6) >> 13;
	X2 = ((sint32)this->b1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = ((uint32)this->ac4 * (uint32)(X3 + 32768)) >> 15;
	B7 = ((uint32)UP - B3) * (50000);

	if (B7 < 0x80000000)
		P = (B7 * 2) / B4;
	else
		P = (B7 / B4) * 2;

	dbg("X1: %d\nX2: %d\nX3: %d\nB4: %d\nB7: %d\nP: %d\n", X1, X2, X3, B4, B7, P);

	X1 = (P >> 8) * (P >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * P) >> 16;

	P = P + ((X1 + X2 + (sint32)3791) >> 4);
	this->LAST_BMP_REAL_PRESSURE = P * 0.75;

	dbg("X1: %d\nX2: %d\nP: %d\n", X1, X2, P);

#endif
	return true;
}


static double get_temperature(struct slogger_data_type *tp)
{
	struct bmp180_inst *this = tp->user_arg;

	BMP180_Read(this);
#ifndef CONFIG_USEFLOAT
	return ((double)this->LAST_BMP_TEMPERATURE) / 10;
#else
	return this->LAST_BMP_TEMPERATURE;
#endif
}

static double get_pressure(struct slogger_data_type *tp)
{
	struct bmp180_inst *this = tp->user_arg;

	BMP180_Read(this);
#ifndef CONFIG_USEFLOAT
	return ((double)this->LAST_BMP_REAL_PRESSURE) / 100;
#else
	return this->LAST_BMP_REAL_PRESSURE;
#endif
}

struct bmp180_inst *BMP180_Init()
{
	uint16 reg;

	if (!i2c_master_readUint16(BMP180_ADDRESS, BMP180_REG_CHIPID, &reg))
		return false;


	if (reg != BMP180_MAGIC_CHIPID && reg != BMP180_MAGIC_CHIPID2) {
		dbg("Invalid chip id: 0x%X, mustbe 0x%X\n", reg, BMP180_MAGIC_CHIPID);
		return false;
	}

	if (!i2c_master_readUint16(BMP180_ADDRESS, BMP180_REG_VERSION, &reg))
		return false;

	console_printf("i2c: Detected BMP180 sensor at address 0x%x\n", BMP180_ADDRESS);
	struct bmp180_inst *this = os_malloc(sizeof(*this));
	this->sld[0].type = "Temperature";
	this->sld[0].unit = "C";
	this->sld[0].description = "BMP180 Temperature";
	this->sld[0].user_arg = this;
	this->sld[0].get_current_value = get_temperature;

	this->sld[1].type = "Pressure";
	this->sld[1].unit = "mmHg";
	this->sld[1].description = "BMP180 Presure";
	this->sld[1].user_arg = this;
	this->sld[1].get_current_value = get_pressure;

	struct slogger_instance *ilog = svclog_get_global_instance();
	sensorlogger_instance_register_data_type(ilog, &this->sld[0]);
	sensorlogger_instance_register_data_type(ilog, &this->sld[1]);

#ifdef CONFIG_USEFLOAT
	//Read calibration values
	this->AC1 = i2c_master_readRegister16(BMP180_ADDRESS, 0xAA);
	this->AC2 = i2c_master_readRegister16(BMP180_ADDRESS, 0xAC);
	this->AC3 = i2c_master_readRegister16(BMP180_ADDRESS, 0xAE);
	this->AC4 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB0);
	this->AC5 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB2);
	this->AC6 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB4);
	this->VB1 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB6);
	this->VB2 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB8);
	this->MB = i2c_master_readRegister16(BMP180_ADDRESS, 0xBA);
	this->MC = i2c_master_readRegister16(BMP180_ADDRESS, 0xBC);
	this->MD = i2c_master_readRegister16(BMP180_ADDRESS, 0xBE);

	//Compute floating-point polynominals:
	float c3, c4, b1;
	c3 = 160.0 * pow(2, -15) * this->AC3;
	c4 = pow(10, -3) * pow(2, -15) * this->AC4;
	b1 = pow(160, 2) * pow(2, -30) * this->VB1;
	this->c5 = (pow(2, -15) / 160) * this->AC5;
	this->c6 = this->AC6;
	this->mc = (pow(2, 11) / pow(160, 2)) * this->MC;
	this->md = this->MD / 160.0;
	this->x0 = this->AC1;
	this->x1 = 160.0 * pow(2, -13) * this->AC2;
	this->x2 = pow(160, 2) * pow(2, -25) * this->VB2;
	this->ay0 = c4 * pow(2, 15);
	this->ay1 = c4 * c3;
	this->ay2 = c4 * b1;
	this->p0 = (3791.0 - 8.0) / 1600.0;
	this->p1 = 1.0 - 7357.0 * pow(2, -20);
	this->p2 = 3038.0 * 100.0 * pow(2, -36);
	return true;
#else

	if (i2c_master_readSint16(BMP180_ADDRESS, 0xAA, &this->ac1)
	    && i2c_master_readSint16(BMP180_ADDRESS, 0xAC, &this->ac2)
	    && i2c_master_readSint16(BMP180_ADDRESS, 0xAE, &this->ac3)
	    && i2c_master_readUint16(BMP180_ADDRESS, 0xB0, &this->ac4)
	    && i2c_master_readUint16(BMP180_ADDRESS, 0xB2, &this->ac5)
	    && i2c_master_readUint16(BMP180_ADDRESS, 0xB4, &this->ac6)
	    && i2c_master_readSint16(BMP180_ADDRESS, 0xB6, &this->b1)
	    && i2c_master_readSint16(BMP180_ADDRESS, 0xB8, &this->b2)
	    && i2c_master_readSint16(BMP180_ADDRESS, 0xBA, &this->mb)
	    && i2c_master_readSint16(BMP180_ADDRESS, 0xBC, &this->mc)
	    && i2c_master_readSint16(BMP180_ADDRESS, 0xBE, &this->md)) {
		dbg("ac1: %d\nac2: %d\nac3: %d\nac4: %d\nac5: %d\nac6: %d\nb1: %d\nb2: %d\nmb: %d\nmc: %d\nmd: %d\n",
		    this->ac1,
		    this->ac2,
		    this->ac3,
		    this->ac4,
		    this->ac5,
		    this->ac6,
		    this->b1,
		    this->b2,
		    this->mb,
		    this->mc,
		    this->md);
		return true;
	}
	console_printf("Cant read calibration values\n");
	return false;
#endif
}

FR_CONSTRUCTOR(bmp180_init){
	BMP180_Init();
}
