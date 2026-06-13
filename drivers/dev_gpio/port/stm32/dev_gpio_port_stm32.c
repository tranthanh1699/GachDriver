#include "dev_gpio_port_stm32.h"
#include "dev_gpio_cfg.h"
#include "dev_compiler.h"

#define STM32_PORT_PIN_COUNT  DEV_GPIO_CFG_PIN_COUNT

static const dev_gpio_hw_pin_t s_gpio_map[STM32_PORT_PIN_COUNT] = {
    [DEV_GPIO_LED_STATUS]  = { DEV_GPIO_LED_STATUS,  GPIOB, GPIO_PIN_0 },
    [DEV_GPIO_BUTTON_USER] = { DEV_GPIO_BUTTON_USER, GPIOC, GPIO_PIN_13 },
};

static bool s_pin_valid(dev_gpio_pin_t pin)
{
    if (pin >= STM32_PORT_PIN_COUNT) { return false; }
    return (s_gpio_map[pin].port != NULL);
}

static void stm32_enable_clocks(void)
{
    uint16_t i;
    for (i = 0U; i < STM32_PORT_PIN_COUNT; i++) {
        if (!s_pin_valid((dev_gpio_pin_t)i)) { continue; }
        if (s_gpio_map[i].port == GPIOA) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
        else if (s_gpio_map[i].port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
        else if (s_gpio_map[i].port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
    }
}

static GPIO_PinState stm32_map_level(dev_gpio_level_t level)
{
    return (level == DEV_GPIO_LEVEL_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

static dev_gpio_level_t stm32_map_pin_state(GPIO_PinState state)
{
    return (state == GPIO_PIN_SET) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW;
}

dev_err_t dev_gpio_port_init(void)
{
    stm32_enable_clocks();
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    uint16_t i;
    for (i = 0U; i < STM32_PORT_PIN_COUNT; i++) {
        if (s_pin_valid((dev_gpio_pin_t)i)) {
            HAL_GPIO_DeInit(s_gpio_map[i].port, s_gpio_map[i].hal_pin);
        }
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    GPIO_InitTypeDef init = {0};

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    init.Pin   = s_gpio_map[pin].hal_pin;
    init.Mode  = GPIO_MODE_INPUT;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (pull) {
    case DEV_GPIO_PULL_NONE: init.Pull = GPIO_NOPULL;   break;
    case DEV_GPIO_PULL_UP:   init.Pull = GPIO_PULLUP;   break;
    case DEV_GPIO_PULL_DOWN: init.Pull = GPIO_PULLDOWN; break;
    default: return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_Init(s_gpio_map[pin].port, &init);
    return DEV_OK;
}

dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t initial_level)
{
    GPIO_InitTypeDef init = {0};

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    init.Pin   = s_gpio_map[pin].hal_pin;
    init.Mode  = GPIO_MODE_OUTPUT_PP;
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(s_gpio_map[pin].port, &init);
    HAL_GPIO_WritePin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin,
                      stm32_map_level(initial_level));
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    if (level == NULL) { return DEV_ERR_NULL_PTR; }

    *level = stm32_map_pin_state(
        HAL_GPIO_ReadPin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin));
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    HAL_GPIO_WritePin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin,
                      stm32_map_level(level));
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    HAL_GPIO_TogglePin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin);
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    GPIO_InitTypeDef init = {0};

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    init.Pin   = s_gpio_map[pin].hal_pin;
    init.Mode  = GPIO_MODE_INPUT;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (pull) {
    case DEV_GPIO_PULL_NONE: init.Pull = GPIO_NOPULL;   break;
    case DEV_GPIO_PULL_UP:   init.Pull = GPIO_PULLUP;   break;
    case DEV_GPIO_PULL_DOWN: init.Pull = GPIO_PULLDOWN; break;
    default: return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_Init(s_gpio_map[pin].port, &init);
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr)
{
    GPIO_InitTypeDef init = {0};

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    init.Pin   = s_gpio_map[pin].hal_pin;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (intr) {
    case DEV_GPIO_INTR_DISABLE:      init.Mode = GPIO_MODE_INPUT;           break;
    case DEV_GPIO_INTR_RISING_EDGE:  init.Mode = GPIO_MODE_IT_RISING;       break;
    case DEV_GPIO_INTR_FALLING_EDGE: init.Mode = GPIO_MODE_IT_FALLING;      break;
    case DEV_GPIO_INTR_BOTH_EDGES:   init.Mode = GPIO_MODE_IT_RISING_FALLING; break;
    default: return DEV_ERR_NOT_SUPPORTED;
    }

    init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(s_gpio_map[pin].port, &init);
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin)
{
    IRQn_Type irq;

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    switch (s_gpio_map[pin].hal_pin) {
    case GPIO_PIN_0:  irq = EXTI0_IRQn;  break;
    case GPIO_PIN_13: irq = EXTI15_10_IRQn; break;
    default: return DEV_ERR_NOT_SUPPORTED;
    }

    HAL_NVIC_SetPriority(irq, 0x0FU, 0x0FU);
    HAL_NVIC_EnableIRQ(irq);
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin)
{
    IRQn_Type irq;

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    switch (s_gpio_map[pin].hal_pin) {
    case GPIO_PIN_0:  irq = EXTI0_IRQn;  break;
    case GPIO_PIN_13: irq = EXTI15_10_IRQn; break;
    default: return DEV_ERR_NOT_SUPPORTED;
    }

    HAL_NVIC_DisableIRQ(irq);
    return DEV_OK;
}

void HAL_GPIO_EXTI_Callback(uint16_t hal_pin)
{
    dev_gpio_pin_t pin;

    switch (hal_pin) {
    case GPIO_PIN_0:  pin = DEV_GPIO_LED_STATUS;  break;
    case GPIO_PIN_13: pin = DEV_GPIO_BUTTON_USER; break;
    default: return;
    }

    dev_gpio_dispatch_isr(pin);
}
