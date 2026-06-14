#include "dev_uart_port_stm32.h"
#include "dev_compiler.h"

#ifdef HAL_UART_MODULE_ENABLED

#include "dev_ringbuf.h"

/*
 * Cube-generated HAL handles — declare as extern.
 * If your handles are named differently, update the map below.
 */
extern UART_HandleTypeDef huart1;

static uint8_t s_console_rx_buf[DEV_UART_CONSOLE_RX_BUFFER_SIZE];

static dev_ringbuf_t s_rx_rings[DEV_UART_CFG_MAX_INSTANCES];
static uint8_t       s_rx_byte[DEV_UART_CFG_MAX_INSTANCES];
static bool          s_rx_running[DEV_UART_CFG_MAX_INSTANCES];

static const dev_uart_hw_t s_uart_map[DEV_UART_CFG_MAX_INSTANCES] = {
    [DEV_UART_CONSOLE] = { DEV_UART_CONSOLE, &huart1,
        DEV_UART_BAUDRATE_115200, s_console_rx_buf, DEV_UART_CONSOLE_RX_BUFFER_SIZE }
};

static const dev_uart_hw_t *find_uart(dev_uart_id_t id)
    { return (id < DEV_UART_CFG_MAX_INSTANCES && s_uart_map[id].handle) ? &s_uart_map[id] : NULL; }

static dev_err_t stm32_map(HAL_StatusTypeDef h)
{
    switch (h) {
    case HAL_OK:      return DEV_OK;
    case HAL_TIMEOUT: return DEV_ERR_TIMEOUT;
    case HAL_BUSY:    return DEV_ERR_BUSY;
    case HAL_ERROR:   return DEV_ERR_HW_FAILURE;
    default:          return DEV_ERR_HW_FAILURE;
    }
}

static uint32_t stm32_word_len(dev_uart_data_bits_t d)
    { return (d == DEV_UART_DATA_BITS_9) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B; }
static uint32_t stm32_stop(dev_uart_stop_bits_t s)
    { return (s == DEV_UART_STOP_BITS_2) ? UART_STOPBITS_2 : UART_STOPBITS_1; }
static uint32_t stm32_parity(dev_uart_parity_t p)
{
    switch (p) {
    case DEV_UART_PARITY_EVEN: return UART_PARITY_EVEN;
    case DEV_UART_PARITY_ODD:  return UART_PARITY_ODD;
    default:                   return UART_PARITY_NONE;
    }
}
static uint32_t stm32_flow(dev_uart_flow_control_t f)
{
    switch (f) {
    case DEV_UART_FLOW_CONTROL_RTS:     return UART_HWCONTROL_RTS;
    case DEV_UART_FLOW_CONTROL_CTS:     return UART_HWCONTROL_CTS;
    case DEV_UART_FLOW_CONTROL_RTS_CTS: return UART_HWCONTROL_RTS_CTS;
    default:                            return UART_HWCONTROL_NONE;
    }
}

/* ── Port API ── */

dev_err_t dev_uart_port_init(void)
{
    for (uint16_t i = 0U; i < DEV_UART_CFG_MAX_INSTANCES; i++) {
        const dev_uart_hw_t *u = &s_uart_map[i];
        if (u->handle) {
            dev_ringbuf_init(&s_rx_rings[i], u->rx_buffer, u->rx_buffer_size);
#if (DEV_UART_CFG_RX_BUFFER_ENABLED == 1U)
            if (HAL_UART_Receive_IT(u->handle, &s_rx_byte[i], DEV_UART_STM32_RX_TEMP_BYTE_COUNT) == HAL_OK) {
                s_rx_running[i] = true;
            }
#endif
        }
    }
    return DEV_OK;
}

dev_err_t dev_uart_port_deinit(void)
{
    for (uint16_t i = 0U; i < DEV_UART_CFG_MAX_INSTANCES; i++) {
        if (s_uart_map[i].handle) {
            s_rx_running[i] = false;
            HAL_UART_AbortReceive_IT(s_uart_map[i].handle);
        }
    }
    return DEV_OK;
}

