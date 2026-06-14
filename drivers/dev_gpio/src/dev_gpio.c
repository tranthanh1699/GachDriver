#include "dev_gpio.h"
#include "dev_common.h"

static bool g_initialized = false;

/* ── Clock helpers ── */

static void dev_gpio_stm32_enable_port_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if (port == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if (port == GPIOC)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    else if (port == GPIOD)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
    else if (port == GPIOE)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
    else
    {
        /* Port not handled — add new port here when needed */
    }
}

static void dev_gpio_stm32_setup_pin(const dev_gpio_hw_pin_t *hw)
{
    GPIO_InitTypeDef init = {0};

    init.Pin   = hw->pin;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (hw->mode) {
    case DEV_GPIO_MODE_INPUT:
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        break;

    case DEV_GPIO_MODE_OUTPUT:
        init.Mode = GPIO_MODE_OUTPUT_PP;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_WritePin(hw->port, hw->pin, GPIO_PIN_RESET);
        break;

    case DEV_GPIO_MODE_INPUT_PULLUP:
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_PULLUP;
        break;

    case DEV_GPIO_MODE_INPUT_PULLDOWN:
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_PULLDOWN;
        break;

    default:
        break;
    }

    HAL_GPIO_Init(hw->port, &init);
}

/* ── Public API ── */

dev_err_t dev_gpio_init(void)
{
    uint16_t i;

    if (g_initialized) {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    const dev_gpio_hw_pin_t *map   = dev_gpio_get_hw_map();
    uint16_t                  count = dev_gpio_get_pin_count();

    /* Enable clocks for all used GPIO ports */
    for (i = 0U; i < count; i++) {
        dev_gpio_stm32_enable_port_clock(map[i].port);
    }

    /* Configure each pin */
    for (i = 0U; i < count; i++) {
        dev_gpio_stm32_setup_pin(&map[i]);
    }

    g_initialized = true;
    return DEV_OK;
}

dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    GPIO_PinState hal_level;

#if (DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED == 1U)
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }
    if (pin >= dev_gpio_get_pin_count()) {
        return DEV_ERR_INVALID_ARG;
    }
#else
    DEV_UNUSED(g_initialized);
#endif

    hal_level = (level == DEV_GPIO_LEVEL_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    HAL_GPIO_WritePin(dev_gpio_get_hw_map()[pin].port,
                      dev_gpio_get_hw_map()[pin].pin,
                      hal_level);

    return DEV_OK;
}

dev_gpio_level_t dev_gpio_read(dev_gpio_pin_t pin)
{
#if (DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED == 1U)
    if (!g_initialized) {
        return DEV_GPIO_LEVEL_LOW;
    }
    if (pin >= dev_gpio_get_pin_count()) {
        return DEV_GPIO_LEVEL_LOW;
    }
#endif

    GPIO_PinState state = HAL_GPIO_ReadPin(dev_gpio_get_hw_map()[pin].port,
                                           dev_gpio_get_hw_map()[pin].pin);

    return (state == GPIO_PIN_SET) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW;
}

dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin)
{
#if (DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED == 1U)
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }
    if (pin >= dev_gpio_get_pin_count()) {
        return DEV_ERR_INVALID_ARG;
    }
#endif

    HAL_GPIO_TogglePin(dev_gpio_get_hw_map()[pin].port,
                       dev_gpio_get_hw_map()[pin].pin);

    return DEV_OK;
}
