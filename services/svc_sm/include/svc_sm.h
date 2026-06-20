#ifndef SVC_SM_H
#define SVC_SM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_sm_types.h"
#include "svc_sm_cfg.h"

/* ── Lifecycle ── */

/**
 * @brief Initialize the service state manager.
 *
 * Must be called once before any other svc_sm function.
 * Transitions from UNINIT to STARTUP internally.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_ALREADY_INITIALIZED if called more than once.
 */
dev_err_t svc_sm_init(void);

/**
 * @brief Run the startup sequence.
 *
 * Executes STARTUP -> INIT -> POST_INIT -> RUN:
 *   1. Module init callbacks (forward order)
 *   2. Module start callbacks (forward order)
 *   3. app_init()
 *   4. app_start()
 *   5. Transition to RUN
 *
 * If a critical module fails, transition to ERROR.
 *
 * @return DEV_OK on success, error code on failure.
 * @return DEV_ERR_NOT_INITIALIZED if svc_sm_init() has not been called.
 */
dev_err_t svc_sm_startup(void);

/**
 * @brief Handle one iteration of the state machine.
 *
 * Called repeatedly from the superloop. In RUN state:
 *   - Calls module handle callbacks (if enabled)
 *   - Calls app_run() (if enabled)
 *   - Processes pending requests (shutdown, error)
 *   - In ERROR state, calls app_error()
 *
 * This function must return quickly. No infinite loops or long
 * blocking delays.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_sm_handle(void);

/**
 * @brief Perform a safe shutdown sequence.
 *
 * Transitions through PREPARE_SHUTDOWN to SHUTDOWN:
 *   1. app_shutdown()
 *   2. Module stop callbacks (reverse order)
 *   3. Module shutdown callbacks (reverse order)
 *   4. Module deinit callbacks (reverse order)
 *
 * @return DEV_OK on success.
 */
dev_err_t svc_sm_shutdown(void);

/**
 * @brief Deinitialize the state manager.
 *
 * @return DEV_OK on success.
 */
dev_err_t svc_sm_deinit(void);

/* ── State queries ── */

/**
 * @brief Get the current state.
 *
 * @return Current svc_sm_state_t value.
 */
svc_sm_state_t svc_sm_get_state(void);

/**
 * @brief Get the previous state.
 *
 * @return Previous svc_sm_state_t value.
 */
svc_sm_state_t svc_sm_get_previous_state(void);

/**
 * @brief Check if the state manager is initialized.
 *
 * @return true if svc_sm_init() has been called.
 */
bool svc_sm_is_initialized(void);

/**
 * @brief Check if the system is in RUN state.
 *
 * @return true if current state is SVC_SM_STATE_RUN.
 */
bool svc_sm_is_running(void);

/* ── Requests ── */

/**
 * @brief Request a system shutdown.
 *
 * The shutdown will be processed during the next svc_sm_handle() call.
 *
 * @return DEV_OK on success, DEV_ERR_INVALID_STATE if shutdown is
 *         not allowed from the current state.
 */
dev_err_t svc_sm_request_shutdown(void);

/**
 * @brief Request an error state transition.
 *
 * Stores the error reason and transitions to ERROR.
 *
 * @param reason The error code describing the failure.
 * @return DEV_OK on success.
 */
dev_err_t svc_sm_request_error(dev_err_t reason);

/* ── Error info ── */

/**
 * @brief Retrieve the last stored error information.
 *
 * @param info Pointer to svc_sm_error_info_t to fill.
 * @return DEV_OK on success.
 * @return DEV_ERR_NULL_PTR if info is NULL.
 */
dev_err_t svc_sm_get_last_error(svc_sm_error_info_t *info);

#if (SVC_SM_CFG_SLEEP_ENABLED == 1U)

/**
 * @brief Request sleep mode.
 *
 * @return DEV_OK on success.
 */
dev_err_t svc_sm_request_sleep(void);

/**
 * @brief Request wakeup from sleep.
 *
 * @return DEV_OK on success.
 */
dev_err_t svc_sm_request_wakeup(void);

#endif /* SVC_SM_CFG_SLEEP_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* SVC_SM_H */
