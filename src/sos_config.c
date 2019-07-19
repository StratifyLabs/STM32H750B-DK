/*

Copyright 2011-2018 Stratify Labs, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	 http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

 */


#include <sys/lock.h>
#include <fcntl.h>
#include <errno.h>
#include <mcu/mcu.h>
#include <mcu/debug.h>
#include <mcu/periph.h>
#include <device/sys.h>
#include <device/null.h>
#include <device/zero.h>
#include <device/random.h>
#include <device/uartfifo.h>
#include <device/usbfifo.h>
#include <device/fifo.h>
#include <device/cfifo.h>
#include <device/switchboard.h>
#include <device/sys.h>
#include <sos/link.h>
#include <sos/fs/sysfs.h>
#include <sos/fs/appfs.h>
#include <sos/fs/devfs.h>
#include <sos/fs/sffs.h>
#include <sos/sos.h>
#include <device/drive_cfi.h>


#include "config.h"
#include "link_config.h"


//--------------------------------------------Stratify OS Configuration-------------------------------------------------
const sos_board_config_t sos_board_config = {
	.clk_usecond_tmr = SOS_BOARD_TMR, //TIM2 -- 32 bit timer
	.task_total = SOS_BOARD_TASK_TOTAL,
	.stdin_dev = "/dev/stdio-in" ,
	.stdout_dev = "/dev/stdio-out",
	.stderr_dev = "/dev/stdio-out",
	.o_sys_flags = SYS_FLAG_IS_STDIO_FIFO | SYS_FLAG_IS_TRACE | SOS_BOARD_FLAGS,
	.sys_name = SL_CONFIG_NAME,
	.sys_version = SL_CONFIG_VERSION_STRING,
	.sys_id = SL_CONFIG_DOCUMENT_ID,
	.sys_memory_size = SOS_BOARD_SYSTEM_MEMORY_SIZE,
	.start = sos_default_thread,
	.start_args = &link_transport,
	.start_stack_size = SOS_DEFAULT_START_STACK_SIZE,
	.socket_api = 0,
	.request = 0,
	.git_hash = SOS_GIT_HASH,
	.trace_dev = "/dev/trace",
	.trace_event = SOS_BOARD_TRACE_EVENT
};

//This declares the task tables required by Stratify OS for applications and threads
SOS_DECLARE_TASK_TABLE(SOS_BOARD_TASK_TOTAL);

//--------------------------------------------Device Filesystem-------------------------------------------------


/*
 * Defaults configurations
 *
 * This provides the default pin assignments and settings for peripherals. If
 * the defaults are not provided, the application must specify them.
 *
 * Defaults should be added for peripherals that are dedicated for use on the
 * board. For example, if a UART has an external connection and label on the
 * board, the BSP should provide the default configuration.
 *
 *
 *
 */
UARTFIFO_DECLARE_CONFIG_STATE(uart0_fifo, 1024, 64,
										UART_FLAG_SET_LINE_CODING_DEFAULT, 8, 115200,
										0, 2,
										0, 3,
										0xff, 0xff,
										0xff, 0xff);

FIFO_DECLARE_CONFIG_STATE(stdio_in, SOS_BOARD_STDIO_BUFFER_SIZE);
FIFO_DECLARE_CONFIG_STATE(stdio_out, SOS_BOARD_STDIO_BUFFER_SIZE);

const stm32_qspi_dma_config_t qspi_dma_config = {
	.qspi_config = {
		.attr = {
			.o_flags = QSPI_FLAG_SET_MASTER,
			.freq = 60000000UL,
			.pin_assignment = {
				.data[0] = {3,11}, //PD11
				.data[1] = {5,9}, //PF9
				.data[2] = {5,7}, //PF7
				.data[3] = {5,6}, //PF6
				.sck = {5,10}, //PF10
				.cs = {6,6} //PG6
			}
		}
	},
	.dma_config = {
		.tx = {
			.dma_number = STM32_DMA2,
			.stream_number = 7,
			.channel_number = 3,
			.priority = STM32_DMA_PRIORITY_LOW,
			.o_flags = STM32_DMA_FLAG_IS_NORMAL |
			STM32_DMA_FLAG_IS_FIFO |
			STM32_DMA_FLAG_IS_MEMORY_TO_PERIPH |
			STM32_DMA_FLAG_IS_MEMORY_BYTE |
			STM32_DMA_FLAG_IS_PERIPH_BYTE
		},
		.rx = {
			.dma_number = STM32_DMA2,
			.stream_number = 7,
			.channel_number = 3,
			.priority = STM32_DMA_PRIORITY_LOW,
			.o_flags = STM32_DMA_FLAG_IS_NORMAL |
			STM32_DMA_FLAG_IS_FIFO |
			STM32_DMA_FLAG_IS_PERIPH_TO_MEMORY |
			STM32_DMA_FLAG_IS_MEMORY_BYTE |
			STM32_DMA_FLAG_IS_PERIPH_BYTE
		}
	}
};

const devfs_device_t qspi_drive_device = DEVFS_DEVICE("qspi", mcu_qspi_dma, 0, &qspi_dma_config, 0, 0666, SOS_USER_ROOT, S_IFCHR);

