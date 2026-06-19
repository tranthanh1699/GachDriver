#include "dev_eep_layout.h"

const dev_eep_field_t g_dev_eep_fields[] =
{
    {
        DEV_EEP_FIELD_MAGIC,
        DEV_EEP_LAYOUT_MAGIC_OFFSET,
        DEV_EEP_LAYOUT_MAGIC_SIZE,
        "magic"
    },
    {
        DEV_EEP_FIELD_VERSION,
        DEV_EEP_LAYOUT_VERSION_OFFSET,
        DEV_EEP_LAYOUT_VERSION_SIZE,
        "version"
    },
    {
        DEV_EEP_FIELD_CRC,
        DEV_EEP_LAYOUT_CRC_OFFSET,
        DEV_EEP_LAYOUT_CRC_SIZE,
        "crc"
    },
    {
        DEV_EEP_FIELD_BOOT_COUNT,
        DEV_EEP_LAYOUT_BOOT_COUNT_OFFSET,
        DEV_EEP_LAYOUT_BOOT_COUNT_SIZE,
        "boot_count"
    },
    {
        DEV_EEP_FIELD_DEVICE_NAME,
        DEV_EEP_LAYOUT_DEVICE_NAME_OFFSET,
        DEV_EEP_LAYOUT_DEVICE_NAME_SIZE,
        "device_name"
    },
};

const uint16_t g_dev_eep_field_count =
    (uint16_t)(sizeof(g_dev_eep_fields) / sizeof(g_dev_eep_fields[0]));
