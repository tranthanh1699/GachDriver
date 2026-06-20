#ifndef SVC_SM_MODULES_H
#define SVC_SM_MODULES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_sm_types.h"

/**
 * @brief Static module table.
 *
 * Defined in svc_sm_modules.c. Each entry describes a service
 * module that participates in the system lifecycle.
 *
 * Order rules:
 *   - init     : forward
 *   - start    : forward
 *   - handle   : forward
 *   - stop     : reverse
 *   - shutdown : reverse
 *   - deinit   : reverse
 */
extern const svc_sm_module_t g_svc_sm_modules[];

/**
 * @brief Number of entries in g_svc_sm_modules[].
 */
extern const uint16_t g_svc_sm_module_count;

#ifdef __cplusplus
}
#endif

#endif /* SVC_SM_MODULES_H */
