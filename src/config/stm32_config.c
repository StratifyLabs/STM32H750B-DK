
#include <sos/arch.h>
#include <stm32/stm32_config.h>

#include "config.h"

//--------------------------------------------MCU
// Configuration-------------------------------------------------

#define USB_RX_BUFFER_SIZE 1024

static char stm32_usb_rx_buffer[USB_RX_BUFFER_SIZE] MCU_SYS_MEM;

//STM32H750 has 4KB of USB RAM
//each word is 32-bits
//128words = 512 bytes

const stm32_config_t stm32_config = {
    .flash_program_millivolts = 3300,
    .usb = {
        .rx_buffer = stm32_usb_rx_buffer,
        .rx_buffer_size = USB_RX_BUFFER_SIZE,
        .rx_fifo_word_size = 128,   // RX fifo for all endpoints
        .tx_fifo_word_size[0] = 64, // TX endpoint 0 (control)
        .tx_fifo_word_size[1] = 64, // TX endpoint 1 (unused)
        .tx_fifo_word_size[2] = 64,  // TX endpoint 2 (bulk endpoint)
        .tx_fifo_word_size[3] = 0,  // TX endpoint 3
        .tx_fifo_word_size[4] = 0,  // TX endpoint 4
        .tx_fifo_word_size[5] = 0,  // TX endpoint 5
        .tx_fifo_word_size[6] = 0,  // TX endpoint 6
        .tx_fifo_word_size[7] = 0,  // TX endpoint 7
        .tx_fifo_word_size[8] = 0,  // TX endpoint 8
    }};
#if 0
MCU_DECLARE_SECRET_KEY_32(secret_key)

const mcu_board_config_t mcu_board_config = {
    .core_cpu_freq = SOS_BOARD_SYSTEM_CLOCK,
    .usb_max_packet_zero = 64,
    .debug_uart_port = 2,
    .debug_uart_attr = {.pin_assignment = {.rx = {3, 9},
                                           .tx = {3, 8},
                                           .cts = {0xff, 0xff},
                                           .rts = {0xff, 0xff}},
                        .freq = 115200,
                        .o_flags = UART_FLAG_SET_LINE_CODING_DEFAULT,
                        .width = 8},
    .secret_key_address = secret_key,
    .secret_key_size = 32,
    .o_flags = MCU_BOARD_CONFIG_FLAG_LED_ACTIVE_HIGH,
    .event_handler = SOS_BOARD_EVENT_HANDLER,
    .led = {1, 7},
    .arch_config = &stm32_config,
    .o_mcu_debug = SOS_BOARD_DEBUG_FLAGS};
#endif
