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

#include <mcu/arch.h>
#include <mcu/mcu.h>
#include <mcu/debug.h>

#include "config.h"

//--------------------------------------------MCU Configuration-------------------------------------------------

static char stm32_usb_rx_buffer[SOS_BOARD_USB_RX_BUFFER_SIZE] MCU_SYS_MEM;

const stm32_config_t stm32_config = {
	.flash_program_millivolts = 3000,
	.usb_rx_buffer = stm32_usb_rx_buffer,
	.usb_rx_buffer_size = SOS_BOARD_USB_RX_BUFFER_SIZE
};


const mcu_board_config_t mcu_board_config = {
	.core_cpu_freq = SOS_BOARD_SYSTEM_CLOCK,
	.usb_max_packet_zero = 64,
	.debug_uart_port = 2, //USART3
	.debug_uart_attr = {
		.pin_assignment = {
			.rx = {1, 11}, //PB10
			.tx = {1, 10}, //PB11
			.cts = {0xff, 0xff},
			.rts = {0xff, 0xff}
		},
		.freq = 115200,
		.o_flags = UART_FLAG_SET_LINE_CODING_DEFAULT,
		.width = 8
	},
	.o_flags = MCU_BOARD_CONFIG_FLAG_ENABLE_CACHE,
	.event_handler = SOS_BOARD_EVENT_HANDLER,
	.led = {9, 2}, //PJ2
	.arch_config = &stm32_config,
	.o_mcu_debug =
	MCU_DEBUG_INFO |
	MCU_DEBUG_SYS |
	MCU_DEBUG_USER0 |
	MCU_DEBUG_USER1 |
	//MCU_DEBUG_MALLOC |
	MCU_DEBUG_DEVICE
};
