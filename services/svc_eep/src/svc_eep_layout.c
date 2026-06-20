#include "svc_eep_layout.h"

const svc_eep_field_t g_svc_eep_fields[] =
{
    {
        SVC_EEP_FIELD_MAGIC,
        SVC_EEP_LAYOUT_MAGIC_OFFSET,
        SVC_EEP_LAYOUT_MAGIC_SIZE,
        "magic"
    },
    {
        SVC_EEP_FIELD_VERSION,
        SVC_EEP_LAYOUT_VERSION_OFFSET,
        SVC_EEP_LAYOUT_VERSION_SIZE,
        "version"
    },
    {
        SVC_EEP_FIELD_CRC,
        SVC_EEP_LAYOUT_CRC_OFFSET,
        SVC_EEP_LAYOUT_CRC_SIZE,
        "crc"
    },
    {
        SVC_EEP_FIELD_BOOT_COUNT,
        SVC_EEP_LAYOUT_BOOT_COUNT_OFFSET,
        SVC_EEP_LAYOUT_BOOT_COUNT_SIZE,
        "boot_count"
    },
    {
        SVC_EEP_FIELD_DEVICE_NAME,
        SVC_EEP_LAYOUT_DEVICE_NAME_OFFSET,
        SVC_EEP_LAYOUT_DEVICE_NAME_SIZE,
        "device_name"
    },
};

const uint16_t g_svc_eep_field_count =
    (uint16_t)(sizeof(g_svc_eep_fields) / sizeof(g_svc_eep_fields[0]));
