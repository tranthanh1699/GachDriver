#ifndef DEV_EEP_LAYOUT_H
#define DEV_EEP_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_eep_types.h"

/* ── Field IDs ── */
#define DEV_EEP_FIELD_MAGIC                 ((dev_eep_field_id_t)0U)
#define DEV_EEP_FIELD_VERSION               ((dev_eep_field_id_t)1U)
#define DEV_EEP_FIELD_CRC                   ((dev_eep_field_id_t)2U)
#define DEV_EEP_FIELD_BOOT_COUNT            ((dev_eep_field_id_t)3U)
#define DEV_EEP_FIELD_DEVICE_NAME           ((dev_eep_field_id_t)4U)

/* ── Layout base address ── */
#define DEV_EEP_LAYOUT_BASE                 ((dev_eep_addr_t)0x0000U)

/*
 * Field offsets and sizes.
 * Each offset = previous offset + previous size — self-verifying chain.
 * To add a field, append after the last entry:
 *   #define DEV_EEP_LAYOUT_MY_FIELD_OFFSET  (DEV_EEP_LAYOUT_<PREV>_OFFSET + DEV_EEP_LAYOUT_<PREV>_SIZE)
 *   #define DEV_EEP_LAYOUT_MY_FIELD_SIZE    ((dev_eep_size_t)NU)
 */
#define DEV_EEP_LAYOUT_MAGIC_OFFSET         (DEV_EEP_LAYOUT_BASE)
#define DEV_EEP_LAYOUT_MAGIC_SIZE           ((dev_eep_size_t)4U)   /* → 0x0004 */

#define DEV_EEP_LAYOUT_VERSION_OFFSET       (DEV_EEP_LAYOUT_MAGIC_OFFSET + DEV_EEP_LAYOUT_MAGIC_SIZE)
#define DEV_EEP_LAYOUT_VERSION_SIZE         ((dev_eep_size_t)2U)   /* → 0x0006 */

#define DEV_EEP_LAYOUT_CRC_OFFSET           (DEV_EEP_LAYOUT_VERSION_OFFSET + DEV_EEP_LAYOUT_VERSION_SIZE)
#define DEV_EEP_LAYOUT_CRC_SIZE             ((dev_eep_size_t)2U)   /* → 0x0008 */

#define DEV_EEP_LAYOUT_BOOT_COUNT_OFFSET    (DEV_EEP_LAYOUT_CRC_OFFSET + DEV_EEP_LAYOUT_CRC_SIZE)
#define DEV_EEP_LAYOUT_BOOT_COUNT_SIZE      ((dev_eep_size_t)4U)   /* → 0x000C */

#define DEV_EEP_LAYOUT_DEVICE_NAME_OFFSET   (DEV_EEP_LAYOUT_BOOT_COUNT_OFFSET + DEV_EEP_LAYOUT_BOOT_COUNT_SIZE)
#define DEV_EEP_LAYOUT_DEVICE_NAME_SIZE     ((dev_eep_size_t)32U)  /* → 0x002C */

/* Total used bytes — next available offset for user fields */
#define DEV_EEP_LAYOUT_USED_SIZE            (DEV_EEP_LAYOUT_DEVICE_NAME_OFFSET + DEV_EEP_LAYOUT_DEVICE_NAME_SIZE)
/* = 0x0000 + 4 + 2 + 2 + 4 + 32 = 0x002C (44 bytes) */

/* ── Data integrity constants ── */
#define DEV_EEP_MAGIC_VALUE                 (0x44564550UL)
#define DEV_EEP_LAYOUT_VERSION              ((uint16_t)1U)

/* ── CRC region (all data from BASE up to, but not including, the CRC field) ── */
#define DEV_EEP_CRC_START_OFFSET            (DEV_EEP_LAYOUT_BASE)
#define DEV_EEP_CRC_DATA_LENGTH             (DEV_EEP_LAYOUT_CRC_OFFSET)

/* ── Field table (defined in dev_eep_layout.c) ── */
extern const dev_eep_field_t  g_dev_eep_fields[];
extern const uint16_t         g_dev_eep_field_count;

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_LAYOUT_H */
