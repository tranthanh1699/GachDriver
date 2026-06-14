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

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_H */
