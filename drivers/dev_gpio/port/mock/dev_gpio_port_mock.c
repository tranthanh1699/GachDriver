#include "dev_gpio_port_mock.h"

/* ── Internal state (indexed by config index, NOT raw channel ID) ── */

static dev_gpio_level_t      m_levels[DEV_GPIO_CFG_MAX_CHANNELS];
static dev_gpio_direction_t  m_directions[DEV_GPIO_CFG_MAX_CHANNELS];
static dev_gpio_pull_t       m_pulls[DEV_GPIO_CFG_MAX_CHANNELS];
static dev_gpio_intr_type_t  m_interrupts[DEV_GPIO_CFG_MAX_CHANNELS];
static bool                  m_interrupt_enabled[DEV_GPIO_CFG_MAX_CHANNELS];
static uint16_t              m_channel_count;
static bool                  m_initialized;

/* ── Channel ID → config index mapping ── */

static const dev_gpio_config_t *m_config;

static uint16_t mock_find_index(dev_gpio_channel_t channel)
{
    uint16_t i;
    for (i = 0U; i < m_channel_count; i++) {
        if (m_config->channels[i].channel == channel) {
            return i;
        }
    }
    return m_channel_count; /* not found */
}

/* ── Error injection ── */

static dev_err_t m_global_error   = DEV_OK;
static dev_err_t m_op_errors[DEV_GPIO_PORT_MOCK_OP_COUNT];
static bool      m_op_error_set[DEV_GPIO_PORT_MOCK_OP_COUNT];
static uint16_t  m_fail_after_n   = 0U;
static uint16_t  m_call_counter   = 0U;
static dev_err_t m_fail_after_error = DEV_OK;
static bool      m_fail_after_active = false;

static dev_err_t mock_check_error(dev_gpio_port_mock_op_t op)
{
    m_call_counter++;

    if (m_fail_after_active && (m_call_counter == m_fail_after_n)) {
        return m_fail_after_error;
    }

    if (m_op_error_set[op]) {
        return m_op_errors[op];
    }

    if (m_global_error != DEV_OK) {
        return m_global_error;
    }

    return DEV_OK;
}

void dev_gpio_port_mock_set_error(dev_err_t error)
{
    m_global_error = error;
}

void dev_gpio_port_mock_set_error_for_op(dev_gpio_port_mock_op_t op, dev_err_t error)
{
    if (op < DEV_GPIO_PORT_MOCK_OP_COUNT) {
        m_op_errors[op]   = error;
        m_op_error_set[op] = true;
    }
}

void dev_gpio_port_mock_set_fail_after(uint16_t call_count, dev_err_t error)
{
    m_fail_after_n      = call_count;
    m_fail_after_error  = error;
    m_fail_after_active = true;
    m_call_counter      = 0U;
}

void dev_gpio_port_mock_clear_error(void)
{
    uint16_t i;
    m_global_error = DEV_OK;
    for (i = 0U; i < DEV_GPIO_PORT_MOCK_OP_COUNT; i++) {
        m_op_errors[i]    = DEV_OK;
        m_op_error_set[i] = false;
    }
    m_fail_after_active = false;
    m_fail_after_n      = 0U;
    m_call_counter      = 0U;
}

/* ── ISR simulation ── */

void dev_gpio_port_mock_trigger_isr(dev_gpio_channel_t channel)
{
    dev_gpio_dispatch_isr(channel);
}

/* ── State inspection ── */

dev_gpio_level_t dev_gpio_port_mock_get_level(dev_gpio_channel_t channel)
{
    uint16_t idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_GPIO_LEVEL_LOW;
    }
    return m_levels[idx];
}

dev_gpio_direction_t dev_gpio_port_mock_get_direction(dev_gpio_channel_t channel)
{
    uint16_t idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_GPIO_DIRECTION_INPUT;
    }
    return m_directions[idx];
}

dev_gpio_pull_t dev_gpio_port_mock_get_pull(dev_gpio_channel_t channel)
{
    uint16_t idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_GPIO_PULL_NONE;
    }
    return m_pulls[idx];
}

