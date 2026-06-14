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

/* ── Interrupt support ── */

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 1U)

typedef struct {
    dev_gpio_callback_t callback;
    void               *user_arg;
    bool                enabled;
} dev_gpio_isr_entry_t;

static dev_gpio_isr_entry_t g_isr_entries[DEV_GPIO_CFG_MAX_PINS];

/*
 * Map STM32 GPIO pin mask to EXTI IRQ number.
 *
 * STM32 EXTI line mapping (per pin number):
 *   Pin 0          → EXTI0
 *   Pin 1          → EXTI1
 *   Pin 2          → EXTI2
 *   Pin 3          → EXTI3
 *   Pin 4          → EXTI4
 *   Pin 5..9       → EXTI9_5
 *   Pin 10..15     → EXTI15_10
 */
static IRQn_Type dev_gpio_stm32_get_exti_irq(uint16_t hal_pin)
{
    switch (hal_pin) {
    case GPIO_PIN_0:  return EXTI0_IRQn;
    case GPIO_PIN_1:  return EXTI1_IRQn;
    case GPIO_PIN_2:  return EXTI2_IRQn;
    case GPIO_PIN_3:  return EXTI3_IRQn;
    case GPIO_PIN_4:  return EXTI4_IRQn;
    case GPIO_PIN_5:  return EXTI9_5_IRQn;
    case GPIO_PIN_6:  return EXTI9_5_IRQn;
    case GPIO_PIN_7:  return EXTI9_5_IRQn;
    case GPIO_PIN_8:  return EXTI9_5_IRQn;
    case GPIO_PIN_9:  return EXTI9_5_IRQn;
    case GPIO_PIN_10: return EXTI15_10_IRQn;
    case GPIO_PIN_11: return EXTI15_10_IRQn;
    case GPIO_PIN_12: return EXTI15_10_IRQn;
    case GPIO_PIN_13: return EXTI15_10_IRQn;
    case GPIO_PIN_14: return EXTI15_10_IRQn;
    case GPIO_PIN_15: return EXTI15_10_IRQn;
    default:          return (IRQn_Type)0xFFU;
    }
}

/*
 * Map STM32 GPIO pin mask to EXTI GPIO mode.
 */
static uint32_t dev_gpio_stm32_get_exti_mode(dev_gpio_intr_t intr)
{
    switch (intr) {
    case DEV_GPIO_INTR_RISING_EDGE:  return GPIO_MODE_IT_RISING;
    case DEV_GPIO_INTR_FALLING_EDGE: return GPIO_MODE_IT_FALLING;
    case DEV_GPIO_INTR_BOTH_EDGES:   return GPIO_MODE_IT_RISING_FALLING;
    default:                         return GPIO_MODE_IT_RISING;
    }
}

dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t             pin,
                                    dev_gpio_intr_t            intr,
                                    dev_gpio_callback_t        callback,
                                    void                      *user_arg)
{
    const dev_gpio_hw_pin_t *hw;
    GPIO_InitTypeDef         init = {0};
    IRQn_Type                irq;

    if (callback == NULL) {
        return DEV_ERR_NULL_PTR;
    }

#if (DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED == 1U)
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }
    if (pin >= dev_gpio_get_pin_count()) {
        return DEV_ERR_INVALID_ARG;
    }
#endif

    hw = &dev_gpio_get_hw_map()[pin];

    irq = dev_gpio_stm32_get_exti_irq(hw->pin);
    if (irq == (IRQn_Type)0xFFU) {
        return DEV_ERR_NOT_SUPPORTED;
    }

    /* Configure EXTI interrupt mode on the pin */
    init.Pin   = hw->pin;
    init.Mode  = dev_gpio_stm32_get_exti_mode(intr);
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(hw->port, &init);

    /* Register callback */
    g_isr_entries[pin].callback  = callback;
    g_isr_entries[pin].user_arg  = user_arg;
    g_isr_entries[pin].enabled   = true;

    /* Enable NVIC */
    HAL_NVIC_SetPriority(irq, 0x0FU, 0x0FU);
    HAL_NVIC_EnableIRQ(irq);

    return DEV_OK;
}

dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin)
{
    const dev_gpio_hw_pin_t *hw;
    GPIO_InitTypeDef         init = {0};
    IRQn_Type                irq;

#if (DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED == 1U)
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }
    if (pin >= dev_gpio_get_pin_count()) {
        return DEV_ERR_INVALID_ARG;
    }
#endif

    hw = &dev_gpio_get_hw_map()[pin];

    /* Disable NVIC */
    irq = dev_gpio_stm32_get_exti_irq(hw->pin);
    if (irq != (IRQn_Type)0xFFU) {
        HAL_NVIC_DisableIRQ(irq);
    }

    /* Restore pin to regular input mode */
    init.Pin   = hw->pin;
    init.Mode  = GPIO_MODE_INPUT;
    init.Pull  = (hw->mode == DEV_GPIO_MODE_INPUT_PULLUP)   ? GPIO_PULLUP   :
                 (hw->mode == DEV_GPIO_MODE_INPUT_PULLDOWN) ? GPIO_PULLDOWN :
                                                              GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(hw->port, &init);

    /* Clear callback */
    g_isr_entries[pin].callback  = NULL;
    g_isr_entries[pin].user_arg  = NULL;
    g_isr_entries[pin].enabled   = false;

    return DEV_OK;
}

void dev_gpio_dispatch_isr(uint16_t hal_pin)
{
    uint16_t i;

    for (i = 0U; i < dev_gpio_get_pin_count(); i++) {
        const dev_gpio_hw_pin_t *hw = &dev_gpio_get_hw_map()[i];

        if (hw->pin != hal_pin) {
            continue;
        }

        if (g_isr_entries[i].enabled && (g_isr_entries[i].callback != NULL)) {
            g_isr_entries[i].callback((dev_gpio_pin_t)i, g_isr_entries[i].user_arg);
        }

        break;
    }
}

#endif /* DEV_GPIO_CFG_INTERRUPT_ENABLED */
