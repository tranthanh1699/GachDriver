#include "dev_gpio_port_mock.h"

static dev_gpio_level_t m_levels[DEV_GPIO_CFG_MAX_PINS];
static bool             m_is_output[DEV_GPIO_CFG_MAX_PINS];
static bool             m_intr_enabled[DEV_GPIO_CFG_MAX_PINS];
static dev_err_t        m_error = DEV_OK;

void dev_gpio_port_mock_set_error(dev_err_t e)  { m_error = e; }
void dev_gpio_port_mock_clear_error(void)        { m_error = DEV_OK; }
void dev_gpio_port_mock_trigger_isr(dev_gpio_pin_t pin) { dev_gpio_dispatch_isr(pin); }

dev_gpio_level_t dev_gpio_port_mock_get_level(dev_gpio_pin_t pin)
    { return (pin < DEV_GPIO_CFG_PIN_COUNT) ? m_levels[pin] : DEV_GPIO_LEVEL_LOW; }
bool dev_gpio_port_mock_is_output(dev_gpio_pin_t pin)
    { return (pin < DEV_GPIO_CFG_PIN_COUNT) && m_is_output[pin]; }
bool dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_pin_t pin)
    { return (pin < DEV_GPIO_CFG_PIN_COUNT) && m_intr_enabled[pin]; }

static bool pin_ok(dev_gpio_pin_t p) { return (p < DEV_GPIO_CFG_PIN_COUNT); }

dev_err_t dev_gpio_port_init(void)
    { if (m_error != DEV_OK) return m_error; return DEV_OK; }
dev_err_t dev_gpio_port_deinit(void)
    { if (m_error != DEV_OK) return m_error; return DEV_OK; }
dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    (void)pull;
    if (m_error != DEV_OK) return m_error;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    m_is_output[pin] = false; return DEV_OK;
}
dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t lvl)
{
    if (m_error != DEV_OK) return m_error;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    m_is_output[pin] = true; m_levels[pin] = lvl; return DEV_OK;
}
dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    if (m_error != DEV_OK) return m_error;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    if (level == NULL) return DEV_ERR_NULL_PTR;
    *level = m_levels[pin]; return DEV_OK;
}
dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t lvl)
{
    if (m_error != DEV_OK) return m_error;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    m_levels[pin] = lvl; return DEV_OK;
}
dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin)
{
    if (m_error != DEV_OK) return m_error;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    m_levels[pin] = (m_levels[pin] == DEV_GPIO_LEVEL_LOW) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW;
    return DEV_OK;
}
dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    (void)pull;
    if (m_error != DEV_OK) return m_error;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    return DEV_OK;
}
dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr)
{
    if (m_error != DEV_OK) return m_error;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    if (intr == DEV_GPIO_INTR_BOTH_EDGES || intr == DEV_GPIO_INTR_LOW_LEVEL || intr == DEV_GPIO_INTR_HIGH_LEVEL)
        return DEV_ERR_NOT_SUPPORTED;
    return DEV_OK;
}
dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin)
{
    if (m_error != DEV_OK) return m_error;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    m_intr_enabled[pin] = true; return DEV_OK;
}
dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin)
{
    if (m_error != DEV_OK) return m_error;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    m_intr_enabled[pin] = false; return DEV_OK;
}
