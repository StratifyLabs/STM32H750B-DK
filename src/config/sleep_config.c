

#include "config.h"

void sleep_idle() {

  //H750 can only sleep when not connected to USB

  //__WFI();
}

void sleep_hibernate(int seconds) {
  MCU_UNUSED_ARGUMENT(seconds);
  // set deep sleep
  __WFI();
}

void sleep_powerdown() {
  // set deep sleep with standby
  __WFI();
}
