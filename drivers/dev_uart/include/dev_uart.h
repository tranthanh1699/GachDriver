#ifndef DEV_UART_H
#define DEV_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_uart_types.h"
#include "dev_uart_cfg.h"
#include "dev_error.h"

dev_err_t dev_uart_init(void);
dev_err_t dev_uart_deinit(void);
bool     dev_uart_is_initialized(void);
bool     dev_uart_is_valid(dev_uart_id_t uart);

dev_err_t dev_uart_config(dev_uart_id_t uart, dev_uart_baudrate_t baud,
                          dev_uart_data_bits_t db, dev_uart_stop_bits_t sb,
                          dev_uart_parity_t p, dev_uart_flow_control_t fc);
dev_err_t dev_uart_set_baudrate(dev_uart_id_t uart, dev_uart_baudrate_t baud);

dev_err_t dev_uart_write(dev_uart_id_t uart, const uint8_t *data,
                         uint16_t len, dev_uart_timeout_t to);
dev_err_t dev_uart_write_byte(dev_uart_id_t uart, uint8_t data, dev_uart_timeout_t to);
dev_err_t dev_uart_write_string(dev_uart_id_t uart, const char *text, dev_uart_timeout_t to);

dev_err_t dev_uart_read(dev_uart_id_t uart, uint8_t *data, uint16_t len,
                        uint16_t *read_len, dev_uart_timeout_t to);
dev_err_t dev_uart_read_byte(dev_uart_id_t uart, uint8_t *data, dev_uart_timeout_t to);

uint16_t  dev_uart_rx_available(dev_uart_id_t uart);
dev_err_t dev_uart_flush_rx(dev_uart_id_t uart);
dev_err_t dev_uart_flush_tx(dev_uart_id_t uart);
dev_err_t dev_uart_rx_start(dev_uart_id_t uart);
dev_err_t dev_uart_rx_stop(dev_uart_id_t uart);

#ifdef __cplusplus
}
#endif

#endif /* DEV_UART_H */
