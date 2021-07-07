
#include <cortexm/task.h>
#include <sos/debug.h>
#include <sos/sos.h>
#include <stm32/stm32_config.h>
#include <stm32/stm32_types.h>

#include <stm32h7xx/stm32h7xx_hal_uart_ex.h>

#include "config.h"
#include "debug_config.h"

#if ___debug
static UART_HandleTypeDef m_huart MCU_SYS_MEM;
#endif

#define DEBUG_LED_PORT GPIOJ
#define DEBUG_LED_PINMASK (1 << 2)

void debug_initialize() {

  GPIO_InitTypeDef gpio_init;
  gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init.Pin = DEBUG_LED_PINMASK;
  gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DEBUG_LED_PORT, &gpio_init);
  debug_disable_led();



#if ___debug
  __HAL_RCC_USART3_CLK_ENABLE();

  {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    PeriphClkInitStruct.Usart234578ClockSelection =
        RCC_USART234578CLKSOURCE_D2PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
  }

  /**USART3 GPIO Configuration
    PB11     ------> USART3_TX
    PB10     ------> USART3_RX
    */
  gpio_init.Pin = (1 << 10) | (1 << 11);
  gpio_init.Mode = GPIO_MODE_AF_PP;
  gpio_init.Pull = GPIO_PULLUP;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOB, &gpio_init);

  m_huart.Instance = USART3;
  m_huart.Init.BaudRate = 400000;
  m_huart.Init.WordLength = UART_WORDLENGTH_8B;
  m_huart.Init.StopBits = UART_STOPBITS_1;
  m_huart.Init.Parity = UART_PARITY_NONE;
  m_huart.Init.Mode = UART_MODE_TX_RX;
  m_huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  m_huart.Init.OverSampling = UART_OVERSAMPLING_16;
  m_huart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  m_huart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  m_huart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  HAL_UART_Init(&m_huart);

  if (HAL_UARTEx_SetTxFifoThreshold(&m_huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    ;
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&m_huart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    ;
  }
  if (HAL_UARTEx_DisableFifoMode(&m_huart) != HAL_OK)
  {
    ;
  }

  u32 clocksource;
  UART_GETCLOCKSOURCE((&m_huart), clocksource);
  sos_debug_printf("UART Clock source %d\n",  clocksource );

  u32 clock_value = HAL_RCC_GetPCLK1Freq();
  sos_debug_printf("UART Clock value %d\n",  clock_value );
  clock_value = HAL_RCC_GetHCLKFreq();
  sos_debug_printf("HCLK value %d\n",  clock_value );
  clock_value = HAL_RCC_GetSysClockFreq();
  sos_debug_printf("Sys Clock value %d\n",  clock_value );

#endif

#if 0
  debug_enable_led();
  const char message[] = "Hello2\n";
  while(1){
    debug_write(message, sizeof(message)-1);
    cortexm_delay_ms(200);
  }
#endif


}

void debug_write(const void *buf, int nbyte) {
#if ___debug
  const u8 *cbuf = buf;
  for (int i = 0; i < nbyte; i++) {
    u8 c = cbuf[i];
    HAL_UART_Transmit(&m_huart, &c, 1, HAL_MAX_DELAY);
  }
#endif
}

void debug_enable_led() {
  HAL_GPIO_WritePin(DEBUG_LED_PORT, DEBUG_LED_PINMASK, GPIO_PIN_RESET);
}

void debug_disable_led() {
  HAL_GPIO_WritePin(DEBUG_LED_PORT, DEBUG_LED_PINMASK, GPIO_PIN_SET);
}

#define TRACE_COUNT 16
#define TRACE_FRAME_SIZE sizeof(link_trace_event_t)
#define TRACE_BUFFER_SIZE (sizeof(link_trace_event_t) * TRACE_COUNT)
static char trace_buffer[TRACE_FRAME_SIZE * TRACE_COUNT];
const ffifo_config_t debug_trace_config = {.frame_count = TRACE_COUNT,
                                           .frame_size =
                                               sizeof(link_trace_event_t),
                                           .buffer = trace_buffer};
ffifo_state_t debug_trace_state;

void debug_trace_event(void *event) {
  link_trace_event_header_t *header = event;
  devfs_async_t async;
  const devfs_device_t *trace_dev = &(sos_config.fs.devfs_list[0]);

  // write the event to the fifo
  memset(&async, 0, sizeof(devfs_async_t));
  async.tid = task_get_current();
  async.buf = event;
  async.nbyte = header->size;
  async.flags = O_RDWR;
  trace_dev->driver.write(&(trace_dev->handle), &async);
}
