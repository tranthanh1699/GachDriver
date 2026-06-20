#ifndef SVC_SM_TYPES_H
#define SVC_SM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_error.h"

/* ── State ── */

typedef enum
{
    SVC_SM_STATE_UNINIT = 0,
    SVC_SM_STATE_STARTUP,
    SVC_SM_STATE_INIT,
    SVC_SM_STATE_POST_INIT,
    SVC_SM_STATE_RUN,
    SVC_SM_STATE_PREPARE_SHUTDOWN,
    SVC_SM_STATE_SHUTDOWN,
    SVC_SM_STATE_ERROR,

#if (SVC_SM_CFG_SLEEP_ENABLED == 1U)
    SVC_SM_STATE_PREPARE_SLEEP,
    SVC_SM_STATE_SLEEP,
    SVC_SM_STATE_WAKEUP,
#endif
} svc_sm_state_t;

/* ── Request ── */

typedef enum
{
    SVC_SM_REQUEST_NONE = 0,
    SVC_SM_REQUEST_SHUTDOWN,
    SVC_SM_REQUEST_ERROR,

#if (SVC_SM_CFG_SLEEP_ENABLED == 1U)
    SVC_SM_REQUEST_SLEEP,
    SVC_SM_REQUEST_WAKEUP,
#endif
} svc_sm_request_t;

/* ── Error info ── */

typedef struct
{
    dev_err_t       error;
    svc_sm_state_t  state;
    const char     *module_name;
} svc_sm_error_info_t;

/* ── Module descriptor ── */

typedef dev_err_t (*svc_sm_module_fn_t)(void);

typedef struct
{
    const char         *name;
    svc_sm_module_fn_t  init;
    svc_sm_module_fn_t  start;
    svc_sm_module_fn_t  handle;
    svc_sm_module_fn_t  stop;
    svc_sm_module_fn_t  shutdown;
    svc_sm_module_fn_t  deinit;
    bool                critical;
} svc_sm_module_t;

#ifdef __cplusplus
}
#endif

#endif /* SVC_SM_TYPES_H */
