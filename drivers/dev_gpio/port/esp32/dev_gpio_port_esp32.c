#include "dev_gpio_port_esp32.h"
#include "dev_gpio_cfg.h"
#include "dev_compiler.h"

static const dev_gpio_hw_pin_t s_gpio_map[DEV_GPIO_CFG_PIN_COUNT] = {
    [DEV_GPIO_LED_STATUS]  = { DEV_GPIO_LED_STATUS,  2 },
    [DEV_GPIO_BUTTON_USER] = { DEV_GPIO_BUTTON_USER, 0 },
};

static bool s_pin_valid(dev_gpio_pin_t pin)
{
    return (pin < DEV_GPIO_CFG_PIN_COUNT);
}

dev_err_t dev_gpio_port_init(void)
{
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    uint16_t i;
    for (i = 0U; i < DEV_GPIO_CFG_PIN_COUNT; i++) {
        DEV_UNUSED(s_gpio_map[i].gpio_num);
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    DEV_UNUSED(pull);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t initial_level)
{
    DEV_UNUSED(initial_level);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    if (level == NULL) { return DEV_ERR_NULL_PTR; }
    *level = DEV_GPIO_LEVEL_LOW;
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    DEV_UNUSED(level);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    DEV_UNUSED(pull);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr)
{
    DEV_UNUSED(intr);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    if (intr == DEV_GPIO_INTR_LOW_LEVEL || intr == DEV_GPIO_INTR_HIGH_LEVEL) {
        return DEV_ERR_NOT_SUPPORTED;
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}
