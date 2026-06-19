#include "dev_gpio.h"
#include "dev_gpio_port.h"
#include "dev_common.h"

static bool g_initialized = false;

/* ── Callback table ── */

typedef struct {
    dev_gpio_callback_t callback;
    void               *user_arg;
    dev_gpio_intr_t     intr;
    bool                enabled;
} cb_entry_t;

static cb_entry_t g_cb[DEV_GPIO_CFG_MAX_PINS];

/* ── Helpers ── */

static bool pin_ok(dev_gpio_pin_t p)  { return (p < DEV_GPIO_CFG_PIN_COUNT); }
static bool level_ok(dev_gpio_level_t l) { return (l == DEV_GPIO_LEVEL_LOW) || (l == DEV_GPIO_LEVEL_HIGH); }
static bool pull_ok(dev_gpio_pull_t p)   { return (p <= DEV_GPIO_PULL_DOWN); }

static dev_err_t map_err(dev_err_t e) {
    if (e == DEV_OK)                return DEV_OK;
    if (e == DEV_ERR_INVALID_ARG)   return DEV_ERR_INVALID_ARG;
    if (e == DEV_ERR_NULL_PTR)      return DEV_ERR_NULL_PTR;
    if (e == DEV_ERR_NOT_SUPPORTED) return DEV_ERR_NOT_SUPPORTED;
    return DEV_ERR_HW_FAILURE;
}

/* ── Lifecycle ── */

dev_err_t dev_gpio_init(void)
{
    if (g_initialized) return DEV_ERR_ALREADY_INITIALIZED;
    dev_err_t e = dev_gpio_port_init();
    if (e != DEV_OK) { g_initialized = false; return map_err(e); }
    g_initialized = true;
    return DEV_OK;
}

dev_err_t dev_gpio_deinit(void)
{
    uint16_t i; dev_err_t pe;
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    for (i = 0U; i < DEV_GPIO_CFG_MAX_PINS; i++)
        { g_cb[i].callback = NULL; g_cb[i].user_arg = NULL; g_cb[i].intr = DEV_GPIO_INTR_DISABLE; g_cb[i].enabled = false; }
    pe = dev_gpio_port_deinit(); g_initialized = false;
    return (pe != DEV_OK) ? map_err(pe) : DEV_OK;
}

bool dev_gpio_is_initialized(void) { return g_initialized; }

/* ── Input ── */

dev_err_t dev_gpio_input(dev_gpio_pin_t pin)
    { if (!g_initialized) return DEV_ERR_NOT_INITIALIZED; if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
      return map_err(dev_gpio_port_input(pin, DEV_GPIO_PULL_NONE)); }
dev_err_t dev_gpio_input_pullup(dev_gpio_pin_t pin)
    { if (!g_initialized) return DEV_ERR_NOT_INITIALIZED; if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
      return map_err(dev_gpio_port_input(pin, DEV_GPIO_PULL_UP)); }
dev_err_t dev_gpio_input_pulldown(dev_gpio_pin_t pin)
    { if (!g_initialized) return DEV_ERR_NOT_INITIALIZED; if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
      return map_err(dev_gpio_port_input(pin, DEV_GPIO_PULL_DOWN)); }

/* ── Output ── */

dev_err_t dev_gpio_output(dev_gpio_pin_t pin)
    { if (!g_initialized) return DEV_ERR_NOT_INITIALIZED; if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
      return map_err(dev_gpio_port_output(pin, DEV_GPIO_LEVEL_LOW)); }
dev_err_t dev_gpio_output_level(dev_gpio_pin_t pin, dev_gpio_level_t lvl)
    { if (!g_initialized) return DEV_ERR_NOT_INITIALIZED; if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
      if (!level_ok(lvl)) return DEV_ERR_INVALID_ARG;
      return map_err(dev_gpio_port_output(pin, lvl)); }

/* ── Read / Write / Toggle ── */

