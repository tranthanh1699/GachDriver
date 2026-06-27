#ifndef SVC_EEP_INTERNAL_H
#define SVC_EEP_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_eep_types.h"

/* ── Internal block configuration (with mirror) ──
 *
 * Embeds svc_eep_block_info_t as a named sub-object so that
 * &cfg->info is a valid, aliasing-compliant pointer to the
 * public metadata type.  Do NOT duplicate fields manually.
 */

typedef struct
{
    svc_eep_block_info_t info;
    uint8_t              *mirror;
} svc_eep_block_cfg_t;

/* ── Block runtime state (mutable) ── */

typedef struct
{
    bool loaded;
    bool dirty;
} svc_eep_block_state_t;

/**
 * @brief Get the internal block configuration (includes mirror pointer).
 *
 * INTERNAL — for use only by svc_eep.c and svc_eep_blocks.c.
 * External code must use svc_eep_get_block_info() which returns
 * a mirror-free descriptor.
 *
 * @param block_id Block identifier (must be < SVC_EEP_BLOCK_COUNT).
 * @return Pointer to internal block config, or NULL if block_id is out of range.
 */
const svc_eep_block_cfg_t *svc_eep_get_block_cfg(svc_eep_block_id_t block_id);

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_INTERNAL_H */
