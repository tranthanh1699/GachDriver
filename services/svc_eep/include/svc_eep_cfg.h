#ifndef SVC_EEP_CFG_H
#define SVC_EEP_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_eep_types.h"
#include "dev_compiler.h"
#include "dev_i2c_cfg.h"

/* ── Feature toggles ── */
#define SVC_EEP_CFG_MAX_DEVICES                    (1U)
#define SVC_EEP_CFG_RUNTIME_CHECK_ENABLED          DEV_ON
#define SVC_EEP_CFG_MIRROR_ENABLED                 DEV_ON
#define SVC_EEP_CFG_DIRTY_TRACKING_ENABLED         DEV_ON
#define SVC_EEP_CFG_CRC_ENABLED                    DEV_ON
#define SVC_EEP_CFG_AUTO_READ_ALL_ON_INIT          DEV_ON
#define SVC_EEP_CFG_AUTO_FLUSH_ON_SHUTDOWN         DEV_ON
#define SVC_EEP_CFG_WRITE_ONLY_IF_CHANGED          DEV_ON
#define SVC_EEP_CFG_PAGE_WRITE_ENABLED             DEV_ON
#define SVC_EEP_CFG_ACK_POLLING_ENABLED            DEV_ON
#define SVC_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC   DEV_ON

/* ── Timing ── */
#define SVC_EEP_CFG_DEFAULT_WRITE_CYCLE_TIME_MS    (5U)
#define SVC_EEP_CFG_ACK_POLL_TIMEOUT_MS            (10U)
#define SVC_EEP_CFG_ACK_POLL_INTERVAL_US           (100U)

/* ── Device IDs ── */
#define SVC_EEP_MAIN                               ((svc_eep_id_t)0U)

/* ── EEPROM dimensions ── */
#define SVC_EEP_MAIN_TOTAL_SIZE                    (256u)
#define SVC_EEP_MAIN_PAGE_SIZE                     (8U)   /* AT24C02: 8-byte page write buffer */
#define SVC_EEP_MAIN_PAGE_COUNT                    (SVC_EEP_MAIN_TOTAL_SIZE / SVC_EEP_MAIN_PAGE_SIZE)
#define SVC_EEP_MAIN_DIRTY_MAP_SIZE                ((SVC_EEP_MAIN_PAGE_COUNT + 7U) / 8U)

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_CFG_H */
