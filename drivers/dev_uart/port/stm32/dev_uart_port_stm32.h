#ifndef DEV_UART_PORT_STM32_H
#define DEV_UART_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_uart_port.h"
#include "stm32h7xx_hal.h"

/* STM32 internal: 1-byte RX temp buffer for interrupt chaining */
#define DEV_UART_STM32_RX_TEMP_BYTE_COUNT  (1U)

typedef struct {
    dev_uart_id_t          uart_id;
    USART_TypeDef         *instance;
    GPIO_TypeDef          *tx_port;
    uint16_t               tx_pin;
    GPIO_TypeDef          *rx_port;
    uint16_t               rx_pin;
    uint32_t               gpio_alternate;
    dev_uart_baudrate_t    default_baudrate;
    dev_uart_data_bits_t   data_bits;
    dev_uart_stop_bits_t   stop_bits;
    dev_uart_parity_t      parity;
    dev_uart_flow_control_t flow_control;
    uint8_t               *rx_buffer;
    uint16_t               rx_buffer_size;
} dev_uart_hw_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_UART_PORT_STM32_H */
