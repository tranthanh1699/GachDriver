#ifndef SVC_EEP_TYPES_H
#define SVC_EEP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

/* ── Public block metadata (no mirror pointer) ──
 *
 * Returned by svc_eep_get_block_info(). Safe for external consumption;
 * does not expose the mutable mirror pointer.
 */

typedef struct
{
    uint8_t        block_id;
    uint32_t       eep_offset;
    uint16_t       block_size;
} svc_eep_block_info_t;

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_TYPES_H */
