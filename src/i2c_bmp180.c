/* based on Adafruit code */
#include <math.h>
#include "console.h"

#include "driver/i2c_master.h"
#include "driver/i2c_bmp180.h"

#ifdef CONFIG_CMD_BMP180_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

#include "console.h"

#ifdef CONFIG_USEFLOAT
static sint16 AC1,AC2,AC3,VB1,VB2,MB,MC,MD;
static uint16 AC4,AC5,AC6; 
static float c5,c6,mc,md,x0,x1,x2,ay0,ay1,ay2,p0,p1,p2;
#else
static sint16 ac1, ac2, ac3;
static uint16 ac4, ac5, ac6;
static sint16 b1, b2;
static sint16 mb, mc, md; 
#endif

static uint16 BMP180_readRawValue(uint8 cmd) 
{
	i2c_master_writeBytes2(BMP180_ADDRESS, BMP180_REG_CONTROL, cmd);
	switch(cmd){
		case BMP180_COMMAND_TEMPERATURE:
		case BMP180_COMMAND_PRESSURE0:
			os_delay_us(BMP180_CONVERSION_TIME*1000);
			break;
/*Unsupported yet. Need to read 3 bytes from device.
		case BMP180_COMMAND_PRESSURE1:
			os_delay_us(8*1000);
			break;

		case BMP180_COMMAND_PRESSURE2:
			os_delay_us(14*1000);
			break;

		case BMP180_COMMAND_PRESSURE3:
			os_delay_us(26*1000);
			break;
*/
	}
	
	uint16 result;
	if(i2c_master_readUint16(BMP180_ADDRESS, BMP180_REG_RESULT, &result)){
		return result;
	}

	return false;
}

