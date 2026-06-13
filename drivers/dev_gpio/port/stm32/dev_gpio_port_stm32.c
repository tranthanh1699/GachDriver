/**
 * @file    dev_gpio_port_stm32.c
 * @brief   STM32H7xx GPIO port implementation for dev_gpio.
 *
 * Maps logical dev_gpio channels to STM32 GPIO ports and pins.
 * This is the ONLY file that includes stm32h7xx_hal.h for GPIO access.
 */

#include "dev_gpio_port_stm32.h"
#include "stm32h7xx_hal.h"

/* ── Pin mapping table (board-specific) ── */

/*
 * STM32H743 pin assignments for application GPIO:
 *
 *   PB0 → GPIOB, Pin 0  (output, e.g., LED)
 *   PB1 → GPIOB, Pin 1  (input,  e.g., button)
 */
#define STM32_PORT_CHANNEL_COUNT  (2U)

static const dev_gpio_port_pin_t m_pin_map[STM32_PORT_CHANNEL_COUNT] = {
    { DEV_GPIO_CHANNEL_PB0, GPIOB, GPIO_PIN_0 },
    { DEV_GPIO_CHANNEL_PB1, GPIOB, GPIO_PIN_1 },
};

/* ── Helpers ── */

static uint16_t stm32_find_index(dev_gpio_channel_t channel)
{
    uint16_t i;
    for (i = 0U; i < STM32_PORT_CHANNEL_COUNT; i++) {
        if (m_pin_map[i].channel == channel) {
            return i;
        }
    }
    return STM32_PORT_CHANNEL_COUNT; /* not found */
}

