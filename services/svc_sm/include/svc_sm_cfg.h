#ifndef SVC_SM_CFG_H
#define SVC_SM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ── Module limits ── */

#define SVC_SM_CFG_MAX_MODULES                  (16U)

/* ── Runtime checks ── */

#define SVC_SM_CFG_RUNTIME_CHECK_ENABLED        (1U)

/* ── Feature toggles ── */

#define SVC_SM_CFG_ERROR_STATE_ENABLED          (1U)
#define SVC_SM_CFG_SLEEP_ENABLED                (0U)
#define SVC_SM_CFG_SHUTDOWN_ENABLED             (1U)
#define SVC_SM_CFG_SAFE_SHUTDOWN_ENABLED        (1U)

/* ── Runtime behavior ── */

#define SVC_SM_CFG_CALL_MODULE_HANDLE_IN_RUN    (1U)

/* ── Logging ── */

#define SVC_SM_CFG_USE_DEV_LOG                  (1U)

#ifdef __cplusplus
}
#endif

#endif /* SVC_SM_CFG_H */
