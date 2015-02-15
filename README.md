#ABOUT

Frankenstein is a quick and dirty firmware, made from different bits
and pieces (thus the name) for ESP8266. 
The features: 

* A nice and shiny commandline interface that reminds of u-boot. 
  No more sloppy AT commands. 
* Full command line editing and history. 
* A very easy way of adding new commands to the shell 
* More or less clean code
* Highly configurable (sort of)
* Supports firmware updates over tftp 

You can start by grabbing a binary and burning it to your device. 
Remember about backups, for all your data on the flash will be gone. 

#Terminal software

Please note, to use proper commandline editing, use a SANE serial terminal
emulator. 

Linux: minicom, screen, kermit
Windows: putty

Do not file bugs telling me that commandline editing doesn't work in, say, 
coolterm.

If you are communicating with frankenstein with some other hardware, here's some 
technical stuff for you: 

* All lines you send to ESP8266 should end with CR, e.h. "\r". 

* CTRL+C tries to interrupt the running command and unlocks the terminal


#Getting to know frankenstein. The basics

Now that we're clear with that, let's start the device and open up terminal.
You'll see something like this: 

```
Frankenstein ESP8266 Firmware
Powered by Antares 0.2-rc1, Insane Mushroom
(c) Andrew 'Necromant' Andrianov 2014 <andrew@ncrmnt.org>
This is free software (where possible), published under the terms of GPLv2

Memory Layout:
data  : 0x3ffe8000 ~ 0x3ffe8f60, len: 3936
rodata: 0x3ffe8f60 ~ 0x3ffeae58, len: 7928
bss   : 0x3ffeae58 ~ 0x3fff2720, len: 30920
heap  : 0x3fff2720 ~ 0x3fffc000, len: 39136
hello=23env: Environment @ 0x0007b000 size 0x00001000 bytes (0x00000ffc real) 
env: Bad CRC (fe1 vs ffff) using defaults
Saving environment to flash disabled by config
Recompile with CONFIG_ENV_NOWRITE=n
=== Current environment ===
sta-mode    dhcp
default-mode  STA
sta-ip      192.168.0.123
sta-mask    255.255.255.0
sta-gw      192.168.0.1
ap-ip       192.168.1.1
ap-mask     255.255.255.0
ap-gw       192.168.1.1
hostname    frankenstein
bootdelay   5
dhcps-enable  1
=== 201/4092 bytes used ===
dhcpserver: started

 === Press enter to activate this console === 

frankenstein > 
```

The workflow is simple. You type commands, frankenstein does things. Simple?

#Environment

You can store come configuration parameters in 'environment'. 
Environment is just a key=value storage. Certain variables affect behavior of different commands.  
You can list environment with printenv

```
=== Current environment ===
sta-mode    dhcp
default-mode  STA
sta-ip      192.168.0.123
sta-mask    255.255.255.0
sta-gw      192.168.0.1
ap-ip       192.168.1.1
ap-mask     255.255.255.0
ap-gw       192.168.1.1
hostname    frankenstein
bootdelay   5
dhcps-enable  1
telnet-port  23
telnet-autostart  1
telnet-drop  60
tftp-server  192.168.1.215
tftp-dir    /
tftp-file   antares.rom
=== 309/4092 bytes used ===
```

You can set environment values with setenv.

`frankenstein > setenv key value`

You can get environment values with getenv. e.g.

```
frankenstein > getenv key
value
```

All changes are made in ram. To save them to flash run 
saveenv and they will survive reboot. 

Environment variables hold paramers for the firmware that are
applied on boot or affect certain commands.
All variables are described in README.env. You can also store arbitary data 
in environment variables. 


#Wireless modes

Wireless modes can be switched using iwmode command
modes are: NONE, STA, AP, APSTA
Default mode is specified in default-mode environment variable
Use iwmode to switch manually:

```
frankenstein > iwmode STA
mode change: AP->STA
```

#Scanning


```
frankenstein > iwscan
BSSID b0:48:7a:d6:92:a8 channel 11 rssi -88 auth WPA2_PSK     TP-LINK_D692A8
BSSID c0:4a:00:c7:9d:8e channel 11 rssi -80 auth WPA_WPA2_PSK Home_TP-LINK
```

#Connecting to an AP

```
frankenstein > iwconnect apname password
Connected
```

iwconnect starts connection process and waits for some seconds until it 
either connects or an error rises. It will continue to try and reconnect in
background. 

#Checking connection info

ifconfig prints the info about curently active interfaces. they are names ap0 and sta0. 
They do not correspond (yet!) to lwip iface names. 
 
