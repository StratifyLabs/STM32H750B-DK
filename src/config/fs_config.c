
#include <sos/dev/appfs.h>
#include <device/appfs.h>
#include <sos/fs/appfs.h>

#include <mcu/flash.h>

#include "config.h"
#include "fs_config.h"
#include "devfs_config.h"

#define RAM_PAGES (320 - CONFIG_SYSTEM_MEMORY_SIZE / 1024UL)
#define FLASH_START (0x08000000 + 2 * 16 * 1024UL)
#define RAM_START (0x20000000 + CONFIG_SYSTEM_MEMORY_SIZE)

// Application Filesystem ------------------------------------------
static u32 ram_usage_table[APPFS_RAM_USAGE_WORDS(RAM_PAGES)] MCU_SYS_MEM;

// flash doesn't need config or state
static const devfs_device_t flash0 =
    DEVFS_DEVICE("flash0", mcu_flash, 0, 0, 0, 0666, SYSFS_ROOT, S_IFBLK);

const appfs_mem_config_t appfs_mem_config = {
    .usage_size = sizeof(ram_usage_table),
    .section_count = 4,
    .usage = ram_usage_table,
    .flash_driver = &flash0,
    .sections = {
        {.o_flags = MEM_FLAG_IS_FLASH,
         .page_count = 2,
         .page_size = 16 * 1024UL,
         // skip the first 2 pages for the bootloader
         .address = FLASH_START},
        {.o_flags = MEM_FLAG_IS_FLASH,
         .page_count = 1,
         .page_size = 64 * 1024UL,
         .address = FLASH_START + 2 * 16 * 1024UL},
        {.o_flags = MEM_FLAG_IS_FLASH,
         .page_count = 1,
         .page_size = 128 * 1024UL,
         .address = FLASH_START + 2 * 16 * 1024UL + 64 * 1024UL},
        // the last 2 128K flash pages are used for the OS
        {.o_flags = MEM_FLAG_IS_RAM,
         .page_count = RAM_PAGES,
         .page_size = MCU_RAM_PAGE_SIZE,
         .address = RAM_START},
    }};

const devfs_device_t mem0 = DEVFS_DEVICE(
    "mem0", appfs_mem, 0, &appfs_mem_config, 0, 0666, SYSFS_ROOT, S_IFBLK);

// Root Filesystem---------------------------------------------------

/*
 * This is the root filesystem that determines what is mounted at /.
 *
 * The default is /app (for installing and running applciations in RAM and
 * flash) and /dev which provides the device tree defined above.
 *
 * Additional filesystems (such as FatFs) can be added if the hardware and
 * drivers are provided by the board.
 *
 */

const sysfs_t sysfs_list[] = {
    // managing applications
#if !_IS_BOOT
    APPFS_MOUNT("/app", &mem0, 0777, SYSFS_ROOT),
#endif
    // the list of devices
    DEVFS_MOUNT("/dev", devfs_list, 0777, SYSFS_ROOT),
    // root mount
    SYSFS_MOUNT("/", sysfs_list, 0777, SYSFS_ROOT), SYSFS_TERMINATOR};
