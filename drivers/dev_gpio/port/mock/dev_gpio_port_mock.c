#include "dev_gpio_port_mock.h"

static dev_gpio_level_t m_levels[DEV_GPIO_CFG_MAX_PINS];
static bool             m_is_output[DEV_GPIO_CFG_MAX_PINS];
static bool             m_interrupt_enabled[DEV_GPIO_CFG_MAX_PINS];
static dev_err_t        m_injected_error = DEV_OK;

/* ── Error injection ── */

void dev_gpio_port_mock_set_error(dev_err_t error)
{
    m_injected_error = error;
}

void dev_gpio_port_mock_clear_error(void)
{
    m_injected_error = DEV_OK;
}

/* ── ISR simulation ── */

void dev_gpio_port_mock_trigger_isr(dev_gpio_pin_t pin)
{
    dev_gpio_dispatch_isr(pin);
}

/* ── State inspection ── */

dev_gpio_level_t dev_gpio_port_mock_get_level(dev_gpio_pin_t pin)
{
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_GPIO_LEVEL_LOW; }
    return m_levels[pin];
}

bool dev_gpio_port_mock_is_output(dev_gpio_pin_t pin)
{
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return false; }
    return m_is_output[pin];
}

bool dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_pin_t pin)
{
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return false; }
    return m_interrupt_enabled[pin];
}

/* ── Port API ── */

dev_err_t dev_gpio_port_init(void)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    DEV_UNUSED(pull);
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_is_output[pin] = false;
    m_levels[pin] = DEV_GPIO_LEVEL_LOW;
    return DEV_OK;
}

dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t initial_level)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_is_output[pin] = true;
    m_levels[pin] = initial_level;
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }
    if (level == NULL) { return DEV_ERR_NULL_PTR; }

    *level = m_levels[pin];
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_levels[pin] = level;
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    if (m_levels[pin] == DEV_GPIO_LEVEL_LOW) {
        m_levels[pin] = DEV_GPIO_LEVEL_HIGH;
    } else {
        m_levels[pin] = DEV_GPIO_LEVEL_LOW;
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    DEV_UNUSED(pull);
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    /* Simulate unsupported interrupt modes for negative testing */
    if ((intr == DEV_GPIO_INTR_BOTH_EDGES) ||
        (intr == DEV_GPIO_INTR_LOW_LEVEL) ||
        (intr == DEV_GPIO_INTR_HIGH_LEVEL)) {
        return DEV_ERR_NOT_SUPPORTED;
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_interrupt_enabled[pin] = true;
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_interrupt_enabled[pin] = false;
    return DEV_OK;
}
