/* Copyright (c) 2014 Ivan Grokhotkov. All rights reserved. 
 * This file is part of the atproto AT protocol library
 *
 * Redistribution and use is permitted according to the conditions of the
 * 3-clause BSD license to be found in the LICENSE file.
 */

// #include "dce_common.h"
// #include "dce_private.h"
#include "c_types.h"
#include "spi_flash.h"
#include "config_store.h"
#include "user_interface.h"
#include "ets_sys.h"

#define CONFIG_START_SECTOR 0x3C
#define CONFIG_SECTOR (CONFIG_START_SECTOR + 0)
#define CONFIG_ADDR (SPI_FLASH_SEC_SIZE * CONFIG_SECTOR)

#define CONFIG_WIFI_SECTOR 0x7E

static config_t s_config;
static int s_config_loaded = 0;

void ICACHE_FLASH_ATTR config_read(config_t* config)
{
    spi_flash_read(CONFIG_ADDR, (uint32*) config, sizeof(config_t));
}

void ICACHE_FLASH_ATTR config_write(config_t* config)
{
    ETS_UART_INTR_DISABLE();
    spi_flash_erase_sector(CONFIG_SECTOR);
    spi_flash_write(CONFIG_ADDR, (uint32*) config, sizeof(config_t));
    ETS_UART_INTR_ENABLE();
}

config_t* ICACHE_FLASH_ATTR config_get()
{
    if (!s_config_loaded)
    {
        config_read(&s_config);
        s_config_loaded = 1;
    }
    return &s_config;
}

void ICACHE_FLASH_ATTR config_save()
{
    config_write(&s_config);
    config_t tmp;
    config_read(&tmp);
    if (memcmp(&tmp, &s_config, sizeof(config_t)) != 0)
    {
        // DCE_FAIL("config verify failed");
    }
}

config_t* ICACHE_FLASH_ATTR config_init()
{
    config_t* config = config_get();
    if (config->magic != CONFIG_MAGIC || config->version != CONFIG_VERSION)
    {
        config_init_default();
    }
    return config;
}

void ICACHE_FLASH_ATTR config_init_default()
{
    config_t* config = config_get();
    config->magic = CONFIG_MAGIC;
    config->version = CONFIG_VERSION;
    config->baud_rate = 9600;
    config_save();
    
    ETS_UART_INTR_DISABLE();
    spi_flash_erase_sector(CONFIG_WIFI_SECTOR);
    ETS_UART_INTR_ENABLE();
}

