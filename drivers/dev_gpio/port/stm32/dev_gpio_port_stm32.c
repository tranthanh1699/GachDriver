#include "dev_gpio_port_stm32.h"

/* Auto-generated from X-Macro */
#define DEV_GPIO_PORT_BUILD_MAP(name, port, pin, mode, pull) \
    [DEV_GPIO_##name] = { DEV_GPIO_##name, port, pin, mode, pull },

static const dev_gpio_hw_pin_t s_gpio_map[DEV_GPIO_CFG_PIN_COUNT] = {
    DEV_GPIO_PIN_LIST(DEV_GPIO_PORT_BUILD_MAP)
};
#undef DEV_GPIO_PORT_BUILD_MAP

const dev_gpio_hw_pin_t * dev_gpio_port_get_hw_map(void) { return s_gpio_map; }
uint16_t dev_gpio_port_get_pin_count(void) { return DEV_GPIO_CFG_PIN_COUNT; }

/* ── Clock enable ── */

static void stm32_enable_clock(GPIO_TypeDef *port)
{
    if      (port == GPIOA) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
    else if (port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
    else if (port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
    else if (port == GPIOD) { __HAL_RCC_GPIOD_CLK_ENABLE(); }
    else if (port == GPIOE) { __HAL_RCC_GPIOE_CLK_ENABLE(); }
}

static GPIO_PinState stm32_level(dev_gpio_level_t l)
{
    return (l == DEV_GPIO_LEVEL_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

static dev_gpio_level_t stm32_read_level(GPIO_PinState s)
{
    return (s == GPIO_PIN_SET) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW;
}

/* ── Port API ── */

dev_err_t dev_gpio_port_init(void)
{
    uint16_t i;
    for (i = 0U; i < DEV_GPIO_CFG_PIN_COUNT; i++) {
        stm32_enable_clock(s_gpio_map[i].port);
    }
    for (i = 0U; i < DEV_GPIO_CFG_PIN_COUNT; i++) {
        GPIO_InitTypeDef init = {0};
        const dev_gpio_hw_pin_t *hw = &s_gpio_map[i];
        init.Pin   = hw->hal_pin;
        init.Speed = GPIO_SPEED_FREQ_LOW;

        switch (hw->mode) {
        case DEV_GPIO_PORT_MODE_INPUT:
            init.Mode = GPIO_MODE_INPUT; init.Pull = GPIO_NOPULL; break;
        case DEV_GPIO_PORT_MODE_OUTPUT:
            init.Mode = GPIO_MODE_OUTPUT_PP; init.Pull = GPIO_NOPULL;
            HAL_GPIO_WritePin(hw->port, hw->hal_pin, GPIO_PIN_RESET); break;
        case DEV_GPIO_PORT_MODE_INPUT_PULLUP:
            init.Mode = GPIO_MODE_INPUT; init.Pull = GPIO_PULLUP; break;
        case DEV_GPIO_PORT_MODE_INPUT_PULLDOWN:
            init.Mode = GPIO_MODE_INPUT; init.Pull = GPIO_PULLDOWN; break;
        }
        HAL_GPIO_Init(hw->port, &init);
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)    { return DEV_OK; }
dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    GPIO_InitTypeDef init = {0};
    const dev_gpio_hw_pin_t *hw = &s_gpio_map[pin];
    init.Pin = hw->hal_pin; init.Mode = GPIO_MODE_INPUT; init.Speed = GPIO_SPEED_FREQ_LOW;
    init.Pull = (pull == DEV_GPIO_PULL_UP) ? GPIO_PULLUP :
                (pull == DEV_GPIO_PULL_DOWN) ? GPIO_PULLDOWN : GPIO_NOPULL;
    HAL_GPIO_Init(hw->port, &init);
    return DEV_OK;
}

dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    GPIO_InitTypeDef init = {0};
    const dev_gpio_hw_pin_t *hw = &s_gpio_map[pin];
    init.Pin = hw->hal_pin; init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pull = GPIO_NOPULL; init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(hw->port, &init);
    HAL_GPIO_WritePin(hw->port, hw->hal_pin, stm32_level(level));
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    if (level == NULL) return DEV_ERR_NULL_PTR;
    *level = stm32_read_level(HAL_GPIO_ReadPin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin));
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    HAL_GPIO_WritePin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin, stm32_level(level));
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin)
{
    HAL_GPIO_TogglePin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin);
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    return dev_gpio_port_input(pin, pull);
}

dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr)
{
    GPIO_InitTypeDef init = {0};
    const dev_gpio_hw_pin_t *hw = &s_gpio_map[pin];
    init.Pin = hw->hal_pin; init.Speed = GPIO_SPEED_FREQ_LOW; init.Pull = GPIO_NOPULL;
    switch (intr) {
    case DEV_GPIO_INTR_DISABLE:      init.Mode = GPIO_MODE_INPUT;           break;
    case DEV_GPIO_INTR_RISING_EDGE:  init.Mode = GPIO_MODE_IT_RISING;       break;
    case DEV_GPIO_INTR_FALLING_EDGE: init.Mode = GPIO_MODE_IT_FALLING;      break;
    case DEV_GPIO_INTR_BOTH_EDGES:   init.Mode = GPIO_MODE_IT_RISING_FALLING; break;
    default: return DEV_ERR_INVALID_ARG;
    }
    HAL_GPIO_Init(hw->port, &init);
    return DEV_OK;
}

static IRQn_Type stm32_get_exti_irq(uint16_t hal_pin)
{
    switch (hal_pin) {
    case GPIO_PIN_0:  return EXTI0_IRQn;
    case GPIO_PIN_1:  return EXTI1_IRQn;
    case GPIO_PIN_2:  return EXTI2_IRQn;
    case GPIO_PIN_3:  return EXTI3_IRQn;
    case GPIO_PIN_4:  return EXTI4_IRQn;
    case GPIO_PIN_5:  case GPIO_PIN_6: case GPIO_PIN_7:
    case GPIO_PIN_8:  case GPIO_PIN_9:  return EXTI9_5_IRQn;
    case GPIO_PIN_10: case GPIO_PIN_11: case GPIO_PIN_12:
    case GPIO_PIN_13: case GPIO_PIN_14: case GPIO_PIN_15: return EXTI15_10_IRQn;
    default: return (IRQn_Type)0xFFU;
    }
}

dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin)
{
    IRQn_Type irq = stm32_get_exti_irq(s_gpio_map[pin].hal_pin);
    if (irq == (IRQn_Type)0xFFU) return DEV_ERR_NOT_SUPPORTED;
    HAL_NVIC_SetPriority(irq, 0x0FU, 0x0FU);
    HAL_NVIC_EnableIRQ(irq);
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin)
{
    IRQn_Type irq = stm32_get_exti_irq(s_gpio_map[pin].hal_pin);
    if (irq != (IRQn_Type)0xFFU) HAL_NVIC_DisableIRQ(irq);
    return DEV_OK;
}

void HAL_GPIO_EXTI_Callback(uint16_t hal_pin)
{
    uint16_t i;
    for (i = 0U; i < DEV_GPIO_CFG_PIN_COUNT; i++) {
        if (s_gpio_map[i].hal_pin == hal_pin) {
            dev_gpio_dispatch_isr((dev_gpio_pin_t)i);
            return;
        }
    }
}
