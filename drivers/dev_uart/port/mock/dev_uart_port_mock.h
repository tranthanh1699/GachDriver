#ifndef DEV_UART_PORT_MOCK_H
#define DEV_UART_PORT_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_uart_port.h"

void dev_uart_port_mock_reset(void);
void dev_uart_port_mock_set_error(dev_err_t error);
void dev_uart_port_mock_clear_error(void);
void dev_uart_port_mock_push_rx_byte(dev_uart_id_t uart, uint8_t byte);
uint8_t dev_uart_port_mock_get_tx_byte(dev_uart_id_t uart);

#ifdef __cplusplus
}
#endif

#endif /* DEV_UART_PORT_MOCK_H */