dev_err_t dev_uart_port_config(dev_uart_id_t u, dev_uart_baudrate_t b,
                               dev_uart_data_bits_t db, dev_uart_stop_bits_t sb,
                               dev_uart_parity_t p, dev_uart_flow_control_t fc)
{
    const dev_uart_hw_t *h = find_uart(u);
    if (!h) return DEV_ERR_INVALID_ARG;
    if (fc != DEV_UART_FLOW_CONTROL_NONE) return DEV_ERR_NOT_SUPPORTED;

    h->handle->Init.BaudRate   = b;
    h->handle->Init.WordLength = stm32_word_len(db);
    h->handle->Init.StopBits   = stm32_stop(sb);
    h->handle->Init.Parity     = stm32_parity(p);
    h->handle->Init.HwFlowCtl  = stm32_flow(fc);
    return stm32_map(HAL_UART_Init(h->handle));
}

dev_err_t dev_uart_port_set_baudrate(dev_uart_id_t u, dev_uart_baudrate_t b)
{
    const dev_uart_hw_t *h = find_uart(u);
    if (!h) return DEV_ERR_INVALID_ARG;
    h->handle->Init.BaudRate = b;
    return stm32_map(HAL_UART_Init(h->handle));
}

dev_err_t dev_uart_port_write(dev_uart_id_t u, const uint8_t *d, uint16_t l, dev_uart_timeout_t to)
{
    const dev_uart_hw_t *h = find_uart(u);
    if (!h) return DEV_ERR_INVALID_ARG;
    return stm32_map(HAL_UART_Transmit(h->handle, (uint8_t *)d, l, to));
}

dev_err_t dev_uart_port_read(dev_uart_id_t u, uint8_t *data, uint16_t len,
                             uint16_t *rl, dev_uart_timeout_t to)
{
    const dev_uart_hw_t *h = find_uart(u);
    if (!h) { *rl = 0U; return DEV_ERR_INVALID_ARG; }
    *rl = 0U;
    while (*rl < len) {
        uint8_t byte;
        dev_err_t e = dev_ringbuf_pop(&s_rx_rings[u], &byte);
        if (e != DEV_OK) {
            if (to == DEV_UART_TIMEOUT_NO_WAIT) return DEV_ERR_EMPTY;
            break;
        }
        data[(*rl)++] = byte;
    }
    DEV_UNUSED(h); DEV_UNUSED(to);
    return (*rl > 0U) ? DEV_OK : DEV_ERR_EMPTY;
}

uint16_t dev_uart_port_rx_available(dev_uart_id_t u)
    { return (uint16_t)dev_ringbuf_available(&s_rx_rings[u]); }

dev_err_t dev_uart_port_flush_rx(dev_uart_id_t u)
    { if (!find_uart(u)) return DEV_ERR_INVALID_ARG; dev_ringbuf_flush(&s_rx_rings[u]); return DEV_OK; }
dev_err_t dev_uart_port_flush_tx(dev_uart_id_t u)
    { DEV_UNUSED(u); return DEV_OK; }

dev_err_t dev_uart_port_rx_start(dev_uart_id_t u)
{
    const dev_uart_hw_t *h = find_uart(u);
    if (!h) return DEV_ERR_INVALID_ARG;
    dev_err_t e = stm32_map(HAL_UART_Receive_IT(h->handle, &s_rx_byte[u], DEV_UART_STM32_RX_TEMP_BYTE_COUNT));
    if (e == DEV_OK) s_rx_running[u] = true;
    return e;
}

dev_err_t dev_uart_port_rx_stop(dev_uart_id_t u)
{
    const dev_uart_hw_t *h = find_uart(u);
    if (!h) return DEV_ERR_INVALID_ARG;
    s_rx_running[u] = false;
    HAL_UART_AbortReceive_IT(h->handle);
    return DEV_OK;
}

void dev_uart_port_stm32_rx_cplt_callback(UART_HandleTypeDef *huart)
{
    for (uint16_t i = 0U; i < DEV_UART_CFG_MAX_INSTANCES; i++) {
        if (huart == s_uart_map[i].handle && s_rx_running[i]) {
            (void)dev_ringbuf_write(&s_rx_rings[i], s_rx_byte[i]);
            if (HAL_UART_Receive_IT(huart, &s_rx_byte[i], DEV_UART_STM32_RX_TEMP_BYTE_COUNT) != HAL_OK) {
                s_rx_running[i] = false;
            }
            return;
        }
    }
}

/* STM32 Interrupt Handlers */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    dev_uart_port_stm32_rx_cplt_callback(huart);
}

#else /* HAL_UART_MODULE_ENABLED not defined */

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

#endif /* HAL_UART_MODULE_ENABLED */