```
frankenstein > ifconfig
ap0: WiFi Access Point Interface
     state: Running
     inet addr:192.168.4.1 Mask:255.255.255.0 Gateway:192.168.4.1 
sta0: WiFi Client Interface
     state   : Connected
     rssi    : -60
     channel : 3
     inet addr:192.168.0.198 Mask:255.255.255.0 Gateway:192.168.0.20 

```

#Configuring an AP

apconfig with no arguments shows current ap configuration.

```
frankenstein > apconfig
SSID: arven AUTH WPA2_PSK BSSID: 1a:fe:34:98:dc:9e

frankenstein > apconfig myap WPA2_PSK 12345678

frankenstein > apconfig
SSID: myap AUTH WPA2_PSK BSSID: 1a:fe:34:98:dc:9e

```

To enable DHCP server set 'dhcps-enable' to '1' and reboot. 
To start AP on boot set 'default-mode' to 'AP' or 'APSTA'


#listening for data

A very simple TCP test command. It will liste on a port and print out the line 
of text received. '\n' terminates connection. 
BUG: As of 0.9.2 SDK it's impossible to terminate a running TCP listener. Say thanks
to espressif! May be 0.9.3 will fix it.

```
frankenstein > listen 8080
Listening (TCP) on port 8080
connect    | 192.168.0.101:60804 
receive    | hello
disconnect | 192.168.0.101:60803 
```

#Sending out data. 
A very simple TCP test command. It will connect to a remote host, write a string
and disconnect. Disconnect (for some reason) takes a while. 

```
frankenstein > send 192.168.0.101 8080 hello
connected!
data sent
disconnected!

```

#A full list of scary commands available is listed below. 

```
help       - Show this message
argtest    - Print out argc/argv
deepsleep  - Enter deep sleep for some microseconds
             deepsleep 10000
reset      - Soft-reboot the device 
chipinfo   - Display chip information
meminfo    - Display memory information
hname      - Set dhcp hostname
envreset   - Reset environment to defaults
             resetenv
saveenv    - Write environment to flash
             setenv var value
setenv     - Set an environment variable
             setenv var value
printenv   - Print all environment variables
apconfig   - Setup Access Point. 
             apconfig name OPEN/WEP/WPA_PSK/WPA2_PSK/WPA_WPA2_PSK [password]
iwconnect  - Join a network/Display connection status. 
             iwconnect ssid password
iwmode     - Get/set wireless mode. Available modes: NONE, STA, AP, APSTA
             iwmode STA
iwscan     - Scan for available stations
ifconfig   - Show network interfaces and their status
             ifconfig [iface]
             ifconfig sta0
gpio       - Control gpio lines. gpio mode line [value] 
             gpio in 0
             gpio out 0 1
flash_scan - Scan the upper portion of FLASH for dirty blocks
             Used to find out where the blobs store config
             flash_scan
spi_dump   - Hexdump flash contents
             spi_dump start len
wipeparams - Wipe blob configuration section of flash
             wipeparams any three arguments
spi_wipe   - Wipe the whole spi flash blank
             wipe any three arguments
listen     - Listen for incoming data ona port
             listen 8080
send       - Send data to a remote host. 
             send hostname port [data]
ds18b20    - Read temperature from DS18B20 chip.
             ds18b20 <gpio>
 
```

# Over-the-air firmware updates. 

Frankenstein supports OTA updates via tftp.
TFTP update requirements: 

* Firmware you're flashing is <=256KiB (SPI_FLASH_SIZE /2 )
* Firmware is made up of _ONE_ file that will be burned at 0x0000

To update use the following: 

* Set up a tftp server on the host pc. e.g. tftpd-hpa.   
* Set 'tftp-server' environment variable to the tftp server ip address. 
* Set 'tftp-dir' and 'tftp-file' to point to your firmware file. 
e.g. To get /tftpboot/antares.rom you'll need

setenv tftp-dir /tftpboot/
setenv tftp-file antares.rom

* (optional) saveenv

* tftp

If everything goes well - you'll be running your new firmware in a few seconds.  
tftp doesn't check what stuff you have in your image, so if 

# Known bugs

* iwscan just hangs and iwconnect never connects to an access point

This happens right after flashing Frankenstein for the first time. Power-cycle 
(e.g remove and apply power, not just soft-reset) and it will work. This is a known 
bug, I'm working on a fix. 

* Firmware is unstable and randomly reboots.

Just after flashing run wipeparams. Stored settings from older SDK tend to confuse 
Espressif's blobs and make them go nuts

* No way to connect to AP that has whitespaces in its name or password/No whitespace escaping 

Known limitation of microrl. Is on the TODO list

* gpio controls only gpio0 and gpio2

Known limitation, since only these are broken out on my modules. It's somewhere on my TODO list.