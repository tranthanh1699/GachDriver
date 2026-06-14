#include "dev_uart_port_mock.h"
#include "dev_ringbuf.h"
#include "dev_common.h"

static uint8_t    s_rx_buf[DEV_UART_CFG_MAX_INSTANCES][512U];
static dev_ringbuf_t s_rx_rings[DEV_UART_CFG_MAX_INSTANCES];
static dev_err_t  s_error = DEV_OK;

void dev_uart_port_mock_reset(void)
{
    s_error = DEV_OK;
    for (uint8_t i = 0U; i < DEV_UART_CFG_MAX_INSTANCES; i++)
        dev_ringbuf_init(&s_rx_rings[i], s_rx_buf[i], (uint16_t)sizeof(s_rx_buf[i]));
}
void dev_uart_port_mock_set_error(dev_err_t e)   { s_error = e; }
void dev_uart_port_mock_clear_error(void)         { s_error = DEV_OK; }
void dev_uart_port_mock_push_rx_byte(dev_uart_id_t u, uint8_t b)
    { if (u < DEV_UART_CFG_MAX_INSTANCES) dev_ringbuf_try_push(&s_rx_rings[u], b); }
uint8_t dev_uart_port_mock_get_tx_byte(dev_uart_id_t u) { DEV_UNUSED(u); return 0U; }

dev_err_t dev_uart_port_init(void)
    { dev_uart_port_mock_reset(); return s_error; }
dev_err_t dev_uart_port_deinit(void) { return s_error; }
dev_err_t dev_uart_port_config(dev_uart_id_t u, dev_uart_baudrate_t b,
                               dev_uart_data_bits_t db, dev_uart_stop_bits_t sb,
                               dev_uart_parity_t p, dev_uart_flow_control_t fc)
    { DEV_UNUSED(u);DEV_UNUSED(b);DEV_UNUSED(db);DEV_UNUSED(sb);DEV_UNUSED(p);
      if (fc != DEV_UART_FLOW_CONTROL_NONE) return DEV_ERR_NOT_SUPPORTED; return s_error; }
dev_err_t dev_uart_port_set_baudrate(dev_uart_id_t u, dev_uart_baudrate_t b)
    { DEV_UNUSED(u); DEV_UNUSED(b); return s_error; }
dev_err_t dev_uart_port_write(dev_uart_id_t u, const uint8_t *d, uint16_t l, dev_uart_timeout_t to)
    { DEV_UNUSED(u);DEV_UNUSED(d);DEV_UNUSED(l);DEV_UNUSED(to); return s_error; }
dev_err_t dev_uart_port_read(dev_uart_id_t u, uint8_t *data, uint16_t len,
                             uint16_t *read_len, dev_uart_timeout_t to)
{
    DEV_UNUSED(to);
    if (s_error != DEV_OK) { *read_len = 0U; return s_error; }
    *read_len = 0U;
    while (*read_len < len) {
        uint8_t b;
        if (!dev_ringbuf_try_pop(&s_rx_rings[u], &b)) break;
        data[(*read_len)++] = b;
    }
    return (*read_len > 0U) ? DEV_OK : DEV_ERR_EMPTY;
}
uint16_t dev_uart_port_rx_available(dev_uart_id_t u)
    { return (uint16_t)dev_ringbuf_available(&s_rx_rings[u]); }
dev_err_t dev_uart_port_flush_rx(dev_uart_id_t u)
    { dev_ringbuf_flush(&s_rx_rings[u]); return s_error; }
dev_err_t dev_uart_port_flush_tx(dev_uart_id_t u)  { DEV_UNUSED(u); return s_error; }
dev_err_t dev_uart_port_rx_start(dev_uart_id_t u)  { DEV_UNUSED(u); return s_error; }
dev_err_t dev_uart_port_rx_stop(dev_uart_id_t u)   { DEV_UNUSED(u); return s_error; }