bool BMP180_Read()
{
#ifdef CONFIG_USEFLOAT
	float tu,pu,a,s,x,y,z;

	tu = BMP180_readRawValue(BMP180_COMMAND_TEMPERATURE);
	a = c5 * (tu - c6);
	LAST_BMP_TEMPERATURE = a + (mc / (a + md));

 	pu = BMP180_readRawValue(BMP180_COMMAND_PRESSURE0);
	s = LAST_BMP_TEMPERATURE - 25.0;
	x = (x2 * pow(s,2)) + (x1 * s) + x0;
	y = (ay2 * pow(s,2)) + (ay1 * s) + ay0;
	z = (pu - x) / y;
	LAST_BMP_REAL_PRESSURE = (((p2 * pow(z,2)) + (p1 * z) + p0) * 0.75);
#else
	int32 UT;
	uint16 UP;
	int32 B3, B5, B6;
	uint32 B4, B7;
	int32 X1, X2, X3;
	int32 T, P;
	
	UT = BMP180_readRawValue(BMP180_COMMAND_TEMPERATURE);
	X1 = (UT - (sint32)ac6) * ((sint32)ac5) >> 15;
	X2 = ((sint32)mc << 11) / (X1 + (sint32)md); 
	B5 = X1 + X2;
	T  = (B5+8) >> 4;
	LAST_BMP_TEMPERATURE = T; 

	dbg( "UT: %d\nX1: %d\nX2: %d\nB5: %d\n", UT, X1, X2, B5);

	UP = BMP180_readRawValue(BMP180_COMMAND_PRESSURE0);
	B6 = B5 - 4000;
	X1 = ((sint32)b2 * ((B6 * B6) >> 12)) >> 11;
	X2 = ((sint32)ac2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = (((sint32)ac1 * 4 + X3) + 2) >> 2;

	dbg( "UP: %d\nB5: %d\nB6: %d\nX1: %d\nX2: %d\nX3: %d\n", UP, B5, B6, X1, X2, X3);

	X1 = ((sint32)ac3 * B6) >> 13;
	X2 = ((sint32)b1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = ((uint32)ac4 * (uint32)(X3 + 32768)) >> 15;
	B7 = ((uint32)UP - B3) * (50000);
	
	if (B7 < 0x80000000) {
		P = (B7 * 2) / B4;
	} else {
		P = (B7 / B4) * 2;
	}

	dbg( "X1: %d\nX2: %d\nX3: %d\nB4: %d\nB7: %d\nP: %d\n", X1, X2, X3, B4, B7, P);

	X1 = (P >> 8) * (P >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * P) >> 16;

	P  = P + ((X1 + X2 + (sint32)3791) >> 4);
	LAST_BMP_REAL_PRESSURE = P * 0.75;

	dbg( "X1: %d\nX2: %d\nP: %d\n", X1, X2, P);

#endif
	return true;
}

bool BMP180_Init()
{

	uint16 reg;
	if(!i2c_master_readUint16(BMP180_ADDRESS, BMP180_REG_CHIPID, &reg))
		return false;
	

	if(reg != BMP180_MAGIC_CHIPID){
		dbg( "Invalid chip id: 0x%X, mustbe 0x%X\n", reg, BMP180_MAGIC_CHIPID);
		return false;
	}

	if(!i2c_master_readUint16(BMP180_ADDRESS, BMP180_REG_VERSION, &reg))
		return false;

#ifdef CONFIG_USEFLOAT
	//Read calibration values
	AC1 = i2c_master_readRegister16(BMP180_ADDRESS, 0xAA);				 
	AC2 = i2c_master_readRegister16(BMP180_ADDRESS, 0xAC);
	AC3 = i2c_master_readRegister16(BMP180_ADDRESS, 0xAE);
	AC4 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB0);
	AC5 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB2);
	AC6 = i2c_master_readRegister16(BMP180_ADDRESS, 0xB4);
	VB1  = i2c_master_readRegister16(BMP180_ADDRESS, 0xB6);
	VB2  = i2c_master_readRegister16(BMP180_ADDRESS, 0xB8);
	MB  = i2c_master_readRegister16(BMP180_ADDRESS, 0xBA);
	MC  = i2c_master_readRegister16(BMP180_ADDRESS, 0xBC);
	MD  = i2c_master_readRegister16(BMP180_ADDRESS, 0xBE);

	//Compute floating-point polynominals:
	float c3,c4,b1;
	c3 = 160.0 * pow(2,-15) * AC3;
	c4 = pow(10,-3) * pow(2,-15) * AC4;
	b1 = pow(160,2) * pow(2,-30) * VB1;
	c5 = (pow(2,-15) / 160) * AC5;
	c6 = AC6;
	mc = (pow(2,11) / pow(160,2)) * MC;
	md = MD / 160.0;
	x0 = AC1;
	x1 = 160.0 * pow(2,-13) * AC2;
	x2 = pow(160,2) * pow(2,-25) * VB2;
	ay0 = c4 * pow(2,15);
	ay1 = c4 * c3;
	ay2 = c4 * b1;
	p0 = (3791.0 - 8.0) / 1600.0;
	p1 = 1.0 - 7357.0 * pow(2,-20);
	p2 = 3038.0 * 100.0 * pow(2,-36);
	IS_ALREADY_INITED = true;
	return true;
#else

	if(i2c_master_readSint16(BMP180_ADDRESS, 0xAA, &ac1)
	&& i2c_master_readSint16(BMP180_ADDRESS, 0xAC, &ac2)
	&& i2c_master_readSint16(BMP180_ADDRESS, 0xAE, &ac3)
	&& i2c_master_readUint16(BMP180_ADDRESS, 0xB0, &ac4)
	&& i2c_master_readUint16(BMP180_ADDRESS, 0xB2, &ac5)
	&& i2c_master_readUint16(BMP180_ADDRESS, 0xB4, &ac6)
	&& i2c_master_readSint16(BMP180_ADDRESS, 0xB6, &b1)
	&& i2c_master_readSint16(BMP180_ADDRESS, 0xB8, &b2)
	&& i2c_master_readSint16(BMP180_ADDRESS, 0xBA, &mb)
	&& i2c_master_readSint16(BMP180_ADDRESS, 0xBC, &mc)
	&& i2c_master_readSint16(BMP180_ADDRESS, 0xBE, &md)){

	dbg( "ac1: %d\nac2: %d\nac3: %d\nac4: %d\nac5: %d\nac6: %d\nb1: %d\nb2: %d\nmb: %d\nmc: %d\nmd: %d\n", ac1,ac2,ac3,ac4,ac5,ac6,b1,b2,mb,mc,md);
		
		IS_ALREADY_INITED = true;
		return true;
	}
	console_printf( "Cant read calibration values\n");
	return false;
#endif
}

static int do_i2c_bmp180(int argc, const char* const* argv)
{
	if(argc == 1 || strcmp(argv[1], "read") == 0){

		if((IS_ALREADY_INITED || BMP180_Init()) && BMP180_Read()){
			console_printf( argc == 1 ? "%d %d\n" : "Temperature: %d C\nPressure: %d mmHg\n", 
#ifdef CONFIG_USEFLOAT
				(int)(LAST_BMP_TEMPERATURE*100), 
				(uint32_t)(LAST_BMP_REAL_PRESSURE*100)
#else
				LAST_BMP_TEMPERATURE,
				LAST_BMP_REAL_PRESSURE
#endif
			);
		}else{
			console_printf( "Failed to read value\n" );
		}
	} else

	if(strcmp(argv[1], "init") == 0){

		console_printf( BMP180_Init() ? "Ok\n":"Failed\n" );
	} 

	return false;
}

CONSOLE_CMD(i2c_bmp180, 0, 2, 
		do_i2c_bmp180, NULL, NULL, 
		"I2C pressure sensor BMP180"
		HELPSTR_NEWLINE "i2c_bmp180 init"
		HELPSTR_NEWLINE "i2c_bmp180 [read]"
);
