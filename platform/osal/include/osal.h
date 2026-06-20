#ifndef OSAL_H
#define OSAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_cfg.h"

/**
 * @brief Initialize the OSAL layer.
 *
 * Must be called once before any other OSAL function.
 * Initializes the selected backend port.
 *
 * @return DEV_OK on success, error code otherwise.
 * @return DEV_ERR_ALREADY_INITIALIZED if called more than once.
 */
dev_err_t osal_init(void);

/**
 * @brief Check if the OSAL layer has been initialized.
 *
 * @return true if osal_init() has completed successfully.
 */
bool osal_is_initialized(void);

/**
 * @brief Get the current system tick in milliseconds.
 *
 * @return Millisecond tick counter value.
 *         Behavior is undefined if osal_init() has not been called.
 */
uint32_t osal_get_tick_ms(void);

/**
 * @brief Blocking delay in milliseconds.
 *
 * @param delay_ms Number of milliseconds to delay.
 *                 Behavior is undefined if osal_init() has not been called.
 */
void osal_delay_ms(uint32_t delay_ms);

/**
 * @brief Check whether a real-time kernel is running.
 *
 * @return true if an RTOS kernel scheduler is active.
 *         Always false in bare-metal builds.
 */
bool osal_is_kernel_running(void);

/**
 * @brief Start the real-time kernel scheduler.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_SUPPORTED when the bare-metal backend is active.
 * @return DEV_ERR_NOT_INITIALIZED if osal_init() has not been called.
 */
dev_err_t osal_kernel_start(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_H */
