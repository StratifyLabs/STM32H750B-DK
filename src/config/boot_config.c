
#include <cortexm/task.h>
#include <sos/debug.h>
#include <sos/sos.h>
#include <stm32/stm32_config.h>
#include <stm32/stm32_types.h>

#include "config.h"
#include "boot_config.h"

#if _IS_BOOT

int boot_is_bootloader_requested(){

  //check for HW request for the bootloader

  //check for SW request for the bootloader

  return 0;
}


void boot_invoke_bootloader(void * args){
  MCU_UNUSED_ARGUMENT(args);
}

#endif
