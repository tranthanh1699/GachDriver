#ifndef DEV_UART_PORT_STM32_H
#define DEV_UART_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_uart_port.h"

/*
 * STM32 Cube-managed UART port.
 *
 * Assumes CubeMX/CubeIDE has already configured:
 *   - GPIO alternate function for TX/RX
 *   - UART peripheral clock
 *   - NVIC priority
 *   - HAL UART init (huart1, huart2, etc.)
 *
 * This port only wraps Cube HAL handles and manages RX buffering
 * via dev_ringbuf. No GPIO/clock/NVIC configuration is performed.
 *
 * To enable:
 *   1. Uncomment #define HAL_UART_MODULE_ENABLED in stm32h7xx_hal_conf.h
 *   2. Add stm32h7xx_hal_uart.c to STM32_Drivers library
 *   3. Ensure huartX handles are declared extern in your project
 */

#define DEV_UART_STM32_CUBE_MANAGED_HW_INIT  (1U)
#define DEV_UART_STM32_RX_TEMP_BYTE_COUNT    (1U)

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32h7xx_hal.h"

typedef struct {
    dev_uart_id_t          uart_id;
    UART_HandleTypeDef    *handle;
    dev_uart_baudrate_t    default_baudrate;
    uint8_t               *rx_buffer;
    uint16_t               rx_buffer_size;
} dev_uart_hw_t;

/* Port ISR dispatch — call from HAL_UART_RxCpltCallback */
void dev_uart_port_stm32_rx_cplt_callback(UART_HandleTypeDef *huart);

#endif /* HAL_UART_MODULE_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* DEV_UART_PORT_STM32_H */
