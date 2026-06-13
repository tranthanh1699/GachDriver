#ifndef DEV_GPIO_H
#define DEV_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "dev_gpio_cfg.h"
#include "dev_error.h"

/**
 * @brief Initialize the GPIO wrapper and port layer.
 *
 * Calls dev_gpio_port_init() to enable clocks and prepare hardware.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_ALREADY_INITIALIZED if already initialized.
 * @return DEV_ERR_HW_FAILURE if port initialization fails.
 *
 * @note Not reentrant. Not ISR-safe.
 */
dev_err_t dev_gpio_init(void);

/**
 * @brief De-initialize the GPIO wrapper.
 *
 * Clears all callbacks, disables interrupts, deinitializes the port.
 * Module is forced to UNINITIALIZED regardless of port errors.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_HW_FAILURE if port deinit fails.
 *
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_deinit(void);

/**
 * @brief Check if GPIO wrapper is initialized.
 *
 * @return true if initialized.
 *
 * @note ISR-safe. Reentrant.
 */
bool dev_gpio_is_initialized(void);

/**
 * @brief Configure pin as input without pull.
 *
 * @param pin Logical pin ID (0..DEV_GPIO_CFG_PIN_COUNT-1).
 * @return DEV_OK, DEV_ERR_NOT_INITIALIZED, DEV_ERR_INVALID_ARG, DEV_ERR_HW_FAILURE.
 */
dev_err_t dev_gpio_input(dev_gpio_pin_t pin);

/**
 * @brief Configure pin as input with pull-up.
 */
dev_err_t dev_gpio_input_pullup(dev_gpio_pin_t pin);

/**
 * @brief Configure pin as input with pull-down.
 */
dev_err_t dev_gpio_input_pulldown(dev_gpio_pin_t pin);

/**
 * @brief Configure pin as output (default LOW).
 */
dev_err_t dev_gpio_output(dev_gpio_pin_t pin);

/**
 * @brief Configure pin as output with specified initial level.
 *
 * Avoids output glitches where supported by hardware.
 *
 * @param pin Logical pin ID.
 * @param level Initial output level.
 * @return DEV_ERR_INVALID_ARG if level is not LOW or HIGH.
 */
dev_err_t dev_gpio_output_level(dev_gpio_pin_t pin, dev_gpio_level_t level);

/**
 * @brief Read the logic level of a pin.
 *
 * Reads into a local temporary; *level is written only on success.
 *
 * @param pin Logical pin ID.
 * @param level Output pointer (must not be NULL).
 * @return DEV_ERR_NULL_PTR if level is NULL.
 */
dev_err_t dev_gpio_read(dev_gpio_pin_t pin, dev_gpio_level_t *level);

/**
 * @brief Write a logic level to a pin.
 *
 * @return DEV_ERR_INVALID_ARG if level is not LOW or HIGH.
 */
dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level);

/**
 * @brief Toggle the output level of a pin.
 */
dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin);

/**
 * @brief Set pull mode of a pin at runtime.
 */
dev_err_t dev_gpio_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull);

/**
 * @brief Write HIGH to a pin (convenience).
 */
dev_err_t dev_gpio_high(dev_gpio_pin_t pin);

/**
 * @brief Write LOW to a pin (convenience).
 */
dev_err_t dev_gpio_low(dev_gpio_pin_t pin);

/**
 * @brief Configure interrupt mode and register callback.
 *
 * @param pin Logical pin ID.
 * @param intr Interrupt mode. Use DEV_GPIO_INTR_DISABLE with cb=NULL to clear.
 * @param callback ISR callback. Must not be NULL unless intr is DISABLE.
 * @param user_arg User argument passed to callback.
 *
 * @return DEV_ERR_NULL_PTR if intr != DISABLE and callback is NULL.
 * @return DEV_ERR_NOT_SUPPORTED if intr mode not supported by port.
 * @return DEV_ERR_NOT_SUPPORTED if DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U.
 */
dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t callback, void *user_arg);

/**
 * @brief Enable interrupt for a pin.
 *
 * Requires a callback to have been registered via dev_gpio_interrupt().
 * Marks enabled BEFORE port call; rolls back on port failure.
 *
 * @return DEV_ERR_INVALID_STATE if no callback registered or intr is DISABLE.
 */
dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin);

/**
 * @brief Disable interrupt for a pin.
 *
 * Marks disabled BEFORE port call. Safe to call when already disabled.
 */
dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_H */
