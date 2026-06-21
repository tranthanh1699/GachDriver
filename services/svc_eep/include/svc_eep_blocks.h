#ifndef SVC_EEP_BLOCKS_H
#define SVC_EEP_BLOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_eep_types.h"

/* ── Block ID enum ──
 *
 * Each entry represents a configured EEPROM data block.
 * Block IDs start at 0 and are sequential.
 * SVC_EEP_BLOCK_COUNT must always be the last member.
 *
 * To add a block:
 *   1. Add a new entry before SVC_EEP_BLOCK_COUNT.
 *   2. Add its mirror buffer and config entry in svc_eep_blocks.c.
 */

typedef enum
{
    SVC_EEP_BLOCK_SYSTEM_CFG = 0,
    SVC_EEP_BLOCK_USER_DATA,
    SVC_EEP_BLOCK_DEVICE_INFO,

    SVC_EEP_BLOCK_COUNT
} svc_eep_block_id_t;

/* ── Compile-time block count validation ── */

#if (SVC_EEP_BLOCK_COUNT > SVC_EEP_CFG_MAX_BLOCKS)
#error "SVC_EEP_BLOCK_COUNT exceeds SVC_EEP_CFG_MAX_BLOCKS"
#endif

/* ── Extern mirror declarations ── */

extern uint8_t s_system_cfg_mirror[];
extern uint8_t s_user_data_mirror[];
extern uint8_t s_device_info_mirror[];

/* ── Block config table (defined in svc_eep_blocks.c) ── */

extern const svc_eep_block_cfg_t s_svc_eep_block_cfg[SVC_EEP_BLOCK_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_BLOCKS_H */
