# Scope

This document is designed to document the sensorlogger subsystem of frankenstein. Or, better, to get you up and running quickly.

# Requirements

- A nextcloud/owncloud server running the [sensorlogger app](https://github.com/alexstocker/)
- Internet connectivity for your esp8266
- frankenstein firmware

# Quick setup

- Setup the environment

```
frankenstein > setenv slog-nextCloudUrl  https://cloud.example.com/nextcloud
frankenstein > setenv slog-userName  user
frankenstein > setenv slog-password  device-password
frankenstein > saveenv
```

For security reasons you are adviced to use 'device password' functionality of nextcloud instead of supplying your own password here, since it's stored in plaintext.

For more configuration parameters see "Environment" section.

- Use `senslog dump` to view current logger settings

```
frankenstein > senslog dump
deviceId: frankenstein-98dc9e
deviceName: frankenstein-sensorlogger
deviceType: dummy
deviceGroup: development
deviceParentGroup: <null>
nextCloudUrl: http://silverblade/nextcloud
userName: user
password: device-password
== Data types ==
type: Voltage
description: VDD3V3 Voltage
unit: mV
dataTypeId: -1
== == ==
frankenstein >
```

- Use `senslog register` to register the device within the nextcloud instance. The command implies `senslog get_dt` that obtains data type ids from the server. You required to run this command every time the device starts up. The command will output the json requests to the server. Example output:

```
frankenstein > senslog register
Registering device at http://silverblade/nextcloud/index.php/apps/sensorlogger/api/v1/registerdevice/
{
        "deviceId":     "frankenstein-99dc9e",
        "deviceName":   "frankenstein-sensorlogger",
        "deviceType":   "dummy",
        "deviceGroup":  "development",
        "deviceParentGroup":    "",
        "deviceDataTypes":      [{
                        "type": "Voltage",
                        "description":  "VDD3V3 Voltage",
                        "unit": "mV"
                }]
}
http status: 200
server response: {"success":true,"message":"Device successfully registered","data":[]}

frankenstein > senslog get_dt
Requesting info at http://silverblade/nextcloud/index.php/apps/sensorlogger/api/v1/getdevicedatatypes/
{
        "deviceId":     "frankenstein-99dc9e"
}
http status: 200
server resp: [{"id":"1","description":"VDD3V3 Voltage","type":"Voltage","short":"mV"}]

frankenstein >
```

Note that `sensog get_dt` will be issued automatically after registering.

At this point you can check, that all data types now have a valid dataTypeId obtained from the server when you call senslog dump.

```
frankenstein > senslog dump
deviceId: frankenstein-99dc9e
deviceName: frankenstein-sensorlogger
deviceType: dummy
deviceGroup: development
deviceParentGroup: <null>
nextCloudUrl: http://silverblade/nextcloud
userName: admin
password: admin123
 == Data types ==
type: Voltage
description: VDD3V3 Voltage
unit: mV
dataTypeId: 1
 == == ==
```

- Send sensor readings to your server using `senslog post`

```
frankenstein > senslog post
Posting data to http://silverblade/nextcloud/index.php/apps/sensorlogger/api/v1/createlog/
{
        "deviceId":     "frankenstein-99dc9e",
        "data": [{
                        "dataTypeId":   1,
                        "value":        "3180.00"
                }]
}
http status: 200
server response: null        
```

- Schedule periodic data submission using `every` frankenstein command

```
frankenstein > every 60 'senslog post'
Scheduling [senslog post] to execute every 60 seconds
Timer armed, 1000ms

frankenstein >
frankenstein > senslog post
Posting data to http://silverblade/nextcloud/index.php/apps/sensorlogger/api/v1/createlog/
{
        "deviceId":     "frankenstein-99dc9e",
        "data": [{
                        "dataTypeId":   1,
                        "value":        "3178.00"
                }]
}
http status: 200
server response: null
```


# 'senslog' command

The new senslog command allows you to do the following:

- `senslog register` - Attempts to register the sensor and obtain datatype ids.

- `senslog get_dt` - only get datatype ids without registering. You are adviced to always use register instead

- `senslog dump` - dump all current information about this sensor node

- `senslog post` - post sensor data for all registered sensors

- `senslog create type description unit` - Create a new datatype from commandline. For this datatype you will have to set current value via commandline before posting

- `senslog set type description unit current_value` - Update current sensor value from commandline. type, description and unit should be the same as those used when you called 'senslog create'.

# Environment

esp8266 sensorlogger is configured via environment variables. All variables have the 'slog-' prefix.

- slog-deviceId - sets device UUID reported to nextcloud during registration. If none is set it is generated using esp8266 chipid. UUID identifies this device.
- slog-deviceType - sets device type. default is 'dummy'
- slog-deviceGroup - sets device group. default is 'development'
- slog-deviceParentGroup - sets device parent group. Default is empty.
- slog-deviceName - sets device name

- slog-userName* - nextcloud username used to authorize
- slog-password* - sets password for authorization. Generating a 'device-password' in owncloud/nextcloud personal settings is recommended.  
- slog-nextCloudUrl* - URL to the nextcloud instance. No slash at the end of the url. e.g. https://cloud.example.com/nextcloud

\* - environment variables marked by * are absolutely required to get things up and running.

N.B. Don't forget to call saveenv to write environment to flash after any adjustments.

# How to connect sensors

There are 2 ways to do that. Either connect them directly to esp8266 (standalone) or use some MCU as a proxy, that will tell frankenstein via uart what to do.

## Standalone

In this case you have to connect your sensors directly to esp8266 and enable the required sensor via menuconfig when building frankenstein.

Currently available sensors:
    * dummy - a set of 2 dumb dummy sensors reporting 'health' an 'mana'.
    * vdd3v3 - a sensor reporting measurements of 3v3 volts at the esp8266
    * adc - a sensor reporting measurements from esp8266 internal adc.
    * ping - a sensor pinging a set of hosts from environment variable and reporting ping times

## Slave mode

In this scenario you connect your sensors to some kind of MCU (e.g. arduino), that collects the data and interacts with frankenstein via UART.

A cheap Blackboard T5 development board can be used as a simple example for this. See this (repository)[https://github.com/nekromant/blackboard-STC15] for complete example firmware that will feed your nextcloud with temperature readings.

This way you can keep the power-hungry esp8266 offline most of the time, switching it on only to deliver the payload of sensor readings.

(This place in this file)[https://github.com/nekromant/blackboard-STC15/blob/master/nextcloud-sensorlogger.c#L26] will serve you as a good start.

# TODO:

- Add more i2c sensors to the sensorlogger infrastructure
- Create some awesome custom hardware for this thing.
- ...
- PROFIT 
