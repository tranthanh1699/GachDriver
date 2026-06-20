#ifndef OSAL_PORT_H
#define OSAL_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"

/**
 * @brief Initialize the OSAL backend.
 *
 * Called once by osal_init(). The port implementation shall
 * perform any backend-specific setup required before the tick
 * or delay functions can be used.
 *
 * @return DEV_OK on success, error code otherwise.
 */
dev_err_t osal_port_init(void);

/**
 * @brief Get the current system tick in milliseconds.
 *
 * @return Millisecond tick counter value.
 */
uint32_t osal_port_get_tick_ms(void);

/**
 * @brief Blocking delay in milliseconds.
 *
 * @param delay_ms Number of milliseconds to delay.
 */
void osal_port_delay_ms(uint32_t delay_ms);

/**
 * @brief Check whether a real-time kernel is running.
 *
 * In bare-metal mode, this always returns false.
 *
 * @return true if an RTOS kernel is running, false otherwise.
 */
bool osal_port_is_kernel_running(void);

/**
 * @brief Start the real-time kernel scheduler.
 *
 * In bare-metal mode, this returns DEV_ERR_NOT_SUPPORTED.
 *
 * @return DEV_OK on success, DEV_ERR_NOT_SUPPORTED in bare-metal.
 */
dev_err_t osal_port_kernel_start(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_PORT_H */
