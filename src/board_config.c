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

#include <string.h>
#include <fcntl.h>
#include <sos/sos.h>
#include <mcu/debug.h>
#include <cortexm/task.h>
#include <sos/link/types.h>
#include <mcu/bootloader.h>
#include <sos/dev/drive.h>
#include <sos/crypt_api.h>
#include <mcu/tmr.h>
#include <mcu/arch.h>
#include <mcu/arch/stm32/stm32h7xx/stm32h7xx_hal_pwr.h>

#include "board_config.h"
#include "link_config.h"

#define TRACE_COUNT 8
#define TRACE_FRAME_SIZE sizeof(link_trace_event_t)
#define TRACE_BUFFER_SIZE (sizeof(link_trace_event_t)*TRACE_COUNT)
static char trace_buffer[TRACE_FRAME_SIZE*TRACE_COUNT];
const ffifo_config_t board_trace_config = {
	.frame_count = TRACE_COUNT,
	.frame_size = sizeof(link_trace_event_t),
	.buffer = trace_buffer
};
ffifo_state_t board_trace_state;

extern void SystemClock_Config();

void board_trace_event(void * event){
	link_trace_event_header_t * header = event;
	devfs_async_t async;
	const devfs_device_t * trace_dev = &(devfs_list[0]);

	//write the event to the fifo
	memset(&async, 0, sizeof(devfs_async_t));
	async.tid = task_get_current();
	async.buf = event;
	async.nbyte = header->size;
	async.flags = O_RDWR;
	trace_dev->driver.write(&(trace_dev->handle), &async);
}

#if _IS_BOOT
static int load_kernel_image();
static void svcall_is_bootloader_requested(void * args);

const bootloader_board_config_t boot_board_config = {
	.sw_req_loc = 0x20002000, //needs to reside in RAM that is preserved through reset and available to both bootloader and OS
	.sw_req_value = 0x55AA55AA, //this can be any value
	.program_start_addr = 0x24000000, //RAM image starts here
	.hw_req.port = 0xff, .hw_req.pin = 0xff,
	.o_flags = 0,
	.link_transport_driver = 0,
	.id = 1,
};


static void * stack_ptr;
static void (*app_reset)();
#define SYSTICK_CTRL_TICKINT (1<<1)

static void erase_flash_image(int signo){

	int result;
	mcu_debug_log_info(MCU_DEBUG_USER0, "signal %d", signo);

	//execute an erase on /dev/drive1 so that a new image can be installed
	int fd = open("/dev/drive1", O_RDWR);
	if( fd < 0 ){
		mcu_debug_log_error(MCU_DEBUG_USER0, "failed to open drive1");
		return;
	}


	drive_info_t info;

	ioctl(fd, I_DRIVE_GETINFO, &info);

	drive_attr_t attributes;

	for(
		 int i=0;
		 i < info.num_write_blocks;
		 i += info.erase_block_size
		 ){

		if( (i % (16*info.erase_block_size)) == 0 ){
			cortexm_svcall(sos_led_svcall_enable, 0);
		}

		mcu_debug_printf("Erase block at address %ld\n", i);
		while( ioctl(fd, I_DRIVE_ISBUSY) > 0 ){
			usleep(info.erase_block_time);
		}

		attributes.o_flags = DRIVE_FLAG_ERASE_BLOCKS;
		attributes.start = i;
		attributes.end = i;
		if( (result = ioctl(fd, I_DRIVE_SETATTR, &attributes)) < 0 ){
			mcu_debug_log_info(
						MCU_DEBUG_USER0,
						"failed to erase drive (%d, %d)",
						result, errno);
			break;
		}

		cortexm_svcall(sos_led_svcall_disable, 0);
	}

	while( ioctl(fd, I_DRIVE_ISBUSY) > 0 ){
		usleep(info.erase_block_time);
	}

	u8 read_buffer[16];
	u32 not_blank_count = 0;
	while(
		(result = read(fd, read_buffer, 16)) > 0
			){
		for(u32 i=0; i < 16; i++){
			if( read_buffer[i] != 0xff ){
				not_blank_count++;
			}
		}
	}

	if( not_blank_count > 0 ){
		mcu_debug_printf("Not blank: %ld\n", not_blank_count);
	}

	close(fd);

}

