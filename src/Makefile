objects-y+=main.o stdio.o helpers.o
objects-y+=microrl.o
objects-y+=console.o
objects-y+=hostname.o
objects-y+=env.o
objects-y+=strbuf.o
objects-y+=tcpbuf.o

objects-y+=telnet.o

objects-y+=cmd_common.o cmd_env.o uart.o

objects-$(CONFIG_CMD_IW)+=cmd_iw.o
objects-$(CONFIG_CMD_IW)+=cmd_uart.o
objects-$(CONFIG_CMD_IF)+=cmd_ifconfig.o
objects-$(CONFIG_CMD_GPIO)+=cmd_gpio.o
objects-$(CONFIG_CMD_FLASH)+=cmd_flash.o
objects-$(CONFIG_CMD_LISTEN)+=cmd_listen.o
objects-$(CONFIG_CMD_SEND)+=cmd_send.o
objects-$(CONFIG_CMD_DS18B20)+=cmd_ds18b20.o
objects-$(CONFIG_CMD_TFTP)+=cmd_tftp.o tftp.o
objects-$(CONFIG_CMD_AT)+=cmd_at.o
objects-$(CONFIG_CMD_ADC)+=cmd_adc.o
objects-$(CONFIG_ENABLE_PASSTHROUGH)+=cmd_passthrough.o


subdirs-y+=netapps/

subdirs-y+=lwip
