#include "svc_eep_blocks.h"
#include "svc_eep_internal.h"

/* ── Block sizes (edit only these when adding/changing blocks) ── */

#define SVC_EEP_BLOCK_SYSTEM_CFG_SIZE     (32U)
#define SVC_EEP_BLOCK_USER_DATA_SIZE      (64U)
#define SVC_EEP_BLOCK_DEVICE_INFO_SIZE    (16U)

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

/* ── Static RAM mirror buffers (private — no external access) ── */

static uint8_t s_system_cfg_mirror[SVC_EEP_BLOCK_SYSTEM_CFG_SIZE];
static uint8_t s_user_data_mirror[SVC_EEP_BLOCK_USER_DATA_SIZE];
static uint8_t s_device_info_mirror[SVC_EEP_BLOCK_DEVICE_INFO_SIZE];

/* ── Block config table (private — access via svc_eep_get_block_cfg) ── */

static const svc_eep_block_cfg_t s_svc_eep_block_cfg[SVC_EEP_BLOCK_COUNT] =
{
    [SVC_EEP_BLOCK_SYSTEM_CFG] =
    {
        .info =
        {
            .block_id   = (uint8_t)SVC_EEP_BLOCK_SYSTEM_CFG,
            .eep_offset = SVC_EEP_BLOCK_SYSTEM_CFG_OFFSET,
            .block_size = SVC_EEP_BLOCK_SYSTEM_CFG_SIZE,
        },
        .mirror = s_system_cfg_mirror
    },
    [SVC_EEP_BLOCK_USER_DATA] =
    {
        .info =
        {
            .block_id   = (uint8_t)SVC_EEP_BLOCK_USER_DATA,
            .eep_offset = SVC_EEP_BLOCK_USER_DATA_OFFSET,
            .block_size = SVC_EEP_BLOCK_USER_DATA_SIZE,
        },
        .mirror = s_user_data_mirror
    },
    [SVC_EEP_BLOCK_DEVICE_INFO] =
    {
        .info =
        {
            .block_id   = (uint8_t)SVC_EEP_BLOCK_DEVICE_INFO,
            .eep_offset = SVC_EEP_BLOCK_DEVICE_INFO_OFFSET,
            .block_size = SVC_EEP_BLOCK_DEVICE_INFO_SIZE,
        },
        .mirror = s_device_info_mirror
    },
};

/* ── Public accessors ── */

/**
 * @brief Validate a block ID against the configured range.
 *
 * Handles negative and wrapped values correctly regardless of the
 * compiler's choice of signedness for the enum type.
 */
bool svc_eep_block_id_is_valid(svc_eep_block_id_t block_id)
{
    /* Cast to int32_t first to catch negative values, then to uint32_t
     * for the upper-bound comparison. This handles:
     *  - negative IDs (compiler may represent enum as signed int)
     *  - wrapped IDs (e.g., (svc_eep_block_id_t)256 on an 8-bit base)
     *  - way-out-of-range IDs (e.g., (svc_eep_block_id_t)65535) */
    return ((int32_t)block_id >= 0) &&
           ((uint32_t)block_id < (uint32_t)SVC_EEP_BLOCK_COUNT);
}

/**
 * @brief Get a const pointer to a block's public metadata (no mirror).
 *
 * Returns &cfg->info, which is a valid pointer to a real
 * svc_eep_block_info_t object — no strict-aliasing violation.
 */
const svc_eep_block_info_t *svc_eep_get_block_info(svc_eep_block_id_t block_id)
{
    if (!svc_eep_block_id_is_valid(block_id))
    {
        return NULL;
    }
    return &s_svc_eep_block_cfg[(uint8_t)block_id].info;
}

/**
 * @brief Get the internal block configuration (includes mirror pointer).
 *
 * INTERNAL — declared in svc_eep_internal.h. External code must use
 * svc_eep_get_block_info() which returns a mirror-free descriptor.
 */
const svc_eep_block_cfg_t *svc_eep_get_block_cfg(svc_eep_block_id_t block_id)
{
    if (!svc_eep_block_id_is_valid(block_id))
    {
        return NULL;
    }
    return &s_svc_eep_block_cfg[(uint8_t)block_id];
}

/**
 * @brief Get the number of configured blocks.
 */
uint8_t svc_eep_get_block_count(void)
{
    return (uint8_t)SVC_EEP_BLOCK_COUNT;
}