static void execute_ram_image(int signo){
	MCU_UNUSED_ARGUMENT(signo);

	if( signo == SIGCONT ){
		mcu_debug_log_info(MCU_DEBUG_USER0, "RAM exec requested");
		sleep(1);
	}

	sos_link_transport_usb_close(&link_transport.handle);
	cortexm_svcall((cortexm_svcall_t)cortexm_set_privileged_mode, 0);

	//shutdown the system timer
	devfs_handle_t tmr_handle;
	tmr_handle.port = SOS_BOARD_TMR;
	mcu_tmr_close(&tmr_handle);

	u32 * start_of_program = (u32*)(boot_board_config.program_start_addr);
	stack_ptr = (void*)start_of_program[0];
	app_reset = (void (*)())( start_of_program[1] );

	devfs_handle_t uart_handle;
	uart_handle.port = 1;
	mcu_uart_close(&uart_handle);

	//turn off MPU, disable interrupts and SYSTICK
	cortexm_reset_mode();
	mcu_core_disable_cache();

	//assign the value of the MSP based on the RAM image, currently the PSP is in use so it can be a call
	cortexm_set_stack_ptr(stack_ptr);

	/*
	 * Stop using the PSP. This must be inlined. If a call was made,
	 * the return would cause problems because lr would be popped
	 * from the wrong stack.
	 */
	register u32 control;
	control = __get_CONTROL();
	control &= ~0x03;
	__set_CONTROL(control);

	mcu_core_clean_data_cache();

	/*
	 * This will start executing the OS that is currently in RAM.
	 *
	 */
	app_reset();

	//should never get here
	sos_led_root_error(0);
}

#endif

void board_event_handler(int event, void * args){
	switch(event){
		case MCU_BOARD_CONFIG_EVENT_ROOT_TASK_INIT:
			break;

		case MCU_BOARD_CONFIG_EVENT_ROOT_FATAL:

			//start the bootloader on a fatal event
			//mcu_core_invokebootloader(0, 0);
			if( args != 0 ){
				mcu_debug_log_fatal(MCU_DEBUG_SYS, "Fatal Error %s", (const char*)args);
			} else {
				mcu_debug_log_fatal(MCU_DEBUG_SYS, "Fatal Error unknown");
			}
			while(1){
				;
			}
			break;

		case MCU_BOARD_CONFIG_EVENT_ROOT_INITIALIZE_CLOCK:
			SystemClock_Config();
			//enable backup SRAM
			HAL_PWR_EnableBkUpAccess();
			HAL_PWREx_EnableBkUpReg();
			break;

		case MCU_BOARD_CONFIG_EVENT_START_INIT:
			break;

		case MCU_BOARD_CONFIG_EVENT_START_LINK:
			mcu_debug_log_info(MCU_DEBUG_USER0, "Start LED");
#if _IS_BOOT
			int is_bootloader_requested;
			cortexm_svcall(
						svcall_is_bootloader_requested,
						&is_bootloader_requested
						);

			if( (is_bootloader_requested == 0) &&
				 load_kernel_image() == 0 ){
				execute_ram_image(0);
			}

			mcu_debug_log_info(
						MCU_DEBUG_USER0,
						"bootloader running"
						);

			signal(SIGALRM, erase_flash_image);
			signal(SIGCONT, execute_ram_image);

			for(u32 i=0; i < 4; i++){
				cortexm_svcall(sos_led_svcall_enable, 0);
				usleep(50*1000);
				cortexm_svcall(sos_led_svcall_disable, 0);
				usleep(50*1000);
			}
			mcu_debug_log_info(
						MCU_DEBUG_USER0,
						"ready to start link"
						);
#else
			sos_led_startup();
#endif
			break;

		case MCU_BOARD_CONFIG_EVENT_START_FILESYSTEM:
			break;
	}
}

