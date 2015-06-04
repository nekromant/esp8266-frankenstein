objects-y+=main.o stdio.o helpers.o flash_end_marker.o
objects-y+=microrl.o
objects-y+=console.o
# useless objects-y+=hostname.o
objects-y+=env.o
objects-$(CONFIG_SERVICE_TCP)+=cbuf.o
objects-$(CONFIG_SERVICE_TCP)+=cbuftools.o
objects-$(CONFIG_SERVICE_TCP)+=tcpservice.o
objects-$(CONFIG_SERVICE_TELNET)+=svc_telnet.o
objects-$(CONFIG_SERVICE_ECHO)+=svc_echo.o

objects-y+=cmd_common.o cmd_env.o uart.o
objects-y+=pin_map.o
objects-y+=iwconnect.o

objects-$(CONFIG_CMD_IW)+=cmd_iw.o
objects-$(CONFIG_CMD_UART)+=cmd_uart.o
objects-$(CONFIG_CMD_IF)+=cmd_ifconfig.o
objects-$(CONFIG_CMD_GPIO)+=cmd_gpio.o
objects-$(CONFIG_CMD_FLASH)+=cmd_flash.o
objects-$(CONFIG_CMD_LISTEN)+=cmd_listen.o
objects-$(CONFIG_CMD_SEND)+=cmd_send.o
objects-$(CONFIG_CMD_DS18B20)+=cmd_ds18b20.o
objects-$(CONFIG_CMD_DHT22)+=cmd_dht22.o
objects-$(CONFIG_CMD_TFTP)+=cmd_tftp.o tftp.o
objects-$(CONFIG_CMD_AT)+=cmd_at.o
objects-$(CONFIG_CMD_ADC)+=cmd_adc.o
objects-$(CONFIG_CMD_NSLOOKUP)+=cmd_nslookup.o
#objects-$(CONFIG_CMD_PING)+=cmd_ping.o
#objects-$(CONFIG_ENABLE_PASSTHROUGH)+=cmd_passthrough.o
objects-$(CONFIG_CMD_PHY)+=cmd_wiphy.o

objects-$(CONFIG_CMD_I2C)+=cmd_i2c.o i2c_master.o 
objects-$(CONFIG_CMD_I2C_BMP180)+=i2c_bmp180.o
objects-$(CONFIG_CMD_I2C_INA219)+=i2c_ina219.o
objects-$(CONFIG_CMD_I2C_SHT21)+=i2c_sht21.o
objects-$(CONFIG_CMD_I2C_SI7020)+=i2c_si7020.o
objects-$(CONFIG_CMD_I2C_BH1750)+=i2c_bh1750.o
objects-$(CONFIG_CMD_I2C_PCF8591)+=i2c_pcf8591.o
objects-$(CONFIG_CMD_I2C_HMC5883L)+=i2c_hmc5883l.o
objects-$(CONFIG_CMD_I2C_TCS3414CS)+=i2c_tcs3414cs.o
objects-$(CONFIG_CMD_I2C_MLX90614)+=i2c_mlx90614.o

ifneq ($(CONFIG_ESP8266_BLOB_LWIP),y)
objects-y+=esp_timer.o
subdirs-y+=lwip-app/
ifeq ($(CONFIG_LWIP_ESP),y)
subdirs-y+=lwip-esp
endif
ifeq ($(CONFIG_LWIP_GIT),y)
subdirs-y+=lwip-git
endif
endif

objects-$(CONFIG_ENABLE_MQTT)+=cmd_mqtt.o
subdirs-y+=mqtt

objects-$(CONFIG_ENABLE_SCHED)+=sched.o