dev_err_t dev_gpio_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    dev_gpio_level_t t;
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    if (level == NULL) return DEV_ERR_NULL_PTR;
    dev_err_t e = dev_gpio_port_read(pin, &t);
    if (e != DEV_OK) return map_err(e);
    *level = t;
    return DEV_OK;
}

dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t lvl)
    { if (!g_initialized) return DEV_ERR_NOT_INITIALIZED; if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
      if (!level_ok(lvl)) return DEV_ERR_INVALID_ARG;
      return map_err(dev_gpio_port_write(pin, lvl)); }
dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin)
    { if (!g_initialized) return DEV_ERR_NOT_INITIALIZED; if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
      return map_err(dev_gpio_port_toggle(pin)); }
dev_err_t dev_gpio_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
    { if (!g_initialized) return DEV_ERR_NOT_INITIALIZED; if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
      if (!pull_ok(pull)) return DEV_ERR_INVALID_ARG;
      return map_err(dev_gpio_port_set_pull(pin, pull)); }
dev_err_t dev_gpio_high(dev_gpio_pin_t pin) { return dev_gpio_write(pin, DEV_GPIO_LEVEL_HIGH); }
dev_err_t dev_gpio_low(dev_gpio_pin_t pin)  { return dev_gpio_write(pin, DEV_GPIO_LEVEL_LOW); }

/* ── Interrupts ── */

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == DEV_ON)

dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t cb, void *arg)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;

    if (intr == DEV_GPIO_INTR_DISABLE) {
        if (cb != NULL) return DEV_ERR_INVALID_ARG;
        g_cb[pin].enabled = false;
        dev_err_t e = dev_gpio_port_interrupt(pin, DEV_GPIO_INTR_DISABLE);
        if (e != DEV_OK) { g_cb[pin].intr = DEV_GPIO_INTR_DISABLE; return map_err(e); }
        g_cb[pin].callback = NULL; g_cb[pin].user_arg = NULL; g_cb[pin].intr = DEV_GPIO_INTR_DISABLE;
        return DEV_OK;
    }
    if (cb == NULL) return DEV_ERR_NULL_PTR;
    dev_err_t e = dev_gpio_port_interrupt(pin, intr);
    if (e != DEV_OK) return map_err(e);
    g_cb[pin].callback = cb; g_cb[pin].user_arg = arg; g_cb[pin].intr = intr; g_cb[pin].enabled = false;
    return DEV_OK;
}

dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    if (g_cb[pin].callback == NULL) return DEV_ERR_INVALID_STATE;
    if (g_cb[pin].intr == DEV_GPIO_INTR_DISABLE) return DEV_ERR_INVALID_STATE;
    g_cb[pin].enabled = true;
    dev_err_t e = dev_gpio_port_interrupt_enable(pin);
    if (e != DEV_OK) { g_cb[pin].enabled = false; return map_err(e); }
    return DEV_OK;
}

dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!pin_ok(pin)) return DEV_ERR_INVALID_ARG;
    g_cb[pin].enabled = false;
    return map_err(dev_gpio_port_interrupt_disable(pin));
}

void dev_gpio_dispatch_isr(dev_gpio_pin_t pin)
{
    if (pin >= DEV_GPIO_CFG_MAX_PINS) return;
    if (!g_cb[pin].enabled || g_cb[pin].callback == NULL) return;
    g_cb[pin].callback(pin, g_cb[pin].user_arg);
}

#else

dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t cb, void *arg)
    { DEV_UNUSED(pin); DEV_UNUSED(intr); DEV_UNUSED(cb); DEV_UNUSED(arg); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin)    { DEV_UNUSED(pin); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin)   { DEV_UNUSED(pin); return DEV_ERR_NOT_SUPPORTED; }
void     dev_gpio_dispatch_isr(dev_gpio_pin_t pin)          { DEV_UNUSED(pin); }

#endif
