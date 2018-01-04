/* Copyright (c) 2014 Ivan Grokhotkov. All rights reserved. 
 * This file is part of the atproto AT protocol library
 *
 * Redistribution and use is permitted according to the conditions of the
 * 3-clause BSD license to be found in the LICENSE file.
 */

#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#define CONFIG_MAGIC   0x42
#define CONFIG_VERSION 1

#include <stdint.h>

typedef struct {
    int32_t magic;
    int32_t version;
    int32_t baud_rate;
    // bump CONFIG_VERSION when adding new fields
} config_t;


config_t* config_get();
void config_save();
config_t* config_init();
void config_init_default();

#endif//CONFIG_STORE_H
