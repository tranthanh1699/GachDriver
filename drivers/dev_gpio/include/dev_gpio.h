#ifndef DEV_GPIO_H
#define DEV_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "dev_gpio_cfg.h"
#include "dev_error.h"

/**
 * @brief Initialize all GPIO pins defined in DEV_GPIO_PIN_LIST.
 *
 * Enables peripheral clocks and configures each pin's mode and pull.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_ALREADY_INITIALIZED if already initialized.
 * @return DEV_ERR_HW_FAILURE if HAL configuration fails.
 *
 * @note Not reentrant. Not ISR-safe.
 */
dev_err_t dev_gpio_init(void);

/**
 * @brief Write logic level to a logical pin.
 *
 * @param pin   Logical pin ID (DEV_GPIO_LED_STATUS, etc.).
 * @param level DEV_GPIO_LEVEL_LOW or DEV_GPIO_LEVEL_HIGH.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if dev_gpio_init() not called.
 * @return DEV_ERR_INVALID_ARG if pin >= DEV_GPIO_CFG_PIN_COUNT.
 */
dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level);

/**
 * @brief Read logic level from a logical pin.
 *
 * @param pin Logical pin ID.
 *
 * @return DEV_GPIO_LEVEL_HIGH or DEV_GPIO_LEVEL_LOW.
 *         Returns DEV_GPIO_LEVEL_LOW if not initialized or invalid pin.
 */
dev_gpio_level_t dev_gpio_read(dev_gpio_pin_t pin);

/**
 * @brief Toggle output level of a logical pin.
 *
 * @param pin Logical pin ID.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if dev_gpio_init() not called.
 * @return DEV_ERR_INVALID_ARG if pin >= DEV_GPIO_CFG_PIN_COUNT.
 */
dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin);

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 1U)

/**
 * @brief Enable interrupt on a GPIO pin.
 *
 * Configures EXTI mode, NVIC priority, and registers the callback.
 * Only pins configured as input (DEV_GPIO_MODE_INPUT, INPUT_PULLUP,
 * INPUT_PULLDOWN) should have interrupts enabled.
 *
 * @param pin      Logical pin ID.
 * @param intr     Interrupt trigger: RISING_EDGE, FALLING_EDGE, BOTH_EDGES.
 * @param callback ISR callback (called from EXTI IRQ context).
 * @param user_arg User argument passed to callback.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if dev_gpio_init() not called.
 * @return DEV_ERR_INVALID_ARG if pin >= DEV_GPIO_CFG_PIN_COUNT.
 * @return DEV_ERR_NULL_PTR if callback is NULL.
 * @return DEV_ERR_NOT_SUPPORTED if pin number has no mapped EXTI IRQ.
 *
 * @note Callback runs in ISR context — must not block or call non-ISR-safe APIs.
 */
dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t             pin,
                                    dev_gpio_intr_t            intr,
                                    dev_gpio_callback_t        callback,
                                    void                      *user_arg);

/**
 * @brief Disable interrupt on a GPIO pin.
 *
 * Disables NVIC and clears the callback. Safe to call if already disabled.
 *
 * @param pin Logical pin ID.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if dev_gpio_init() not called.
 * @return DEV_ERR_INVALID_ARG if pin >= DEV_GPIO_CFG_PIN_COUNT.
 */
dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin);

/**
 * @brief ISR dispatch — called from HAL_GPIO_EXTI_Callback.
 *
 * Maps STM32 pin number → logical pin ID, looks up and invokes
 * the registered callback. PORT-ONLY: do not call from application code.
 *
 * @param hal_pin STM32 GPIO_PIN_X mask from EXTI callback.
 */
void dev_gpio_dispatch_isr(uint16_t hal_pin);

#else
/* Interrupts compiled out — inline stubs */

#define dev_gpio_interrupt_enable(pin, intr, cb, arg) \
    DEV_ERR_NOT_SUPPORTED

#define dev_gpio_interrupt_disable(pin) \
    DEV_ERR_NOT_SUPPORTED

#define dev_gpio_dispatch_isr(hal_pin)  ((void)(hal_pin))

#endif /* DEV_GPIO_CFG_INTERRUPT_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_H */
