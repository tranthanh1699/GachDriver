#include "dev_gpio.h"
#include "dev_gpio_port.h"
#include "dev_common.h"

/* ── Module state ── */

static bool g_initialized = false;

/* ── Callback table (indexed by pin ID) ── */

typedef struct {
    dev_gpio_callback_t callback;
    void               *user_arg;
    dev_gpio_intr_t     intr;
    bool                enabled;
} dev_gpio_callback_entry_t;

static dev_gpio_callback_entry_t g_callbacks[DEV_GPIO_CFG_MAX_PINS];

/* ── Port error mapping ── */

static dev_err_t dev_gpio_map_port_error(dev_err_t port_err)
{
    if (port_err == DEV_OK)                { return DEV_OK; }
    if (port_err == DEV_ERR_INVALID_ARG)   { return DEV_ERR_INVALID_ARG; }
    if (port_err == DEV_ERR_NULL_PTR)      { return DEV_ERR_NULL_PTR; }
    if (port_err == DEV_ERR_NOT_SUPPORTED) { return DEV_ERR_NOT_SUPPORTED; }
    return DEV_ERR_HW_FAILURE;
}

/* ── Validation helpers ── */

static bool dev_gpio_is_valid_pin(dev_gpio_pin_t pin)
{
    return (pin < DEV_GPIO_CFG_MAX_PINS);
}

static bool dev_gpio_is_valid_level(dev_gpio_level_t level)
{
    return (level == DEV_GPIO_LEVEL_LOW) || (level == DEV_GPIO_LEVEL_HIGH);
}

static bool dev_gpio_is_valid_pull(dev_gpio_pull_t pull)
{
    return (pull == DEV_GPIO_PULL_NONE) ||
           (pull == DEV_GPIO_PULL_UP)   ||
           (pull == DEV_GPIO_PULL_DOWN);
}

/* ── Public API ── */

dev_err_t dev_gpio_init(void)
{
    dev_err_t err;

    if (g_initialized) {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    err = dev_gpio_port_init();
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    g_initialized = true;
    return DEV_OK;
}

dev_err_t dev_gpio_deinit(void)
{
    uint16_t i;
    dev_err_t port_err;
    dev_err_t result = DEV_OK;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Clear all callback entries */
    for (i = 0U; i < DEV_GPIO_CFG_MAX_PINS; i++) {
        g_callbacks[i].callback  = NULL;
        g_callbacks[i].user_arg  = NULL;
        g_callbacks[i].intr      = DEV_GPIO_INTR_DISABLE;
        g_callbacks[i].enabled   = false;
    }

    port_err = dev_gpio_port_deinit();
    g_initialized = false;

    if (port_err != DEV_OK) {
        result = dev_gpio_map_port_error(port_err);
    }

    return result;
}

bool dev_gpio_is_initialized(void)
{
    return g_initialized;
}

dev_err_t dev_gpio_input(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_input(pin, DEV_GPIO_PULL_NONE);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_input_pullup(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_input(pin, DEV_GPIO_PULL_UP);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_input_pulldown(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_input(pin, DEV_GPIO_PULL_DOWN);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_output(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_output(pin, DEV_GPIO_LEVEL_LOW);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_output_level(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (!dev_gpio_is_valid_level(level)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_output(pin, level);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    dev_gpio_level_t temp;
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (level == NULL) { return DEV_ERR_NULL_PTR; }

    err = dev_gpio_port_read(pin, &temp);
    if (err != DEV_OK) { return dev_gpio_map_port_error(err); }

    *level = temp;
    return DEV_OK;
}

dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (!dev_gpio_is_valid_level(level)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_write(pin, level);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_toggle(pin);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (!dev_gpio_is_valid_pull(pull)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_set_pull(pin, pull);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_high(dev_gpio_pin_t pin)
{
    return dev_gpio_write(pin, DEV_GPIO_LEVEL_HIGH);
}

dev_err_t dev_gpio_low(dev_gpio_pin_t pin)
{
    return dev_gpio_write(pin, DEV_GPIO_LEVEL_LOW);
}

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 1U)

dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t callback, void *user_arg)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    if (intr == DEV_GPIO_INTR_DISABLE) {
        if (callback != NULL) { return DEV_ERR_INVALID_ARG; }

        /* Block dispatch FIRST, then configure hardware */
        g_callbacks[pin].enabled = false;

        err = dev_gpio_port_interrupt(pin, DEV_GPIO_INTR_DISABLE);
        if (err != DEV_OK) {
            g_callbacks[pin].intr = DEV_GPIO_INTR_DISABLE;
            return dev_gpio_map_port_error(err);
        }

        g_callbacks[pin].callback  = NULL;
        g_callbacks[pin].user_arg  = NULL;
        g_callbacks[pin].intr      = DEV_GPIO_INTR_DISABLE;
        return DEV_OK;
    }

    /* intr != DISABLE */
    if (callback == NULL) { return DEV_ERR_NULL_PTR; }

    /* Configure hardware FIRST, store only on success */
    err = dev_gpio_port_interrupt(pin, intr);
    if (err != DEV_OK) { return dev_gpio_map_port_error(err); }

    g_callbacks[pin].callback  = callback;
    g_callbacks[pin].user_arg  = user_arg;
    g_callbacks[pin].intr      = intr;
    g_callbacks[pin].enabled   = false;
    return DEV_OK;
}

dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (g_callbacks[pin].callback == NULL) { return DEV_ERR_INVALID_STATE; }
    if (g_callbacks[pin].intr == DEV_GPIO_INTR_DISABLE) { return DEV_ERR_INVALID_STATE; }

    g_callbacks[pin].enabled = true;

    err = dev_gpio_port_interrupt_enable(pin);
    if (err != DEV_OK) {
        g_callbacks[pin].enabled = false;
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    g_callbacks[pin].enabled = false;

    err = dev_gpio_port_interrupt_disable(pin);
    if (err != DEV_OK) { return dev_gpio_map_port_error(err); }

    return DEV_OK;
}

void dev_gpio_dispatch_isr(dev_gpio_pin_t pin)
{
    dev_gpio_callback_t cb;
    void *arg;

    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return; }
    if (!g_callbacks[pin].enabled)    { return; }
    if (g_callbacks[pin].callback == NULL) { return; }

    cb  = g_callbacks[pin].callback;
    arg = g_callbacks[pin].user_arg;

    if (cb != NULL) {
        cb(pin, arg);
    }
}

#else /* DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U */

dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t callback, void *user_arg)
{
    DEV_UNUSED(pin); DEV_UNUSED(intr);
    DEV_UNUSED(callback); DEV_UNUSED(user_arg);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin)
{
    DEV_UNUSED(pin);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin)
{
    DEV_UNUSED(pin);
    return DEV_ERR_NOT_SUPPORTED;
}

void dev_gpio_dispatch_isr(dev_gpio_pin_t pin)
{
    DEV_UNUSED(pin);
}

#endif /* DEV_GPIO_CFG_INTERRUPT_ENABLED */
