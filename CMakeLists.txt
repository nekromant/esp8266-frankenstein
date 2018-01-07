CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(Doxygen)
include(ExternalProject)
#Kconfig stuff
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/generated)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/config)

find_program(KCONF_MCONF kconfig-mconf)
find_program(KCONF_CONF kconfig-conf)

add_custom_target(configure COMMENT "Frankenstein now configured")

add_custom_target(menuconfig
  COMMAND ${KCONF_MCONF} ${CMAKE_SOURCE_DIR}/kcnf
  COMMAND ${CMAKE_COMMAND} ${CMAKE_BINARY_DIR}
)

set(AUTOCONF_H ${CMAKE_BINARY_DIR}/include/generated/autoconf.h)

add_custom_command(OUTPUT ${AUTOCONF_H}
    COMMAND ${KCONF_CONF} --silentoldconfig ${CMAKE_SOURCE_DIR}/kcnf
    DEPENDS ${CMAKE_BINARY_DIR}/.config
    )



function(target_link_iromified target obj)
  get_target_property(tp ${obj} TYPE)
  if (NOT TARGET iromify-${obj})
    if("${tp}" STREQUAL "OBJECT_LIBRARY")
      add_custom_target(iromify-${obj} DEPENDS ${obj} COMMAND
          ${CMAKE_SOURCE_DIR}/scripts/iromify ${CROSS_COMPILE} OBJECT
          $<JOIN:$<TARGET_PROPERTY:${obj},SOURCES>,\t>)
    else()
      add_custom_target(iromify-${obj} DEPENDS ${obj} COMMAND
          ${CMAKE_SOURCE_DIR}/scripts/iromify ${CROSS_COMPILE} LIBRARY
          $<TARGET_FILE:${obj}>)
    endif()
  endif()

  if(tp STREQUAL "OBJECT_LIBRARY")
    target_sources(${target} PUBLIC $<TARGET_OBJECTS:${obj}>)
    add_dependencies(${target} iromify-${obj} ${obj})
  else()
    target_link_libraries(${target} ${obj})
  endif()
endfunction()

function(fr_add_component desc var name config)
  if (${config})
    list(APPEND ${var} ${name})
    list(APPEND CORE_SRCS ${ARGN})
    set(CORE_SRCS ${CORE_SRCS} PARENT_SCOPE)
  endif()
endfunction()

macro(fr_add_service name config)
  fr_add_component("service" FR_SERVICE ${name} ${config} ${ARGN})
endmacro()

macro(fr_add_cmd name config)
  fr_add_component("command" FR_CMD ${name} ${config} ${ARGN})
endmacro()

