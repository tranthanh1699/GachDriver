#include "dev_uart_port_stm32.h"
#include "dev_compiler.h"

#ifdef HAL_UART_MODULE_ENABLED

#include "dev_ringbuf.h"
#include "dev_common.h"

static uint8_t         s_console_rx_buf[DEV_UART_CONSOLE_RX_BUFFER_SIZE];
static uint8_t         s_gnss_rx_buf[DEV_UART_GNSS_RX_BUFFER_SIZE];
static uint8_t         s_modem_rx_buf[DEV_UART_MODEM_RX_BUFFER_SIZE];

static const dev_uart_hw_t s_uart_map[DEV_UART_CFG_MAX_INSTANCES] = {
    [DEV_UART_CONSOLE] = { DEV_UART_CONSOLE, USART1,
        GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10, GPIO_AF7_USART1,
        DEV_UART_BAUDRATE_115200, DEV_UART_DATA_BITS_8, DEV_UART_STOP_BITS_1,
        DEV_UART_PARITY_NONE, DEV_UART_FLOW_CONTROL_NONE,
        s_console_rx_buf, DEV_UART_CONSOLE_RX_BUFFER_SIZE },
    [DEV_UART_GNSS]    = { DEV_UART_GNSS, USART2,
        GPIOD, GPIO_PIN_5, GPIOD, GPIO_PIN_6, GPIO_AF7_USART2,
        DEV_UART_BAUDRATE_9600, DEV_UART_DATA_BITS_8, DEV_UART_STOP_BITS_1,
        DEV_UART_PARITY_NONE, DEV_UART_FLOW_CONTROL_NONE,
        s_gnss_rx_buf, DEV_UART_GNSS_RX_BUFFER_SIZE },
    [DEV_UART_MODEM]   = { DEV_UART_MODEM, USART3,
        GPIOD, GPIO_PIN_8, GPIOD, GPIO_PIN_9, GPIO_AF7_USART3,
        DEV_UART_BAUDRATE_115200, DEV_UART_DATA_BITS_8, DEV_UART_STOP_BITS_1,
        DEV_UART_PARITY_NONE, DEV_UART_FLOW_CONTROL_NONE,
        s_modem_rx_buf, DEV_UART_MODEM_RX_BUFFER_SIZE },
};

static UART_HandleTypeDef s_handles[DEV_UART_CFG_MAX_INSTANCES];
static dev_ringbuf_t      s_rx_rings[DEV_UART_CFG_MAX_INSTANCES];
static uint8_t            s_rx_byte[DEV_UART_CFG_MAX_INSTANCES];
static bool               s_rx_running[DEV_UART_CFG_MAX_INSTANCES];

static const dev_uart_hw_t *find_uart(dev_uart_id_t id)
    { return (id < DEV_UART_CFG_MAX_INSTANCES && s_uart_map[id].instance) ? &s_uart_map[id] : NULL; }

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

