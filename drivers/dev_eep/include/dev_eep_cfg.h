#ifndef DEV_EEP_CFG_H
#define DEV_EEP_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_eep_types.h"
#include "dev_compiler.h"

/* ── Feature toggles ── */

#define DEV_EEP_CFG_RUNTIME_CHECK_ENABLED       DEV_ON
#define DEV_EEP_CFG_ACK_POLLING_ENABLED         DEV_ON
#define DEV_EEP_CFG_PAGE_WRITE_ENABLED          DEV_ON

/* ── Timing ── */

#define DEV_EEP_CFG_DEFAULT_WRITE_CYCLE_TIME_MS (100U)
#define DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS         (10U)
#define DEV_EEP_CFG_ACK_POLL_INTERVAL_US        (100U)

/* ── Device IDs ── */

enum
{
    DEV_EEP_MAIN = 0,
    DEV_EEP_CFG_MAX_DEVICES
};

/* ── EEPROM dimensions (AT24C02: 256 bytes, 8-byte page) ── */

#define DEV_EEP_MAIN_TOTAL_SIZE                 (256U)
#define DEV_EEP_MAIN_PAGE_SIZE                  (8U)
#define DEV_EEP_MAIN_PAGE_COUNT                 (DEV_EEP_MAIN_TOTAL_SIZE / DEV_EEP_MAIN_PAGE_SIZE)

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_CFG_H */