#if _IS_BOOT

typedef struct {
	u32 offset;
	const void * src;
	u32 size;
} copy_block_t;

void copy_block(void * args){
	copy_block_t * p = args;
	memcpy((void*)(0x24000000 + p->offset), p->src, p->size);
}

int load_kernel_image(){
	int image_fd;
	void * hash_context;
	u8 hash_digest[256/8];
	u8 check_hash_digest[256/8];
	const crypt_hash_api_t * hash_api =
			kernel_request_api(CRYPT_SHA256_API_REQUEST);

	if( hash_api == 0 ){
		mcu_debug_log_error(MCU_DEBUG_USER0, "hash API is missing");
		return -1;
	}

	if( hash_api->init(&hash_context) < 0 ){
		mcu_debug_log_error(MCU_DEBUG_USER0, "failed to init hash context");
		return -1;
	}

	image_fd = open("/dev/drive1", O_RDONLY);
	if( image_fd < 0 ){
		mcu_debug_log_warning(MCU_DEBUG_USER0, "debug image not found");
		hash_api->deinit(&hash_context);
		return -1;
	}

	//need to disable MPU
	u8 buffer[256];
	int result = 0;
	copy_block_t args;

	const u32 max_image_size = 524256;
	int page_size;
	args.offset = 0;
	args.size = 256;
	args.src = buffer;
	hash_api->start(hash_context);

	do {
		if( max_image_size - args.offset > 256 ){
			page_size = 256;
		} else {
			page_size = max_image_size - args.offset;
		}
		result = read(image_fd, buffer, page_size);

		if( result > 0 ){
			hash_api->update(
						hash_context,
						buffer,
						result
						);
			args.size = result;
			cortexm_svcall(copy_block, &args);
			args.offset += result;
		}

	} while(
			  (result > 0) &&
			  (args.offset < max_image_size)
			  );

	hash_api->finish(
				hash_context,
				hash_digest,
				256/8
				);

	hash_api->deinit(
				&hash_context
				);

	//make sure the image is valid by checking the hash
	result = read(
				image_fd,
				check_hash_digest,
				sizeof(check_hash_digest)
				);

	close(image_fd);

	if( result == sizeof(check_hash_digest) ){
		if( memcmp(hash_digest, check_hash_digest, 256/8) != 0 ){
			mcu_debug_log_info(
						MCU_DEBUG_USER0,
						"hash is bad from %ld bytes",
						args.offset);
			for(u32 i=0; i < 256/(8); i++){
				mcu_debug_printf("%d = 0x%X == 0x%X\n", i, hash_digest[i], check_hash_digest[i]);
			}
			return -1;
		}
	}

	return 0;
}

void exec_bootloader(void * args){
	//write SW location with key and then reset
	//this is execute in privileged mode
	u32 * bootloader_request_address = (u32*)0x30020000;
	*bootloader_request_address = 0xaabbccdd;
	mcu_core_clean_data_cache();

	cortexm_reset(0);
}

void svcall_is_bootloader_requested(void * args){
	int * is_requested = args;
	u32 * bootloader_request_address = (u32*)0x30020000;

	if( *bootloader_request_address == 0xaabbccdd ){
		*bootloader_request_address = 0;
		mcu_core_clean_data_cache();
		*is_requested = 1;
	} else {
		*is_requested = 0;
	}
}

void boot_event(int event, void * args){
	mcu_board_execute_event_handler(event, args);
}


const bootloader_api_t mcu_core_bootloader_api = {
	.code_size = (u32)&_etext,
	.exec = exec_bootloader,
	.usbd_control_root_init = 0,
	.event = boot_event
};
#endif

