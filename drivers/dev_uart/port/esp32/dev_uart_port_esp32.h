#ifndef DEV_UART_PORT_ESP32_H
#define DEV_UART_PORT_ESP32_H
#ifdef __cplusplus
extern "C" {
#endif
#include "dev_uart_port.h"

typedef struct {
    dev_uart_id_t       uart_id;  int uart_port;  int tx_gpio;  int rx_gpio;
    dev_uart_baudrate_t default_baudrate;  uint16_t rx_buf_size;  uint16_t tx_buf_size;
} dev_uart_hw_t;

#ifdef __cplusplus
}
#endif
#endif