bool dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_channel_t channel)
{
    uint16_t idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return false;
    }
    return m_interrupt_enabled[idx];
}

/* ── Port API implementation ── */

dev_err_t dev_gpio_port_init(const dev_gpio_config_t *config)
{
    dev_err_t err;
    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_INIT);
    if (err != DEV_OK) {
        return err;
    }

    m_config   = config;
    m_initialized = true;
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    dev_err_t err;
    uint16_t i;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_DEINIT);
    if (err != DEV_OK) {
        return err;
    }

    for (i = 0U; i < m_channel_count; i++) {
        m_levels[i]             = DEV_GPIO_LEVEL_LOW;
        m_directions[i]         = DEV_GPIO_DIRECTION_INPUT;
        m_pulls[i]              = DEV_GPIO_PULL_NONE;
        m_interrupts[i]         = DEV_GPIO_INTR_DISABLE;
        m_interrupt_enabled[i]  = false;
    }
    m_initialized   = false;
    m_channel_count = 0U;
    m_config        = NULL;
    return DEV_OK;
}

dev_err_t dev_gpio_port_config_channel(const dev_gpio_channel_config_t *channel_config)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_CONFIG_CHANNEL);
    if (err != DEV_OK) {
        return err;
    }

    if (channel_config == NULL) {
        return DEV_ERR_NULL_PTR;
    }

    idx = m_channel_count;
    if (idx >= DEV_GPIO_CFG_MAX_CHANNELS) {
        return DEV_ERR_OUT_OF_RANGE;
    }

    m_levels[idx]     = channel_config->default_level;
    m_directions[idx] = channel_config->direction;
    m_pulls[idx]      = channel_config->pull;
    m_interrupts[idx] = channel_config->interrupt;
    m_channel_count++;
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_channel_t channel,
                             dev_gpio_level_t *level)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_READ);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    *level = m_levels[idx];
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_channel_t channel,
                              dev_gpio_level_t level)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_WRITE);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    m_levels[idx] = level;
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_channel_t channel)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_TOGGLE);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    if (m_levels[idx] == DEV_GPIO_LEVEL_LOW) {
        m_levels[idx] = DEV_GPIO_LEVEL_HIGH;
    } else {
        m_levels[idx] = DEV_GPIO_LEVEL_LOW;
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_direction(dev_gpio_channel_t channel,
                                      dev_gpio_direction_t direction)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_SET_DIRECTION);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Simulate unsupported direction for negative testing */
    if (direction == DEV_GPIO_DIRECTION_INPUT_OUTPUT) {
        return DEV_ERR_NOT_SUPPORTED;
    }

    m_directions[idx] = direction;
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_channel_t channel,
                                 dev_gpio_pull_t pull)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_SET_PULL);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Simulate unsupported pull for negative testing */
    if (pull == DEV_GPIO_PULL_DOWN) {
        return DEV_ERR_NOT_SUPPORTED;
    }

    m_pulls[idx] = pull;
    return DEV_OK;
}

dev_err_t dev_gpio_port_config_interrupt(dev_gpio_channel_t channel,
                                         dev_gpio_intr_type_t interrupt)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_CONFIG_INTERRUPT);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Simulate unsupported both-edges and level interrupts for negative testing */
    if ((interrupt == DEV_GPIO_INTR_BOTH_EDGES) ||
        (interrupt == DEV_GPIO_INTR_LOW_LEVEL) ||
        (interrupt == DEV_GPIO_INTR_HIGH_LEVEL)) {
        return DEV_ERR_NOT_SUPPORTED;
    }

    m_interrupts[idx] = interrupt;
    return DEV_OK;
}

dev_err_t dev_gpio_port_enable_interrupt(dev_gpio_channel_t channel)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_ENABLE_INTERRUPT);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    m_interrupt_enabled[idx] = true;
    return DEV_OK;
}

dev_err_t dev_gpio_port_disable_interrupt(dev_gpio_channel_t channel)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_DISABLE_INTERRUPT);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    m_interrupt_enabled[idx] = false;
    return DEV_OK;
}
