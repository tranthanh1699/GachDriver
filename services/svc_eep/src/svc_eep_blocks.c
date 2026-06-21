#include "svc_eep_blocks.h"

/* ── Block sizes (edit only these when adding/changing blocks) ── */

#define SVC_EEP_BLOCK_SYSTEM_CFG_SIZE     (1U)
#define SVC_EEP_BLOCK_USER_DATA_SIZE      (1U)
#define SVC_EEP_BLOCK_DEVICE_INFO_SIZE    (1U)

/*
 * ── EEPROM offsets (auto-computed — do not edit manually) ──
 *
 * Each offset = previous offset + previous size.
 * Only change SVC_EEP_BLOCK_BASE and the _SIZE macros above.
 * The chain guarantees no gaps and no overlaps.
 */
#define SVC_EEP_BLOCK_BASE                (0x0000U)
#define SVC_EEP_BLOCK_SYSTEM_CFG_OFFSET   (SVC_EEP_BLOCK_BASE)
#define SVC_EEP_BLOCK_USER_DATA_OFFSET    (SVC_EEP_BLOCK_SYSTEM_CFG_OFFSET + SVC_EEP_BLOCK_SYSTEM_CFG_SIZE)
#define SVC_EEP_BLOCK_DEVICE_INFO_OFFSET  (SVC_EEP_BLOCK_USER_DATA_OFFSET  + SVC_EEP_BLOCK_USER_DATA_SIZE)

/* Total used EEPROM size (auto-computed) */
#define SVC_EEP_BLOCK_TOTAL_USED          (SVC_EEP_BLOCK_DEVICE_INFO_OFFSET + SVC_EEP_BLOCK_DEVICE_INFO_SIZE)

/* ── Static RAM mirror buffers ── */

uint8_t s_system_cfg_mirror[SVC_EEP_BLOCK_SYSTEM_CFG_SIZE];
uint8_t s_user_data_mirror[SVC_EEP_BLOCK_USER_DATA_SIZE];
uint8_t s_device_info_mirror[SVC_EEP_BLOCK_DEVICE_INFO_SIZE];

/* ── Block config table (indexed by block_id) ── */

const svc_eep_block_cfg_t s_svc_eep_block_cfg[SVC_EEP_BLOCK_COUNT] =
{
    [SVC_EEP_BLOCK_SYSTEM_CFG] =
    {
        .block_id   = (uint8_t)SVC_EEP_BLOCK_SYSTEM_CFG,
        .eep_offset = SVC_EEP_BLOCK_SYSTEM_CFG_OFFSET,
        .block_size = SVC_EEP_BLOCK_SYSTEM_CFG_SIZE,
        .mirror     = s_system_cfg_mirror
    },
    [SVC_EEP_BLOCK_USER_DATA] =
    {
        .block_id   = (uint8_t)SVC_EEP_BLOCK_USER_DATA,
        .eep_offset = SVC_EEP_BLOCK_USER_DATA_OFFSET,
        .block_size = SVC_EEP_BLOCK_USER_DATA_SIZE,
        .mirror     = s_user_data_mirror
    },
    [SVC_EEP_BLOCK_DEVICE_INFO] =
    {
        .block_id   = (uint8_t)SVC_EEP_BLOCK_DEVICE_INFO,
        .eep_offset = SVC_EEP_BLOCK_DEVICE_INFO_OFFSET,
        .block_size = SVC_EEP_BLOCK_DEVICE_INFO_SIZE,
        .mirror     = s_device_info_mirror
    },
};