const drive_cfi_config_t drive_cfi_config = {
	.serial_device = &qspi_drive_device,
	.info = {
		.addressable_size = 1,
		.write_block_size = 1, //smallest available write size
		.num_write_blocks = 64*1024*1024UL,  //64MB (512Mbit)
		.erase_block_size = 4096, //smallest eraseable block
		.erase_block_time = 30000UL, //45ms typical
		.erase_device_time = 140000000UL, //140s typical
		.bitrate = 66000000UL
	},
	.opcode = {
		.write_enable = 0x06,
		.page_program = 0x02,
		.block_erase = 0x20,
		.device_erase = 0xC7,
		.fast_read = 0xEB,
		.power_up = 0xAB,
		.power_down = 0xB9,
		.enable_reset = 0x66,
		.reset = 0x99,
		.protect = 0x7E,
		.unprotect = 0x7A,
		.read_busy_status = 0x05, //busy bit is bit 0 of status register 1
		.busy_status_mask = 0x01,
		.enter_qpi_mode = 0x35,
		.enter_4byte_address_mode = 0xb7,
		.page_program_size = 256,
		.read_dummy_cycles = 6,
		.write_dummy_cycles = 0
	},
	.cs = { 0xff, 0xff },
	.qspi_flags =
	QSPI_FLAG_IS_OPCODE_QUAD |
	QSPI_FLAG_IS_DATA_QUAD |
	QSPI_FLAG_IS_ADDRESS_QUAD |
	QSPI_FLAG_IS_ADDRESS_32_BITS
};

drive_cfi_state_t drive_cfi_state MCU_SYS_MEM;

/* This is the list of devices that will show up in the /dev folder.
 */
const devfs_device_t devfs_list[] = {
	//System devices
#if !_IS_BOOT
	DEVFS_DEVICE("trace", ffifo, 0, &board_trace_config, &board_trace_state, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("fifo", cfifo, 0, &board_fifo_config, &board_fifo_state, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("null", null, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("zero", zero, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("random", random, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
#endif
	DEVFS_DEVICE("stdio-out", fifo, 0, &stdio_out_config, &stdio_out_state, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("stdio-in", fifo, 0, &stdio_in_config, &stdio_in_state, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("link-phy-usb", usbfifo, SOS_BOARD_USB_PORT, &sos_link_transport_usb_fifo_cfg, &sos_link_transport_usb_fifo_state, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("sys", sys, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),


	//MCU peripherals
	DEVFS_DEVICE("core", mcu_core, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("core0", mcu_core, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),

	DEVFS_DEVICE("i2c0", mcu_i2c, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("i2c1", mcu_i2c, 1, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("i2c2", mcu_i2c, 2, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("i2c3", mcu_i2c, 3, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),

	DEVFS_DEVICE("pio0", mcu_pio, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //GPIOA
	DEVFS_DEVICE("pio1", mcu_pio, 1, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //GPIOB
	DEVFS_DEVICE("pio2", mcu_pio, 2, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //GPIOC
	DEVFS_DEVICE("pio3", mcu_pio, 3, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //GPIOD
	DEVFS_DEVICE("pio4", mcu_pio, 4, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //GPIOE
	DEVFS_DEVICE("pio5", mcu_pio, 5, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //GPIOF
	DEVFS_DEVICE("pio6", mcu_pio, 6, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //GPIOG
	DEVFS_DEVICE("pio7", mcu_pio, 7, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //GPIOH

	DEVFS_DEVICE("tmr0", mcu_tmr, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //TIM1
	DEVFS_DEVICE("tmr1", mcu_tmr, 1, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //TIM2
	DEVFS_DEVICE("tmr2", mcu_tmr, 2, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("tmr3", mcu_tmr, 3, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("tmr4", mcu_tmr, 4, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("tmr5", mcu_tmr, 5, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("tmr6", mcu_tmr, 6, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),
	DEVFS_DEVICE("tmr7", mcu_tmr, 7, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR), //TIM8
	//Does this chip have more timers?

	DEVFS_DEVICE("uart5", mcu_uart, 5, 0, 0, 0666, SOS_USER_ROOT, S_IFCHR),

	DEVFS_TERMINATOR
};


//--------------------------------------------Root Filesystem---------------------------------------------------

/*
 * This is the root filesystem that determines what is mounted at /.
 *
 * The default is /app (for installing and running applciations in RAM and flash) and /dev which
 * provides the device tree defined above.
 *
 * Additional filesystems (such as FatFs) can be added if the hardware and drivers
 * are provided by the board.
 *
 */


const devfs_device_t mem0 = DEVFS_DEVICE("mem0", mcu_mem, 0, 0, 0, 0666, SOS_USER_ROOT, S_IFBLK);
const sysfs_t sysfs_list[] = {
#if !_IS_BOOT
	APPFS_MOUNT("/app", &mem0, SYSFS_ALL_ACCESS), //the folder for ram/flash applications
#endif
	DEVFS_MOUNT("/dev", devfs_list, SYSFS_READONLY_ACCESS), //the list of devices
	SYSFS_MOUNT("/", sysfs_list, SYSFS_READONLY_ACCESS), //the root filesystem (must be last)
	SYSFS_TERMINATOR
};


