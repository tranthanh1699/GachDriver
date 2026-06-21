#ifndef SVC_EEP_CFG_H
#define SVC_EEP_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_compiler.h"
#include "dev_eep_cfg.h"

/* ── Feature toggles ── */

#define SVC_EEP_CFG_RUNTIME_CHECK_ENABLED              DEV_ON
#define SVC_EEP_CFG_AUTO_SYNC_ON_SHUTDOWN              DEV_ON

/* ── Block limits ── */

#define SVC_EEP_CFG_MAX_BLOCKS                         (16U)

/* ── Mirror allocation mode ── */

#define SVC_EEP_CFG_DYNAMIC_MIRROR_ENABLED             DEV_OFF

/* ── Backward-compatible EEPROM dimension aliases ── */

#define SVC_EEP_MAIN_TOTAL_SIZE    DEV_EEP_MAIN_TOTAL_SIZE
#define SVC_EEP_MAIN_PAGE_SIZE     DEV_EEP_MAIN_PAGE_SIZE
#define SVC_EEP_MAIN_PAGE_COUNT    DEV_EEP_MAIN_PAGE_COUNT

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_CFG_H */
