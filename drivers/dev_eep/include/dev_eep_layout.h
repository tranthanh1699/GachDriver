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

/* ── Field offsets and sizes ── */
#define DEV_EEP_LAYOUT_MAGIC_OFFSET         ((dev_eep_addr_t)0x0000U)
#define DEV_EEP_LAYOUT_MAGIC_SIZE           ((dev_eep_size_t)4U)

#define DEV_EEP_LAYOUT_VERSION_OFFSET       ((dev_eep_addr_t)0x0004U)
#define DEV_EEP_LAYOUT_VERSION_SIZE         ((dev_eep_size_t)2U)

#define DEV_EEP_LAYOUT_CRC_OFFSET           ((dev_eep_addr_t)0x0006U)
#define DEV_EEP_LAYOUT_CRC_SIZE             ((dev_eep_size_t)2U)

#define DEV_EEP_LAYOUT_BOOT_COUNT_OFFSET    ((dev_eep_addr_t)0x0008U)
#define DEV_EEP_LAYOUT_BOOT_COUNT_SIZE      ((dev_eep_size_t)4U)

#define DEV_EEP_LAYOUT_DEVICE_NAME_OFFSET   ((dev_eep_addr_t)0x000CU)
#define DEV_EEP_LAYOUT_DEVICE_NAME_SIZE     ((dev_eep_size_t)32U)

/* ── Data integrity constants ── */
#define DEV_EEP_MAGIC_VALUE                 (0x44564550UL)
#define DEV_EEP_LAYOUT_VERSION              ((uint16_t)1U)

/* ── CRC region (all data except CRC field itself) ── */
#define DEV_EEP_CRC_START_OFFSET            ((dev_eep_addr_t)0x0000U)
#define DEV_EEP_CRC_DATA_LENGTH             (DEV_EEP_LAYOUT_CRC_OFFSET)

/* ── Field table (defined in dev_eep_layout.c) ── */
extern const dev_eep_field_t  g_dev_eep_fields[];
extern const uint16_t         g_dev_eep_field_count;

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_LAYOUT_H */
