#ifndef SVC_EEP_TYPES_H
#define SVC_EEP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

/* ── Block configuration descriptor (static, read-only) ── */

typedef struct
{
    uint8_t        block_id;
    uint32_t       eep_offset;
    uint16_t       block_size;
    uint8_t       *mirror;
} svc_eep_block_cfg_t;

/* ── Block runtime state (mutable) ── */

typedef struct
{
    bool loaded;
    bool dirty;
    bool valid;
} svc_eep_block_state_t;

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_TYPES_H */