function(parse_dot_config config)
  file(READ "${config}" contents)
  string(REGEX REPLACE ";" "\\\\;" contents "${contents}")
  string(REGEX REPLACE "\n" ";" contents "${contents}")
  foreach(line ${contents})
    if(NOT line MATCHES "^#.*")
      string(REPLACE "=" ";" line ${line})
      list(GET line 0 key)
      list(GET line 1 value)
      set(${key} ${value} PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

macro(config_init defconfig)
  if (NOT EXISTS ${CMAKE_BINARY_DIR}/.config)
    file(INSTALL ${defconfig}
      DESTINATION ${CMAKE_BINARY_DIR} RENAME .config
    )
    message("Using defconfig: ${defconfig}")
  endif()
  parse_dot_config(${CMAKE_BINARY_DIR}/.config)
endmacro()

config_init(${CMAKE_SOURCE_DIR}/configs/config_default)

if (NOT CROSS_COMPILE)
  SET(CROSS_COMPILE xtensa-lx106-elf)
endif()
SET(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/cross-compile.cmake")

project(esp8266-frankenstein)
set(CMAKE_EXE_LINKER_FLAGS "-Os -Wl,--gc-sections -mtext-section-literals -nostdlib -Wl,--no-check-sections -u call_user_start -u user_rf_cal_sector_set -Wl,-static  -T${CMAKE_SOURCE_DIR}/lds/eagle.app.v6.ld")
set(ESP8266_LIBRARIES -Wl,--start-group main cirom gcc lwip m pwm hal net80211 phy pp wpa ssl -Wl,--end-group)
set(CMAKE_C_FLAGS	"-Os -std=gnu99 -Wpointer-arith -mlongcalls -Wundef -Wl,-EL -ffunction-sections -fdata-sections -mtext-section-literals -fno-inline-functions -mlongcalls -D__ets__ -DICACHE_FLASH")
set(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -include ${AUTOCONF_H}")

include_directories(include)

set(CORE_SRCS
  src/user_rf_cal_sector_set.c
  src/newlib-dummies.c
  src/stdio.c
  src/helpers.c
  src/flash_end_marker.c
  src/env.c
  src/main.c
  src/microrl.c
  src/pin_map.c
  src/pin_map.c
  src/iwconnect.c
  src/console.c
  src/uart.c
  src/sensorlogger.c
  lib/esp8266-json/cJSON.c
  lib/esphttpclient/httpclient.c
  src/base64.c
  src/cmd_env.c
  src/console_auth.c
  src/cmd_common.c
)

fr_add_service(tcp CONFIG_SERVICE_TCP
  src/cbuf.c
  src/cbuftools.c
  src/tcpservice.c
)

fr_add_service(aura CONFIG_SERVICE_AURA
  src/svc_aura.c
)

fr_add_service(telnet CONFIG_SERVICE_TELNET
  src/svc_telnet.c
)

fr_add_service(sched CONFIG_ENABLE_SCHED
  src/sched.c
)

fr_add_service(sensorlogger CONFIG_SERVICE_SENSORLOGGER
  src/svc_sensorlogger.c
)

include_directories(include/driver)
include_directories(lib/esp8266-json)
include_directories(lib/esphttpclient/)

fr_add_cmd(wireless CONFIG_CMD_IW src/cmd_iw.c)
fr_add_cmd(uart CONFIG_CMD_UART src/cmd_uart.c)
fr_add_cmd(if CONFIG_CMD_IF src/cmd_ifconfig.c)
fr_add_cmd(gpio CONFIG_CMD_GPIO src/cmd_gpio.c)
fr_add_cmd(flash CONFIG_CMD_FLASH src/cmd_flash.c)
fr_add_cmd(listen CONFIG_CMD_LISTEN src/cmd_listen.c)
fr_add_cmd(send CONFIG_CMD_SEND src/cmd_send.c)
fr_add_cmd(DS18B20 CONFIG_CMD_DS18B20 src/cmd_ds18b20.c)
fr_add_service(MQTT CONFIG_ENABLE_MQTT
  src/cmd_mqtt.c
  src/mqtt/config.c
  src/mqtt/mqtt.c
  src/mqtt/mqtt_msg.c
  src/mqtt/proto.c
  src/mqtt/queue.c
  src/mqtt/ringbuf.c
  src/mqtt/utils.c
)

fr_add_service(tftp CONFIG_CMD_TFTP
  src/cmd_tftp.c
  src/tftp.c
)

fr_add_service(httpd CONFIG_SVC_HTTPD
  src/httpd/auth.c
  src/httpd/cgiwifi.c
  src/httpd/io.c
  src/httpd/config_store.c
  src/httpd/heatshrink_decoder.c
  src/httpd/httpd.c
  src/httpd/cgi.c
  src/httpd/espfs.c
  src/httpd/httpdespfs.c
  src/httpd/sysex.c
  src/httpd/main.c
)

fr_add_cmd(at CONFIG_CMD_AT src/cmd_at.c)
fr_add_cmd(adc CONFIG_CMD_ADC src/cmd_adc.c)
fr_add_cmd(nslookup CONFIG_CMD_NSLOOKUP src/cmd_nslookup.c)
fr_add_cmd(wiphy CONFIG_CMD_PHY src/cmd_wiphy.c)
fr_add_cmd(ping CONFIG_CMD_PING src/cmd_ping.c)
fr_add_cmd(i2c CONFIG_CMD_I2C src/cmd_i2c.c src/i2c_master.c)
fr_add_cmd(i2c_bmp180 CONFIG_CMD_I2C_BMP180 src/i2c_bmp180.c)
fr_add_cmd(i2c_ina219 CONFIG_CMD_I2C_INA219 src/i2c_ina219.c)
fr_add_cmd(i2c_sht21 CONFIG_CMD_I2C_SHT21 src/i2c_sht21.c)
fr_add_cmd(i2c_si7020 CONFIG_CMD_I2C_SI7020 src/i2c_si7020.c)
fr_add_cmd(i2c_bh1750 CONFIG_CMD_I2C_BH1750 src/i2c_bh1750.c)
fr_add_cmd(i2c_pcf8591 CONFIG_CMD_I2C_PCF8591 src/i2c_pcf8591.c)
fr_add_cmd(i2c_hmc5883l CONFIG_CMD_I2C_HMC5883L src/i2c_hmc5883l.c)
fr_add_cmd(i2c_tcs3414cs CONFIG_CMD_I2C_TCS3414CS src/i2c_tcs3414cs.c)
fr_add_cmd(i2c_mlx90614 CONFIG_CMD_I2C_MLX90614 src/i2c_mlx90614.c)
fr_add_cmd(passthrough CONFIG_ENABLE_PASSTHROUGH src/cmd_passthrough.c)


fr_add_service(vdd3v3 CONFIG_SENSLOG_VDD3V3 src/sensorloggers/vdd3v3.c)
fr_add_service(vdd3v3 CONFIG_SENSLOG_ADC src/sensorloggers/adc.c)
fr_add_service(ping CONFIG_SENSLOG_PING src/sensorloggers/ping.c)

SET_SOURCE_FILES_PROPERTIES(${AUTOCONF_H}
PROPERTIES HEADER_FILE_ONLY TRUE)

foreach(f ${CORE_SRCS})
  SET_SOURCE_FILES_PROPERTIES(${f} PROPERTIES
      OBJECT_DEPENDS ${AUTOCONF_H})
endforeach()

add_library(frankenstein OBJECT ${CORE_SRCS})
add_executable(frank $<TARGET_OBJECTS:frankenstein>)
target_link_iromified(frank frankenstein)
target_link_libraries(frank ${ESP8266_LIBRARIES} )




ExternalProject_Add(mkespfsimage
    SOURCE_DIR        "${CMAKE_SOURCE_DIR}/mkespfsimage"
    BINARY_DIR        "${CMAKE_BINARY_DIR}/mkespfsimage"
    PREFIX            "${CMAKE_BINARY_DIR}/mkespfsimage/prefix"
    INSTALL_COMMAND   ""
  )


add_custom_target(filesystem
  ALL
  COMMAND find . | ${CMAKE_BINARY_DIR}/mkespfsimage/mkespfsimage > ${CMAKE_BINARY_DIR}/filesystem.espfs
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/html
)

add_custom_target(FLASH
  COMMAND esptool.py --chip esp8266 elf2image frank
  COMMAND pl2303gpio --gpio=0 --out=0 --gpio=1 --out=0
  --gpio=1 --in --sleep 350
  --gpio=0 --in
  COMMAND esptool.py write_flash 0x0 frank-0x00000.bin
  COMMAND pl2303gpio --gpio=0 --out=0 --gpio=1 --out=0
  --gpio=1 --in --sleep 350
  --gpio=0 --in
  COMMAND esptool.py write_flash 0x10000 frank-0x10000.bin
  COMMAND minicom -o -D /dev/ttyUSB0 -b 115200
)
