#ifndef DEV_UART_PORT_NRF52_H
#define DEV_UART_PORT_NRF52_H
#ifdef __cplusplus
extern "C" {
#endif
#include "dev_uart_port.h"
typedef struct { dev_uart_id_t uart_id; uint8_t inst; uint32_t tx; uint32_t rx; dev_uart_baudrate_t baud; uint8_t *rxb; uint16_t rxs; } dev_uart_hw_t;
#ifdef __cplusplus
}
#endif
#endif
