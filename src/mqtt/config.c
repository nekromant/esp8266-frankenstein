/*
* config.c
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "console.h"
#include "env.h"
#include "helpers.h"

#include <lib/mqtt.h>
#include <lib/config.h>
#include <lib/user_config.h>
#include <lib/debug.h>

SYSCFG sysCfg;

void ICACHE_FLASH_ATTR
CFG_Load()
{
	const uint8_t *p;
	console_printf("\r\nload MQTT configuration...\r\n");

	if ((p = (uint8_t *)env_get("mqtt_device_id")) != NULL)
		os_sprintf((char *)sysCfg.device_id, "%s", p);
	else
		os_sprintf((char *)sysCfg.device_id, MQTT_CLIENT_ID, system_get_chip_id());

	if ((p = (uint8_t *)env_get("mqtt_host")) != NULL)
		os_sprintf((char *)sysCfg.mqtt_host, "%s", p);
	else
		os_sprintf((char *)sysCfg.mqtt_host, "%s", MQTT_HOST);

	if ((p = (uint8_t *)env_get("mqtt_port")) != NULL) {
		sysCfg.mqtt_port=(uint16_t)skip_atoul((const char **)&p);	// save 0x10 by not using atoi()
	}
	else
		sysCfg.mqtt_port = MQTT_PORT;

	if ((p = (uint8_t *)env_get("mqtt_user")) != NULL)
		os_sprintf((char *)sysCfg.mqtt_user, "%s", p);
	else
		os_sprintf((char *)sysCfg.mqtt_user, "%s", MQTT_USER);

	if ((p = (uint8_t *)env_get("mqtt_pass")) != NULL)
		os_sprintf((char *)sysCfg.mqtt_pass, "%s", p);
	else
		os_sprintf((char *)sysCfg.mqtt_pass, "%s", MQTT_PASS);


	sysCfg.security = DEFAULT_SECURITY;	/* default non ssl */

	sysCfg.mqtt_keepalive = MQTT_KEEPALIVE;

	console_printf("configuration - Device_id: %s  host: %s  user: %s  pass: %s\r\n",
				sysCfg.device_id,
				sysCfg.mqtt_host,
				sysCfg.mqtt_user,
				sysCfg.mqtt_pass);
}
