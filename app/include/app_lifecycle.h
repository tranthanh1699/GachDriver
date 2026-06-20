#ifndef APP_LIFECYCLE_H
#define APP_LIFECYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_error.h"

/**
 * @brief Application lifecycle callbacks.
 *
 * These functions are called by svc_sm at defined points during
 * the system state machine lifecycle.
 *
 * Each callback may return:
 *   - DEV_OK: continue normal operation.
 *   - Any error code: signal a failure. svc_sm will store the error
 *     and may transition to ERROR state.
 */

/**
 * @brief Called during POST_INIT, after modules are initialized.
 *
 * Use for application-level initialization that depends on services
 * being ready (e.g., restoring state from EEPROM).
 *
 * @return DEV_OK on success, error code on failure.
 */
dev_err_t app_init(void);

/**
 * @brief Called during POST_INIT, after app_init().
 *
 * Use for application-level startup actions.
 *
 * @return DEV_OK on success, error code on failure.
 */
dev_err_t app_start(void);

/**
 * @brief Called repeatedly from the superloop during RUN state.
 *
 * Use for application-level periodic processing.
 * Must return quickly — do not block or loop indefinitely.
 *
 * @return DEV_OK on success, error code on failure.
 */
dev_err_t app_run(void);

/**
 * @brief Called during shutdown, before modules are stopped.
 *
 * Use for application-level pre-shutdown actions
 * (e.g., saving state, signaling external devices).
 *
 * @return DEV_OK on success, error code on failure.
 */
dev_err_t app_stop(void);

/**
 * @brief Called at the start of the shutdown sequence.
 *
 * Use for application-level shutdown actions
 * (e.g., closing files, saving persistent data).
 *
 * @return DEV_OK on success, error code on failure.
 */
dev_err_t app_shutdown(void);

/**
 * @brief Called when the system enters ERROR state.
 *
 * Use for application-level error handling
 * (e.g., logging, safe-state activation, notification).
 *
 * @return DEV_OK on success, error code on failure.
 */
dev_err_t app_error(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_LIFECYCLE_H */
