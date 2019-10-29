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


#ifndef CONFIG_H_
#define CONFIG_H_

#include <mcu/arch.h>
#include "sl_config.h"

#include "board_config.h"

//openocd -f interface/stlink-v2-1.cfg -f target/stm32h7x.cfg -c "program ./STM32H750B-DK/build_boot_debug/STM32H750B-DK.bin 0x08000000; reset run; exit;"


#define SOS_BOARD_SYSTEM_CLOCK 400000000
#if _IS_BOOT
#define SOS_BOARD_SYSTEM_MEMORY_SIZE (32*1024)
#else
#define SOS_BOARD_SYSTEM_MEMORY_SIZE (64*1024)
#endif
#define SOS_BOARD_ID SL_CONFIG_DOCUMENT_ID
#define SOS_BOARD_VERSION SL_CONFIG_VERSION_STRING
#define SOS_BOARD_NAME SL_CONFIG_NAME
#define SOS_BOARD_FLAGS SYS_FLAG_IS_ACTIVE_ON_IDLE

#define SOS_BOARD_USB_RX_BUFFER_SIZE 512
#define SOS_BOARD_STDIO_BUFFER_SIZE 512
#define SOS_BOARD_TMR 1

//Total number of tasks (threads) for the entire system
#define SOS_BOARD_TASK_TOTAL 10
#define SOS_BOARD_EVENT_HANDLER board_event_handler
#define SOS_BOARD_TRACE_EVENT board_trace_event

//--------------------------------------------Symbols-------------------------------------------------

/* By defining Ignore switches, functions can be omitted from the kernel
 * This means any applications that use these functions will fail
 * to install on the BSP.
 *
 * If you are building a custom board, ignoring symbols is a good
 * way to squeeze down the kernel to only what is necessary. However,
 * if you plan on allowing your users to install applications, they
 * might find it challenging when some functions are missing (the
 * applications will compile but fail to install).
 *
 * See [sos/symbols/defines.h](https://github.com/StratifyLabs/StratifyOS/blob/master/include/sos/symbols/defines.h)
 * for all available switches.
 *
 */

#define SYMBOLS_IGNORE_DCOMPLEX 1
#define SYMBOLS_IGNORE_SOCKET 1

//#define SOS_BOARD_ARM_DSP_API_Q15 1
//#define SOS_BOARD_ARM_DSP_API_Q31 1
#define ARM_DSP_API_IGNORE_FILTER 1
#define ARM_DSP_API_IGNORE_MATRIX 0
#define ARM_DSP_API_IGNORE_CONTROLLER 1
#define ARM_DSP_API_IGNORE_DCT 1
#define SOS_BOARD_ARM_DSP_API_F32 1
#define SOS_BOARD_ARM_DSP_CONVERSION_API 1

//other ignore switches
#if 0
#define SYMBOLS_IGNORE_MATH_F 1
#define SYMBOLS_IGNORE_DOUBLE 1
#define SYMBOLS_IGNORE_STDIO_FILE 1
#define SYMBOLS_IGNORE_SIGNAL 1
#define SYMBOLS_IGNORE_PTHREAD_MUTEX 1
#define SYMBOLS_IGNORE_PTHREAD_COND 1
#define SYMBOLS_IGNORE_AIO 1
#define SYMBOLS_IGNORE_WCTYPE 1
#define SYMBOLS_IGNORE_STR 1
#define SYMBOLS_IGNORE_SEM 1
#define SYMBOLS_IGNORE_MQ 1
#endif



#endif /* CONFIG_H_ */
