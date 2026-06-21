#include "svc_eep_blocks.h"

/* ── Block sizes ── */

#define SVC_EEP_BLOCK_SYSTEM_CFG_SIZE     (32U)
#define SVC_EEP_BLOCK_USER_DATA_SIZE      (64U)
#define SVC_EEP_BLOCK_DEVICE_INFO_SIZE    (16U)

/* ── EEPROM offsets ── */
#define SVC_EEP_BLOCK_BASE                (0x0000U)
#define SVC_EEP_BLOCK_SYSTEM_CFG_OFFSET   (0x0000U)
#define SVC_EEP_BLOCK_USER_DATA_OFFSET    (0x0020U)
#define SVC_EEP_BLOCK_DEVICE_INFO_OFFSET  (0x0060U)

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
