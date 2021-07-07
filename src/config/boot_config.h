//Copyright 2021 Stratify Labs, See LICENSE.md for details

#ifndef CONFIG_BOOT_CONFIG_H
#define CONFIG_BOOT_CONFIG_H

#include "config.h"

int boot_is_bootloader_requested();
void boot_invoke_bootloader(void * args);

#endif // CONFIG_BOOT_CONFIG_H
