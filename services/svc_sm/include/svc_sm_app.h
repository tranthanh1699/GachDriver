#ifndef SVC_SM_APP_H
#define SVC_SM_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_sm_types.h"

/**
 * @brief Application lifecycle interface.
 *
 * These functions are declared here and defined in app/src/app_lifecycle.c.
 * svc_sm calls them at defined points during the state machine lifecycle.
 *
 * Unused callbacks may simply return DEV_OK.
 */

dev_err_t app_init(void);
dev_err_t app_start(void);
dev_err_t app_run(void);
dev_err_t app_stop(void);
dev_err_t app_shutdown(void);
dev_err_t app_error(void);

#ifdef __cplusplus
}
#endif

#endif /* SVC_SM_APP_H */
