#include "dev_uart.h"
#include "dev_uart_port.h"
#include "dev_common.h"
#include <string.h>

static bool g_initialized = false;

static bool id_ok(dev_uart_id_t u)        { return (u < DEV_UART_CFG_MAX_INSTANCES); }
static bool db_ok(dev_uart_data_bits_t d)  { return (d <= DEV_UART_DATA_BITS_9); }
static bool sb_ok(dev_uart_stop_bits_t s)  { return (s <= DEV_UART_STOP_BITS_2); }
static bool par_ok(dev_uart_parity_t p)    { return (p <= DEV_UART_PARITY_ODD); }
static bool fc_ok(dev_uart_flow_control_t f){ return (f <= DEV_UART_FLOW_CONTROL_RTS_CTS); }

static dev_err_t map_err(dev_err_t e) {
    if (e == DEV_OK)                return DEV_OK;
    if (e == DEV_ERR_INVALID_ARG)   return DEV_ERR_INVALID_ARG;
    if (e == DEV_ERR_NULL_PTR)      return DEV_ERR_NULL_PTR;
    if (e == DEV_ERR_NOT_SUPPORTED) return DEV_ERR_NOT_SUPPORTED;
    if (e == DEV_ERR_TIMEOUT)       return DEV_ERR_TIMEOUT;
    if (e == DEV_ERR_BUSY)          return DEV_ERR_BUSY;
    if (e == DEV_ERR_OVERFLOW)      return DEV_ERR_OVERFLOW;
    if (e == DEV_ERR_EMPTY)         return DEV_ERR_EMPTY;
    return DEV_ERR_HW_FAILURE;
}

dev_err_t dev_uart_init(void)
{
    if (g_initialized) return DEV_ERR_ALREADY_INITIALIZED;
    g_initialized = true;
    dev_err_t e = dev_uart_port_init();
    if (e != DEV_OK) { g_initialized = false; return map_err(e); }
    return DEV_OK;
}

dev_err_t dev_uart_deinit(void)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    g_initialized = false;
    dev_err_t e = dev_uart_port_deinit();
    return (e != DEV_OK) ? map_err(e) : DEV_OK;
}

bool dev_uart_is_initialized(void) { return g_initialized; }

bool dev_uart_is_valid(dev_uart_id_t uart) { return (uart < DEV_UART_CFG_MAX_INSTANCES); }

dev_err_t dev_uart_config(dev_uart_id_t u, dev_uart_baudrate_t b,
                          dev_uart_data_bits_t db, dev_uart_stop_bits_t sb,
                          dev_uart_parity_t p, dev_uart_flow_control_t fc)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!id_ok(u)) return DEV_ERR_INVALID_ARG;
    if (!db_ok(db) || !sb_ok(sb) || !par_ok(p) || !fc_ok(fc)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_uart_port_config(u, b, db, sb, p, fc));
}

dev_err_t dev_uart_set_baudrate(dev_uart_id_t u, dev_uart_baudrate_t b)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!id_ok(u)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_uart_port_set_baudrate(u, b));
}

dev_err_t dev_uart_write(dev_uart_id_t u, const uint8_t *data, uint16_t len, dev_uart_timeout_t to)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!id_ok(u)) return DEV_ERR_INVALID_ARG;
    if (data == NULL) return DEV_ERR_NULL_PTR;
    if (len == 0U) return DEV_ERR_INVALID_ARG;
    return map_err(dev_uart_port_write(u, data, len, to));
}

dev_err_t dev_uart_write_byte(dev_uart_id_t u, uint8_t data, dev_uart_timeout_t to)
    { return dev_uart_write(u, &data, 1U, to); }

dev_err_t dev_uart_write_string(dev_uart_id_t u, const char *text, dev_uart_timeout_t to)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!id_ok(u)) return DEV_ERR_INVALID_ARG;
    if (text == NULL) return DEV_ERR_NULL_PTR;
    size_t len = strlen(text);
    if (len == 0U) return DEV_OK;
    if (len > DEV_UART_CFG_MAX_STRING_LENGTH) len = DEV_UART_CFG_MAX_STRING_LENGTH;
    return map_err(dev_uart_port_write(u, (const uint8_t *)text, (uint16_t)len, to));
}

dev_err_t dev_uart_read(dev_uart_id_t u, uint8_t *data, uint16_t len,
                        uint16_t *read_len, dev_uart_timeout_t to)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!id_ok(u)) return DEV_ERR_INVALID_ARG;
    if (data == NULL) return DEV_ERR_NULL_PTR;
    if (read_len == NULL) return DEV_ERR_NULL_PTR;
    if (len == 0U) return DEV_ERR_INVALID_ARG;
    return map_err(dev_uart_port_read(u, data, len, read_len, to));
}

dev_err_t dev_uart_read_byte(dev_uart_id_t u, uint8_t *data, dev_uart_timeout_t to)
{
    uint16_t n;
    return dev_uart_read(u, data, 1U, &n, to);
}

uint16_t dev_uart_rx_available(dev_uart_id_t u)
{
    if (!g_initialized || !id_ok(u)) return 0U;
    return dev_uart_port_rx_available(u);
}

dev_err_t dev_uart_flush_rx(dev_uart_id_t u)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!id_ok(u)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_uart_port_flush_rx(u));
}

dev_err_t dev_uart_flush_tx(dev_uart_id_t u)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!id_ok(u)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_uart_port_flush_tx(u));
}

dev_err_t dev_uart_rx_start(dev_uart_id_t u)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!id_ok(u)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_uart_port_rx_start(u));
}

dev_err_t dev_uart_rx_stop(dev_uart_id_t u)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!id_ok(u)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_uart_port_rx_stop(u));
}
