#ifndef SVC_EEP_CFG_H
#define SVC_EEP_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_eep_types.h"
#include "dev_compiler.h"
#include "dev_eep_cfg.h"

/* ── Feature toggles (service-level only) ── */

#define SVC_EEP_CFG_RUNTIME_CHECK_ENABLED          DEV_ON
#define SVC_EEP_CFG_MIRROR_ENABLED                 DEV_ON
#define SVC_EEP_CFG_DIRTY_TRACKING_ENABLED         DEV_ON
#define SVC_EEP_CFG_CRC_ENABLED                    DEV_ON
#define SVC_EEP_CFG_AUTO_READ_ALL_ON_INIT          DEV_ON
#define SVC_EEP_CFG_AUTO_FLUSH_ON_SHUTDOWN         DEV_ON
#define SVC_EEP_CFG_WRITE_ONLY_IF_CHANGED          DEV_ON
#define SVC_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC   DEV_ON

/* ── Device IDs (alias dev_eep IDs for convenience) ── */

enum
{
    SVC_EEP_MAIN = DEV_EEP_MAIN,
    SVC_EEP_CFG_MAX_DEVICES
};

/* ── Dirty map size (computed from dev_eep page count) ── */

#define SVC_EEP_MAIN_DIRTY_MAP_SIZE \
    ((DEV_EEP_MAIN_PAGE_COUNT + 7U) / 8U)

/* ── Backward-compatible aliases (delegated to dev_eep) ── */

#define SVC_EEP_MAIN_TOTAL_SIZE    DEV_EEP_MAIN_TOTAL_SIZE
#define SVC_EEP_MAIN_PAGE_SIZE     DEV_EEP_MAIN_PAGE_SIZE
#define SVC_EEP_MAIN_PAGE_COUNT    DEV_EEP_MAIN_PAGE_COUNT

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_CFG_H */
