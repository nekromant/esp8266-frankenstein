/* reaper7 */

#include "driver/i2c_master.h"
#include "driver/i2c_ina219.h"

static uint32_t _voltage, _current, _shuntvoltage, _power;

bool ICACHE_FLASH_ATTR
INA219_writereg16(uint8 reg, uint16_t data)
{
  i2c_master_start(); //Start i2c
  i2c_master_writeByte(INA219_ADDRESS);
  if (!i2c_master_checkAck()) {
    //os_printf("-%s-%s START slave not ack... return \r\n", __FILE__, __func__);
    i2c_master_stop();
    return(0);
  }
  i2c_master_writeByte(reg);
  if(!i2c_master_checkAck()){
    //os_printf("-%s-%s REG slave not ack... return \r\n", __FILE__, __func__);
    i2c_master_stop();
    return(0);
  }
  
  uint8_t dlv;
  dlv = (uint8_t)data;
  data >>= 8;
  
  //os_printf("-%s-%s DATA H[%d] L[%d] \r\n", __FILE__, __func__, (uint8_t)data, dlv);
  
  i2c_master_writeByte((uint8_t)data);
  if(!i2c_master_checkAck()){
    //os_printf("-%s-%s HDATA slave not ack... return \r\n", __FILE__, __func__);
    i2c_master_stop();
    return(0);
  }
  
  i2c_master_writeByte(dlv);
  if(!i2c_master_checkAck()){
    //os_printf("-%s-%s LDATA slave not ack... return \r\n", __FILE__, __func__);
    i2c_master_stop();
    return(0);
  }
  
  i2c_master_stop();
    
  return(1); 
}

uint16_t ICACHE_FLASH_ATTR
INA219_readreg16(uint8 reg)
{
  i2c_master_start(); 
  i2c_master_writeByte(INA219_ADDRESS); 
  if(!i2c_master_checkAck()){
    //os_printf("-%s-%s START slave not ack... return \r\n", __FILE__, __func__);
    i2c_master_stop();
    return(0);
  }
  i2c_master_writeByte(reg);
  if(!i2c_master_checkAck()){
    //os_printf("-%s-%s REG slave not ack... return \r\n", __FILE__, __func__);
    i2c_master_stop();
    return(0);
  }
  i2c_master_start(); 
  i2c_master_writeByte(INA219_ADDRESS+1); 
  if(!i2c_master_checkAck()){
    //os_printf("-%s-%s START READ slave not ack... return \r\n", __FILE__, __func__);
    i2c_master_stop();
    return(0);
  }
  uint8_t msb = i2c_master_readByte();                 
  i2c_master_setAck(1);                       		
  uint8_t lsb = i2c_master_readByte();     
  i2c_master_setAck(0);                       		
  i2c_master_stop();                           	
  uint16_t res = msb << 8;
  res += lsb;	
  return res; 
}

bool ICACHE_FLASH_ATTR
INA219_Init()
{
  if (!INA219_writereg16(INA219_REG_CALIBRATION, INA219_CALIBRATION_VALUE))
    return 0;
    
  return 1;
}

int32_t ICACHE_FLASH_ATTR
INA219_GetVal(uint8 mode)
{
  if (mode==CONFIGURE_READ_POWERDOWN) {
    if (!INA219_writereg16(INA219_REG_CONFIG, (INA219_CONFIG_VALUE | INA219_MODE_SHUNT_BUS_TRIG)))
      return 0;
    _voltage = INA219_readreg16(INA219_REG_BUSVOLTAGE);
    _voltage = (_voltage >> 3) * 4;
    _current = INA219_readreg16(INA219_REG_CURRENT);
    if (((_current) >> (15)) & 0x01) { //nagative
      _current = 0;
      _shuntvoltage = 0;
      _power = 0;
    } else {
      _current = _current / 100;
      _shuntvoltage = INA219_readreg16(INA219_REG_SHUNTVOLTAGE) / 1000;
      _power = INA219_readreg16(INA219_REG_POWER) / 10;
      _power = _power * 2;
    }
    if (!INA219_writereg16(INA219_REG_CONFIG, (INA219_CONFIG_VALUE | INA219_MODE_POWER_DOWN)))
      return 0;
    return 1;
  } else if (mode==GET_VOLTAGE) {
    return _voltage;
  } else if (mode==GET_CURRENT) {
    return _current;
  } else if (mode==GET_SHUNT_VOLTAGE) {
    return _shuntvoltage;
  } else if (mode==GET_POWER) {
    return _power;
  }
  
  //os_printf("-%s-%s GET[%d]=[%d] \r\n", __FILE__, __func__, val,res);
  
  return 0;
}