static GPIO_PinState stm32_map_level(dev_gpio_level_t level)
{
    return (level == DEV_GPIO_LEVEL_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

static dev_gpio_level_t stm32_map_pin_state(GPIO_PinState state)
{
    return (state == GPIO_PIN_SET) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW;
}

/* ── Port API ── */

dev_err_t dev_gpio_port_init(const dev_gpio_config_t *config)
{
    uint16_t i;

    DEV_UNUSED(config);

    /* Enable GPIOB clock (required for PB0, PB1) */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure each pin */
    for (i = 0U; i < STM32_PORT_CHANNEL_COUNT; i++) {
        dev_err_t err;
        /* Find this pin's config in the common config */
        const dev_gpio_channel_config_t *ch_cfg = NULL;
        uint16_t j;
        for (j = 0U; j < config->channel_count; j++) {
            if (config->channels[j].channel == m_pin_map[i].channel) {
                ch_cfg = &config->channels[j];
                break;
            }
        }
        if (ch_cfg == NULL) {
            continue;
        }

        err = dev_gpio_port_config_channel(ch_cfg);
        if (err != DEV_OK) {
            return err;
        }
    }

    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    uint16_t i;

    /* De-initialize all mapped pins to analog (low power default) */
    for (i = 0U; i < STM32_PORT_CHANNEL_COUNT; i++) {
        HAL_GPIO_DeInit(m_pin_map[i].port, m_pin_map[i].pin);
    }

    __HAL_RCC_GPIOB_CLK_DISABLE();

    return DEV_OK;
}

dev_err_t dev_gpio_port_config_channel(const dev_gpio_channel_config_t *channel_config)
{
    GPIO_InitTypeDef gpio_init = {0};
    uint16_t idx;

    if (channel_config == NULL) {
        return DEV_ERR_NULL_PTR;
    }

    idx = stm32_find_index(channel_config->channel);
    if (idx >= STM32_PORT_CHANNEL_COUNT) {
        return DEV_ERR_INVALID_ARG;
    }

    gpio_init.Pin  = m_pin_map[idx].pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    /* Map direction */
    switch (channel_config->direction) {
    case DEV_GPIO_DIRECTION_INPUT:
        gpio_init.Mode = GPIO_MODE_INPUT;
        break;
    case DEV_GPIO_DIRECTION_OUTPUT:
        gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
        break;
    case DEV_GPIO_DIRECTION_INPUT_OUTPUT:
        return DEV_ERR_NOT_SUPPORTED; /* STM32 OD mode not implemented in this example */
    default:
        return DEV_ERR_INVALID_ARG;
    }

    /* Map pull */
    switch (channel_config->pull) {
    case DEV_GPIO_PULL_NONE:
        gpio_init.Pull = GPIO_NOPULL;
        break;
    case DEV_GPIO_PULL_UP:
        gpio_init.Pull = GPIO_PULLUP;
        break;
    case DEV_GPIO_PULL_DOWN:
        gpio_init.Pull = GPIO_PULLDOWN;
        break;
    default:
        return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_Init(m_pin_map[idx].port, &gpio_init);

    /* Set default output level */
    if (channel_config->direction == DEV_GPIO_DIRECTION_OUTPUT) {
        HAL_GPIO_WritePin(m_pin_map[idx].port, m_pin_map[idx].pin,
                          stm32_map_level(channel_config->default_level));
    }

    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_channel_t channel,
                             dev_gpio_level_t *level)
{
    uint16_t idx;

    if (level == NULL) {
        return DEV_ERR_NULL_PTR;
    }

    idx = stm32_find_index(channel);
    if (idx >= STM32_PORT_CHANNEL_COUNT) {
        return DEV_ERR_INVALID_ARG;
    }

    *level = stm32_map_pin_state(
        HAL_GPIO_ReadPin(m_pin_map[idx].port, m_pin_map[idx].pin));

    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_channel_t channel,
                              dev_gpio_level_t level)
{
    uint16_t idx;

    idx = stm32_find_index(channel);
    if (idx >= STM32_PORT_CHANNEL_COUNT) {
        return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_WritePin(m_pin_map[idx].port, m_pin_map[idx].pin,
                      stm32_map_level(level));

    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_channel_t channel)
{
    uint16_t idx;

    idx = stm32_find_index(channel);
    if (idx >= STM32_PORT_CHANNEL_COUNT) {
        return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_TogglePin(m_pin_map[idx].port, m_pin_map[idx].pin);

    return DEV_OK;
}

dev_err_t dev_gpio_port_set_direction(dev_gpio_channel_t channel,
                                      dev_gpio_direction_t direction)
{
    uint16_t idx;
    GPIO_InitTypeDef gpio_init = {0};

    idx = stm32_find_index(channel);
    if (idx >= STM32_PORT_CHANNEL_COUNT) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Read current config to preserve pull setting */
    gpio_init.Pin  = m_pin_map[idx].pin;
    gpio_init.Pull = GPIO_NOPULL; /* Simplified — real impl would track state */
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (direction) {
    case DEV_GPIO_DIRECTION_INPUT:
        gpio_init.Mode = GPIO_MODE_INPUT;
        break;
    case DEV_GPIO_DIRECTION_OUTPUT:
        gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
        break;
    case DEV_GPIO_DIRECTION_INPUT_OUTPUT:
        return DEV_ERR_NOT_SUPPORTED;
    default:
        return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_Init(m_pin_map[idx].port, &gpio_init);

    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_channel_t channel,
                                 dev_gpio_pull_t pull)
{
    uint16_t idx;
    GPIO_InitTypeDef gpio_init = {0};

    idx = stm32_find_index(channel);
    if (idx >= STM32_PORT_CHANNEL_COUNT) {
        return DEV_ERR_INVALID_ARG;
    }

    gpio_init.Pin  = m_pin_map[idx].pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (pull) {
    case DEV_GPIO_PULL_NONE:
        gpio_init.Pull = GPIO_NOPULL;
        break;
    case DEV_GPIO_PULL_UP:
        gpio_init.Pull = GPIO_PULLUP;
        break;
    case DEV_GPIO_PULL_DOWN:
        gpio_init.Pull = GPIO_PULLDOWN;
        break;
    default:
        return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_Init(m_pin_map[idx].port, &gpio_init);

    return DEV_OK;
}

dev_err_t dev_gpio_port_config_interrupt(dev_gpio_channel_t channel,
                                         dev_gpio_intr_type_t interrupt)
{
    uint16_t idx;
    uint32_t hal_mode;

    idx = stm32_find_index(channel);
    if (idx >= STM32_PORT_CHANNEL_COUNT) {
        return DEV_ERR_INVALID_ARG;
    }

    switch (interrupt) {
    case DEV_GPIO_INTR_DISABLE:
        hal_mode = GPIO_MODE_IT_DISABLE;
        break;
    case DEV_GPIO_INTR_RISING_EDGE:
        hal_mode = GPIO_MODE_IT_RISING;
        break;
    case DEV_GPIO_INTR_FALLING_EDGE:
        hal_mode = GPIO_MODE_IT_FALLING;
        break;
    case DEV_GPIO_INTR_BOTH_EDGES:
        hal_mode = GPIO_MODE_IT_RISING_FALLING;
        break;
    case DEV_GPIO_INTR_LOW_LEVEL:
    case DEV_GPIO_INTR_HIGH_LEVEL:
        return DEV_ERR_NOT_SUPPORTED; /* Level interrupts require EXTI config */
    default:
        return DEV_ERR_INVALID_ARG;
    }

    /* Simplified: configure IT mode. Full impl needs EXTI and NVIC setup. */
    {
        GPIO_InitTypeDef gpio_init = {0};
        gpio_init.Pin   = m_pin_map[idx].pin;
        gpio_init.Mode  = hal_mode;
        gpio_init.Pull  = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(m_pin_map[idx].port, &gpio_init);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_port_enable_interrupt(dev_gpio_channel_t channel)
{
    uint16_t idx;
    IRQn_Type irq;

    idx = stm32_find_index(channel);
    if (idx >= STM32_PORT_CHANNEL_COUNT) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Determine EXTI IRQ number for this pin */
    /* PB0 → EXTI0, PB1 → EXTI1 */
    switch (m_pin_map[idx].pin) {
    case GPIO_PIN_0:  irq = EXTI0_IRQn;   break;
    case GPIO_PIN_1:  irq = EXTI1_IRQn;   break;
    default:
        return DEV_ERR_NOT_SUPPORTED;
    }

    HAL_NVIC_SetPriority(irq, 0x0FU, 0x0FU);
    HAL_NVIC_EnableIRQ(irq);

    return DEV_OK;
}

dev_err_t dev_gpio_port_disable_interrupt(dev_gpio_channel_t channel)
{
    uint16_t idx;
    IRQn_Type irq;

    idx = stm32_find_index(channel);
    if (idx >= STM32_PORT_CHANNEL_COUNT) {
        return DEV_ERR_INVALID_ARG;
    }

    switch (m_pin_map[idx].pin) {
    case GPIO_PIN_0:  irq = EXTI0_IRQn;   break;
    case GPIO_PIN_1:  irq = EXTI1_IRQn;   break;
    default:
        return DEV_ERR_NOT_SUPPORTED;
    }

    HAL_NVIC_DisableIRQ(irq);

    return DEV_OK;
}

/* ── STM32 EXTI interrupt handlers ── */

/*
 * These handlers are called by the STM32 HAL EXTI callback mechanism.
 * They dispatch to the common dev_gpio ISR handler.
 *
 * For a full implementation, you would implement HAL_GPIO_EXTI_Callback()
 * in stm32h7xx_it.c and call dev_gpio_dispatch_isr() from there.
 */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    dev_gpio_channel_t channel;

    switch (pin) {
    case GPIO_PIN_0:
        channel = DEV_GPIO_CHANNEL_PB0;
        break;
    case GPIO_PIN_1:
        channel = DEV_GPIO_CHANNEL_PB1;
        break;
    default:
        return;
    }

    dev_gpio_dispatch_isr(channel);
}