static void stm32_clock_and_gpio(void)
{
    for (uint16_t i = 0U; i < DEV_UART_CFG_MAX_INSTANCES; i++) {
        const dev_uart_hw_t *u = &s_uart_map[i];
        if (!u->instance) continue;

        GPIO_InitTypeDef g = {0};
        g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_LOW;
        g.Alternate = u->gpio_alternate;

        if      (u->tx_port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
        else if (u->tx_port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
        if      (u->rx_port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
        else if (u->rx_port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();

        g.Pin = u->tx_pin; HAL_GPIO_Init(u->tx_port, &g);
        g.Pin = u->rx_pin; HAL_GPIO_Init(u->rx_port, &g);

        if      (u->instance == USART1) __HAL_RCC_USART1_CLK_ENABLE();
        else if (u->instance == USART2) __HAL_RCC_USART2_CLK_ENABLE();
        else if (u->instance == USART3) __HAL_RCC_USART3_CLK_ENABLE();

        s_handles[i].Instance = u->instance;
        s_handles[i].Init.BaudRate     = u->default_baudrate;
        s_handles[i].Init.WordLength   = stm32_word_len(u->data_bits);
        s_handles[i].Init.StopBits     = stm32_stop(u->stop_bits);
        s_handles[i].Init.Parity       = stm32_parity(u->parity);
        s_handles[i].Init.Mode         = UART_MODE_TX_RX;
        s_handles[i].Init.HwFlowCtl    = stm32_flow(u->flow_control);
        s_handles[i].Init.OverSampling = UART_OVERSAMPLING_16;
        if (HAL_UART_Init(&s_handles[i]) != HAL_OK) {
            return DEV_ERR_HW_FAILURE;
        }

        dev_ringbuf_init(&s_rx_rings[i], u->rx_buffer, u->rx_buffer_size);
    }
    return DEV_OK;
}

dev_err_t dev_uart_port_init(void)
{
    if (stm32_clock_and_gpio() != DEV_OK) return DEV_ERR_HW_FAILURE;
    return DEV_OK;
}
dev_err_t dev_uart_port_deinit(void)
    { for (uint16_t i=0U;i<DEV_UART_CFG_MAX_INSTANCES;i++) if(s_uart_map[i].instance)HAL_UART_DeInit(&s_handles[i]); return DEV_OK; }

dev_err_t dev_uart_port_config(dev_uart_id_t u, dev_uart_baudrate_t b,
                               dev_uart_data_bits_t db, dev_uart_stop_bits_t sb,
                               dev_uart_parity_t p, dev_uart_flow_control_t fc)
{
    if (!find_uart(u)) return DEV_ERR_INVALID_ARG;
    if (fc != DEV_UART_FLOW_CONTROL_NONE) return DEV_ERR_NOT_SUPPORTED;
    s_handles[u].Init.BaudRate=b; s_handles[u].Init.WordLength=stm32_word_len(db);
    s_handles[u].Init.StopBits=stm32_stop(sb); s_handles[u].Init.Parity=stm32_parity(p);
    s_handles[u].Init.HwFlowCtl=stm32_flow(fc);
    return stm32_map(HAL_UART_Init(&s_handles[u]));
}

dev_err_t dev_uart_port_set_baudrate(dev_uart_id_t u, dev_uart_baudrate_t b)
    { if(!find_uart(u))return DEV_ERR_INVALID_ARG; s_handles[u].Init.BaudRate=b; return stm32_map(HAL_UART_Init(&s_handles[u])); }

dev_err_t dev_uart_port_write(dev_uart_id_t u, const uint8_t *d, uint16_t l, dev_uart_timeout_t to)
    { if(!find_uart(u))return DEV_ERR_INVALID_ARG; return stm32_map(HAL_UART_Transmit(&s_handles[u],(uint8_t*)d,l,to)); }

dev_err_t dev_uart_port_read(dev_uart_id_t u, uint8_t *data, uint16_t len, uint16_t *rl, dev_uart_timeout_t to)
{
    if(!find_uart(u)){*rl=0U;return DEV_ERR_INVALID_ARG;}
    *rl=0U;
    while(*rl<len){uint8_t b; if(!dev_ringbuf_try_pop(&s_rx_rings[u],&b)){if(to==DEV_UART_TIMEOUT_NO_WAIT)return DEV_ERR_EMPTY;break;} data[(*rl)++]=b;}
    DEV_UNUSED(to); return (*rl>0U)?DEV_OK:DEV_ERR_EMPTY;
}

uint16_t dev_uart_port_rx_available(dev_uart_id_t u) { return (uint16_t)dev_ringbuf_available(&s_rx_rings[u]); }
dev_err_t dev_uart_port_flush_rx(dev_uart_id_t u)    { if(!find_uart(u))return DEV_ERR_INVALID_ARG; dev_ringbuf_flush(&s_rx_rings[u]); return DEV_OK; }
dev_err_t dev_uart_port_flush_tx(dev_uart_id_t u)    { DEV_UNUSED(u); return DEV_OK; }
dev_err_t dev_uart_port_rx_start(dev_uart_id_t u)
{
    if (!find_uart(u)) return DEV_ERR_INVALID_ARG;
    dev_err_t e = stm32_map(HAL_UART_Receive_IT(&s_handles[u], &s_rx_byte[u], DEV_UART_STM32_RX_TEMP_BYTE_COUNT));
    if (e == DEV_OK) s_rx_running[u] = true;
    return e;
}

dev_err_t dev_uart_port_rx_stop(dev_uart_id_t u)
    { if(!find_uart(u))return DEV_ERR_INVALID_ARG; s_rx_running[u]=false; HAL_UART_AbortReceive_IT(&s_handles[u]); return DEV_OK; }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    for (uint16_t i = 0U; i < DEV_UART_CFG_MAX_INSTANCES; i++) {
        if (huart == &s_handles[i] && s_rx_running[i]) {
            (void)dev_ringbuf_try_push(&s_rx_rings[i], s_rx_byte[i]);
            if (HAL_UART_Receive_IT(&s_handles[i], &s_rx_byte[i], DEV_UART_STM32_RX_TEMP_BYTE_COUNT) != HAL_OK) {
                s_rx_running[i] = false;  /* stop RX on re-arm failure */
            }
            return;
        }
    }
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
