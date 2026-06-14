/* nRF52 UART port — PLACEHOLDER STUB. */
#include "dev_uart_port_nrf52.h"
#include "dev_common.h"

static uint8_t s_rx[DEV_UART_CONSOLE_RX_BUFFER_SIZE];

static const dev_uart_hw_t s_map[DEV_UART_CFG_MAX_INSTANCES] = {
    [DEV_UART_CONSOLE] = { DEV_UART_CONSOLE, 0U, 6U, 8U, DEV_UART_BAUDRATE_115200, s_rx, sizeof(s_rx) },
};

dev_err_t dev_uart_port_init(void)                { return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_uart_port_deinit(void)              { return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_uart_port_config(dev_uart_id_t u, dev_uart_baudrate_t b, dev_uart_data_bits_t d,
                               dev_uart_stop_bits_t s, dev_uart_parity_t p, dev_uart_flow_control_t f)
    { DEV_UNUSED(u);DEV_UNUSED(b);DEV_UNUSED(d);DEV_UNUSED(s);DEV_UNUSED(p);DEV_UNUSED(f); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_uart_port_set_baudrate(dev_uart_id_t u, dev_uart_baudrate_t b)
    { DEV_UNUSED(u);DEV_UNUSED(b); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_uart_port_write(dev_uart_id_t u, const uint8_t *d, uint16_t l, dev_uart_timeout_t t)
    { DEV_UNUSED(u);DEV_UNUSED(d);DEV_UNUSED(l);DEV_UNUSED(t); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_uart_port_read(dev_uart_id_t u, uint8_t *d, uint16_t l, uint16_t *rl, dev_uart_timeout_t t)
    { DEV_UNUSED(u);DEV_UNUSED(d);DEV_UNUSED(l);DEV_UNUSED(t); *rl=0U; return DEV_ERR_NOT_SUPPORTED; }
uint16_t dev_uart_port_rx_available(dev_uart_id_t u) { DEV_UNUSED(u); return 0U; }
dev_err_t dev_uart_port_flush_rx(dev_uart_id_t u)  { DEV_UNUSED(u); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_uart_port_flush_tx(dev_uart_id_t u)  { DEV_UNUSED(u); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_uart_port_rx_start(dev_uart_id_t u)  { DEV_UNUSED(u); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_uart_port_rx_stop(dev_uart_id_t u)   { DEV_UNUSED(u); return DEV_ERR_NOT_SUPPORTED; }
