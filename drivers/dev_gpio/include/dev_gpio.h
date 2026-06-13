#ifndef DEV_GPIO_H
#define DEV_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "dev_gpio_cfg.h"
#include "dev_error.h"

/**
 * @brief Initialize the GPIO driver with a board-provided configuration.
 *
 * Validates all channels, initializes the port layer, and configures each channel.
 * Must be called before any other GPIO API.
 *
 * @param config Pointer to board-provided GPIO configuration.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NULL_PTR if config or config->channels is NULL.
 * @return DEV_ERR_INVALID_ARG if channel_count is zero or exceeds DEV_GPIO_CFG_MAX_CHANNELS.
 * @return DEV_ERR_ALREADY_INITIALIZED if already initialized.
 * @return DEV_ERR_CONFIG if duplicate channels are detected.
 * @return DEV_ERR_NOT_SUPPORTED if interrupts are compiled out but a channel requests interrupts.
 * @return DEV_ERR_HW_FAILURE if port initialization fails.
 *
 * @note Must be called before any other GPIO API.
 * @note Not reentrant. Call once during system startup.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_init(const dev_gpio_config_t *config);

/**
 * @brief De-initialize the GPIO driver.
 *
 * Disables all interrupts, clears callbacks, deinitializes the port,
 * and forces the module to UNINITIALIZED state regardless of port errors.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_HW_FAILURE if port deinit fails (module is still deinitialized).
 *
 * @note Must be called when GPIO is initialized.
 * @note Not reentrant.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_deinit(void);

/**
 * @brief Read the logic level of a GPIO channel.
 *
 * Reads into a local temporary; *level is written only on success.
 *
 * @param channel Logical GPIO channel ID.
 * @param level   Output pointer for the read level.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_NULL_PTR if level is NULL.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 * @return DEV_ERR_HW_FAILURE if port read fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe unless the port documents support.
 * @note *level is NOT modified on failure.
 */
dev_err_t dev_gpio_read(dev_gpio_channel_t channel,
                        dev_gpio_level_t *level);

/**
 * @brief Write a logic level to a GPIO channel.
 *
 * @param channel Logical GPIO channel ID.
 * @param level   Output level to write (LOW or HIGH).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found or level is invalid.
 * @return DEV_ERR_INVALID_STATE if channel is configured as input-only.
 * @return DEV_ERR_HW_FAILURE if port write fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe unless the port documents support.
 */
dev_err_t dev_gpio_write(dev_gpio_channel_t channel,
                         dev_gpio_level_t level);

/**
 * @brief Toggle the output level of a GPIO channel.
 *
 * @param channel Logical GPIO channel ID.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 * @return DEV_ERR_INVALID_STATE if channel is configured as input-only.
 * @return DEV_ERR_HW_FAILURE if port toggle fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe unless the port documents support.
 */
dev_err_t dev_gpio_toggle(dev_gpio_channel_t channel);

/**
 * @brief Set the direction of a GPIO channel at runtime.
 *
 * @param channel   Logical GPIO channel ID.
 * @param direction Requested direction.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found or direction is invalid.
 * @return DEV_ERR_NOT_SUPPORTED if the port does not support the requested direction.
 * @return DEV_ERR_HW_FAILURE if port fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_set_direction(dev_gpio_channel_t channel,
                                 dev_gpio_direction_t direction);

/**
 * @brief Set the pull mode of a GPIO channel at runtime.
 *
 * @param channel Logical GPIO channel ID.
 * @param pull    Requested pull mode.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found or pull is invalid.
 * @return DEV_ERR_NOT_SUPPORTED if the port does not support the requested pull mode.
 * @return DEV_ERR_HW_FAILURE if port fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_set_pull(dev_gpio_channel_t channel,
                            dev_gpio_pull_t pull);

/**
 * @brief Configure the interrupt mode for a GPIO channel.
 *
 * @param channel   Logical GPIO channel ID.
 * @param interrupt Requested interrupt mode.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found or interrupt mode is invalid.
 * @return DEV_ERR_NOT_SUPPORTED if the port does not support the requested mode.
 * @return DEV_ERR_HW_FAILURE if port fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_config_interrupt(dev_gpio_channel_t channel,
                                    dev_gpio_intr_type_t interrupt);

/**
 * @brief Register an ISR callback for a GPIO channel.
 *
 * Callback may be NULL to clear a previous registration.
 * The interrupt must be enabled separately via dev_gpio_enable_interrupt().
 *
 * @param channel   Logical GPIO channel ID.
 * @param callback  ISR callback function (may be NULL).
 * @param user_arg  User argument passed to callback (may be NULL).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_register_callback(dev_gpio_channel_t channel,
                                     dev_gpio_isr_callback_t callback,
                                     void *user_arg);

/**
 * @brief Enable interrupt for a GPIO channel.
 *
 * Marks common state enabled BEFORE calling port, so any immediate
 * hardware interrupt is not dropped. Rolls back on port failure.
 *
 * @param channel Logical GPIO channel ID.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 * @return DEV_ERR_HW_FAILURE if port enable fails (common state rolled back).
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_enable_interrupt(dev_gpio_channel_t channel);

/**
 * @brief Disable interrupt for a GPIO channel.
 *
 * Marks common state disabled BEFORE calling port. Safe to call
 * even if already disabled (idempotent).
 *
 * @param channel Logical GPIO channel ID.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 * @return DEV_ERR_HW_FAILURE if port disable fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_disable_interrupt(dev_gpio_channel_t channel);

/**
 * @brief Check if the GPIO driver is initialized.
 *
 * @return true if initialized, false otherwise.
 *
 * @note Reentrant.
 * @note ISR-safe.
 */
bool dev_gpio_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_H */
