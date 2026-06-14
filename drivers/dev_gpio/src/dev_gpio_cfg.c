#include "dev_gpio_cfg.h"

/* ── Auto-generated hardware pin map ── */

#define DEV_GPIO_BUILD_HW_MAP(name, port, pin, mode, pull) \
    [DEV_GPIO_##name] = { DEV_GPIO_##name, port, pin, mode, pull },

static const dev_gpio_hw_pin_t s_gpio_map[DEV_GPIO_CFG_PIN_COUNT] = {
    DEV_GPIO_PIN_LIST(DEV_GPIO_BUILD_HW_MAP)
};

#undef DEV_GPIO_BUILD_HW_MAP

/* ── Accessors ── */

const dev_gpio_hw_pin_t * DevGpio_GetHwMap(void)
{
    return s_gpio_map;
}

uint16_t DevGpio_GetPinCount(void)
{
    return DEV_GPIO_CFG_PIN_COUNT;
}
