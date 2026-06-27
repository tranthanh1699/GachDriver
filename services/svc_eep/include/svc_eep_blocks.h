#ifndef SVC_EEP_BLOCKS_H
#define SVC_EEP_BLOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_eep_types.h"
#include "svc_eep_cfg.h"

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

#ifdef __cplusplus
static_assert((SVC_EEP_BLOCK_COUNT <= SVC_EEP_CFG_MAX_BLOCKS),
              "SVC_EEP_BLOCK_COUNT exceeds SVC_EEP_CFG_MAX_BLOCKS");
#else
_Static_assert((SVC_EEP_BLOCK_COUNT <= SVC_EEP_CFG_MAX_BLOCKS),
               "SVC_EEP_BLOCK_COUNT exceeds SVC_EEP_CFG_MAX_BLOCKS");
#endif

/* ── Block config accessors (implemented in svc_eep_blocks.c) ──
 *
 * Mirror buffers and the config table are private to svc_eep_blocks.c.
 * External code MUST access block configuration only through these
 * accessors and the svc_eep_* service APIs.
 */

/**
 * @brief Get a const pointer to a block's public metadata.
 *
 * Returns a mirror-free descriptor suitable for external consumption.
 * The returned pointer is valid for the lifetime of the application.
 *
 * @param block_id Block identifier.
 * @return Pointer to block info, or NULL if block_id is out of range.
 */
const svc_eep_block_info_t *svc_eep_get_block_info(svc_eep_block_id_t block_id);

/**
 * @brief Validate a block ID against the configured range.
 *
 * Handles negative and wrapped values correctly regardless of the
 * compiler's choice of signedness for the enum type.
 *
 * @param block_id Block identifier to validate.
 * @return true if the ID is valid (0 .. SVC_EEP_BLOCK_COUNT-1).
 */
bool svc_eep_block_id_is_valid(svc_eep_block_id_t block_id);

/**
 * @brief Get the number of configured blocks.
 *
 * @return SVC_EEP_BLOCK_COUNT.
 */
uint8_t svc_eep_get_block_count(void);

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_BLOCKS_H */
