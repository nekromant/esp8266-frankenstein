#ifndef __I2C_INA219_H
#define	__I2C_INA219_H

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

//#define INA219_ADDRESS 0x80
#define INA219_ADDRESS 0x82

#define INA219_REG_CONFIG         0x00
#define INA219_REG_SHUNTVOLTAGE   0x01
#define INA219_REG_BUSVOLTAGE     0x02
#define INA219_REG_POWER          0x03
#define INA219_REG_CURRENT        0x04
#define INA219_REG_CALIBRATION    0x05

typedef enum
{
    INA219_RANGE_16V            = 0b0,
    INA219_RANGE_32V            = 0b1
} ina219_range_t;

typedef enum
{
    INA219_GAIN_40MV            = 0b00,
    INA219_GAIN_80MV            = 0b01,
    INA219_GAIN_160MV           = 0b10,
    INA219_GAIN_320MV           = 0b11
} ina219_gain_t;

typedef enum
{
    INA219_BUS_RES_9BIT         = 0b0000,
    INA219_BUS_RES_10BIT        = 0b0001,
    INA219_BUS_RES_11BIT        = 0b0010,
    INA219_BUS_RES_12BIT        = 0b0011
} ina219_busRes_t;

typedef enum
{
    INA219_SHUNT_RES_9BIT_1S    = 0b0000,
    INA219_SHUNT_RES_10BIT_1S   = 0b0001,
    INA219_SHUNT_RES_11BIT_1S   = 0b0010,
    INA219_SHUNT_RES_12BIT_1S   = 0b0011,
    INA219_SHUNT_RES_12BIT_2S   = 0b1001,
    INA219_SHUNT_RES_12BIT_4S   = 0b1010,
    INA219_SHUNT_RES_12BIT_8S   = 0b1011,
    INA219_SHUNT_RES_12BIT_16S  = 0b1100,
    INA219_SHUNT_RES_12BIT_32S  = 0b1101,
    INA219_SHUNT_RES_12BIT_64S  = 0b1110,
    INA219_SHUNT_RES_12BIT_128S = 0b1111
} ina219_shuntRes_t;

typedef enum
{
    INA219_MODE_POWER_DOWN      = 0b000,
    INA219_MODE_SHUNT_TRIG      = 0b001,
    INA219_MODE_BUS_TRIG        = 0b010,
    INA219_MODE_SHUNT_BUS_TRIG  = 0b011,
    INA219_MODE_ADC_OFF         = 0b100,
    INA219_MODE_SHUNT_CONT      = 0b101,
    INA219_MODE_BUS_CONT        = 0b110,
    INA219_MODE_SHUNT_BUS_CONT  = 0b111,
} ina219_mode_t;

enum {
    CONFIGURE_READ_POWERDOWN = 0,
    GET_VOLTAGE,
    GET_CURRENT,
    GET_SHUNT_VOLTAGE,
    GET_POWER
};

#define INA219_CALIBRATION_VALUE  4096  //RSHUNT = 0.1 ohm; max expected I = 1 A
#define INA219_CONFIG_VALUE  (INA219_RANGE_16V << 13 | INA219_GAIN_80MV << 11 | INA219_BUS_RES_12BIT << 7 | INA219_SHUNT_RES_12BIT_1S << 3)

uint16 LAST_INA219_VOLTAGE;
uint16 LAST_INA219_CURRENT;
uint16 LAST_INA219_SHUNT_VOLTAGE;
uint16 LAST_INA219_POWER;
static bool IS_ALREADY_INITED;

bool INA219_Init(void);
bool INA219_Read(void);

#endif
