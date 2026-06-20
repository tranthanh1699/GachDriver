#ifndef SVC_EEP_LAYOUT_H
#define SVC_EEP_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_eep_types.h"

/* ── Field IDs ── */
#define SVC_EEP_FIELD_MAGIC                 ((svc_eep_field_id_t)0U)
#define SVC_EEP_FIELD_VERSION               ((svc_eep_field_id_t)1U)
#define SVC_EEP_FIELD_CRC                   ((svc_eep_field_id_t)2U)
#define SVC_EEP_FIELD_BOOT_COUNT            ((svc_eep_field_id_t)3U)
#define SVC_EEP_FIELD_DEVICE_NAME           ((svc_eep_field_id_t)4U)

/* ── Layout base address ── */
#define SVC_EEP_LAYOUT_BASE                 ((svc_eep_addr_t)0x0000U)

/*
 * Field offsets and sizes.
 * Each offset = previous offset + previous size — self-verifying chain.
 * To add a field, append after the last entry:
 *   #define SVC_EEP_LAYOUT_MY_FIELD_OFFSET  (SVC_EEP_LAYOUT_<PREV>_OFFSET + SVC_EEP_LAYOUT_<PREV>_SIZE)
 *   #define SVC_EEP_LAYOUT_MY_FIELD_SIZE    ((svc_eep_size_t)NU)
 */
#define SVC_EEP_LAYOUT_MAGIC_OFFSET         (SVC_EEP_LAYOUT_BASE)
#define SVC_EEP_LAYOUT_MAGIC_SIZE           ((svc_eep_size_t)4U)   /* → 0x0004 */

#define SVC_EEP_LAYOUT_VERSION_OFFSET       (SVC_EEP_LAYOUT_MAGIC_OFFSET + SVC_EEP_LAYOUT_MAGIC_SIZE)
#define SVC_EEP_LAYOUT_VERSION_SIZE         ((svc_eep_size_t)2U)   /* → 0x0006 */

#define SVC_EEP_LAYOUT_CRC_OFFSET           (SVC_EEP_LAYOUT_VERSION_OFFSET + SVC_EEP_LAYOUT_VERSION_SIZE)
#define SVC_EEP_LAYOUT_CRC_SIZE             ((svc_eep_size_t)2U)   /* → 0x0008 */

#define SVC_EEP_LAYOUT_BOOT_COUNT_OFFSET    (SVC_EEP_LAYOUT_CRC_OFFSET + SVC_EEP_LAYOUT_CRC_SIZE)
#define SVC_EEP_LAYOUT_BOOT_COUNT_SIZE      ((svc_eep_size_t)4U)   /* → 0x000C */

#define SVC_EEP_LAYOUT_DEVICE_NAME_OFFSET   (SVC_EEP_LAYOUT_BOOT_COUNT_OFFSET + SVC_EEP_LAYOUT_BOOT_COUNT_SIZE)
#define SVC_EEP_LAYOUT_DEVICE_NAME_SIZE     ((svc_eep_size_t)32U)  /* → 0x002C */

/* Total used bytes — next available offset for user fields */
#define SVC_EEP_LAYOUT_USED_SIZE            (SVC_EEP_LAYOUT_DEVICE_NAME_OFFSET + SVC_EEP_LAYOUT_DEVICE_NAME_SIZE)
/* = 0x0000 + 4 + 2 + 2 + 4 + 32 = 0x002C (44 bytes) */

/* ── Data integrity constants ── */
#define SVC_EEP_MAGIC_VALUE                 (0x44564550UL)
#define SVC_EEP_LAYOUT_VERSION              ((uint16_t)1U)

/* ── CRC region (all data from BASE up to, but not including, the CRC field) ── */
#define SVC_EEP_CRC_START_OFFSET            (SVC_EEP_LAYOUT_BASE)
#define SVC_EEP_CRC_DATA_LENGTH             (SVC_EEP_LAYOUT_CRC_OFFSET)

/* ── Field table (defined in svc_eep_layout.c) ── */
extern const svc_eep_field_t  g_svc_eep_fields[];
extern const uint16_t         g_svc_eep_field_count;

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_LAYOUT_H */
