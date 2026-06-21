#include "svc_eep.h"
#include "dev_eep.h"
#include "dev_crc.h"
#include "dev_assert.h"
#include "dev_common.h"
#include <string.h>

/* ── Static RAM buffers ── */

static uint8_t s_eep_main_mirror[DEV_EEP_MAIN_TOTAL_SIZE];
static uint8_t s_eep_main_dirty_map[SVC_EEP_MAIN_DIRTY_MAP_SIZE];

/* ── Device table ── */

static const svc_eep_device_t s_devices[SVC_EEP_CFG_MAX_DEVICES] =
{
    {
        SVC_EEP_MAIN,
        s_eep_main_mirror,
        DEV_EEP_MAIN_TOTAL_SIZE,
        s_eep_main_dirty_map,
        SVC_EEP_MAIN_DIRTY_MAP_SIZE
    },
};

/* ── Internal state ── */

static bool s_initialized = false;

/* ── Internal helpers: forward declarations ── */

static const svc_eep_device_t *svc_eep_find_device(svc_eep_id_t eep_id);
static const svc_eep_field_t  *svc_eep_find_field(svc_eep_field_id_t field_id);
static dev_err_t svc_eep_validate_addr(const svc_eep_device_t *dev,
                                       svc_eep_addr_t addr,
                                       svc_eep_size_t length);
static dev_err_t svc_eep_validate_layout(void);
static bool svc_eep_is_page_dirty(const svc_eep_device_t *dev,
                                  svc_eep_size_t page_index);
static void svc_eep_set_page_dirty(const svc_eep_device_t *dev,
                                   svc_eep_size_t page_index);
static void svc_eep_clear_page_dirty(const svc_eep_device_t *dev,
                                     svc_eep_size_t page_index);
static svc_eep_size_t svc_eep_addr_to_page(svc_eep_addr_t addr);
static dev_err_t svc_eep_read_all(svc_eep_id_t eep_id);
static dev_err_t svc_eep_update_crc(const svc_eep_device_t *dev);
static dev_err_t svc_eep_check_crc(const svc_eep_device_t *dev);
static void svc_eep_load_defaults(const svc_eep_device_t *dev);

/* ── Internal helper implementations ── */

static const svc_eep_device_t *svc_eep_find_device(svc_eep_id_t eep_id)
{
    uint8_t i;

    for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
    {
        if (s_devices[i].eep_id == eep_id)
        {
            return &s_devices[i];
        }
    }
    return NULL;
}

static const svc_eep_field_t *svc_eep_find_field(svc_eep_field_id_t field_id)
{
    uint16_t i;

    for (i = 0U; i < g_svc_eep_field_count; i++)
    {
        if (g_svc_eep_fields[i].field_id == field_id)
        {
            return &g_svc_eep_fields[i];
        }
    }
    return NULL;
}

static dev_err_t svc_eep_validate_addr(const svc_eep_device_t *dev,
                                       svc_eep_addr_t addr,
                                       svc_eep_size_t length)
{
    if (dev == NULL)
    {
        return DEV_ERR_NULL_PTR;
    }
    if (length == 0U)
    {
        return DEV_ERR_INVALID_ARG;
    }
    if (addr >= dev->mirror_size)
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    if ((addr + length) > dev->mirror_size)
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    if ((addr + length) < addr) /* overflow check */
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    return DEV_OK;
}

static dev_err_t svc_eep_validate_layout(void)
{
    uint16_t i;
    uint16_t j;
    const svc_eep_device_t *dev;
    svc_eep_addr_t field_end;
    svc_eep_addr_t other_end;

    dev = svc_eep_find_device(SVC_EEP_MAIN);
    if (dev == NULL)
    {
        return DEV_ERR_CONFIG;
    }

    for (i = 0U; i < g_svc_eep_field_count; i++)
    {
        const svc_eep_field_t *field = &g_svc_eep_fields[i];

        field_end = field->offset + field->size;
        if ((field_end < field->offset) || (field_end > dev->mirror_size))
        {
            return DEV_ERR_CONFIG;
        }

        for (j = (uint16_t)(i + 1U); j < g_svc_eep_field_count; j++)
        {
            const svc_eep_field_t *other = &g_svc_eep_fields[j];
            other_end = other->offset + other->size;

            if (!((field_end <= other->offset) || (other_end <= field->offset)))
            {
                return DEV_ERR_CONFIG;
            }
        }
    }

    return DEV_OK;
}

static bool svc_eep_is_page_dirty(const svc_eep_device_t *dev,
                                  svc_eep_size_t page_index)
{
    uint8_t byte_index;
    uint8_t bit_mask;

    if ((dev == NULL) || (dev->dirty_map == NULL) ||
        (page_index >= DEV_EEP_MAIN_PAGE_COUNT))
    {
        return false;
    }

    byte_index = (uint8_t)(page_index / 8U);
    bit_mask   = (uint8_t)(1U << (page_index % 8U));

    return ((dev->dirty_map[byte_index] & bit_mask) != 0U);
}

static void svc_eep_set_page_dirty(const svc_eep_device_t *dev,
                                   svc_eep_size_t page_index)
{
    uint8_t byte_index;
    uint8_t bit_mask;

    if ((dev == NULL) || (dev->dirty_map == NULL) ||
        (page_index >= DEV_EEP_MAIN_PAGE_COUNT))
    {
        return;
    }

    byte_index = (uint8_t)(page_index / 8U);
    bit_mask   = (uint8_t)(1U << (page_index % 8U));

    dev->dirty_map[byte_index] |= bit_mask;
}

static void svc_eep_clear_page_dirty(const svc_eep_device_t *dev,
                                     svc_eep_size_t page_index)
{
    uint8_t byte_index;
    uint8_t bit_mask;

    if ((dev == NULL) || (dev->dirty_map == NULL) ||
        (page_index >= DEV_EEP_MAIN_PAGE_COUNT))
    {
        return;
    }

    byte_index = (uint8_t)(page_index / 8U);
    bit_mask   = (uint8_t)(~((uint8_t)(1U << (page_index % 8U))));

    dev->dirty_map[byte_index] &= bit_mask;
}

static svc_eep_size_t svc_eep_addr_to_page(svc_eep_addr_t addr)
{
    if (DEV_EEP_MAIN_PAGE_SIZE == 0U)
    {
        return 0U;
    }
    return (addr / DEV_EEP_MAIN_PAGE_SIZE);
}

static dev_err_t svc_eep_update_crc(const svc_eep_device_t *dev)
{
    uint16_t crc_value;
    dev_err_t result;

    DEV_CHECK_RET((dev != NULL), DEV_ERR_NULL_PTR);

    result = dev_crc16_compute(&dev->mirror[SVC_EEP_CRC_START_OFFSET],
                               (size_t)SVC_EEP_CRC_DATA_LENGTH,
                               &crc_value);
    DEV_CHECK_OK_RET(result);

    (void)memcpy(&dev->mirror[SVC_EEP_LAYOUT_CRC_OFFSET],
                 &crc_value,
                 SVC_EEP_LAYOUT_CRC_SIZE);

    return DEV_OK;
}

static dev_err_t svc_eep_check_crc(const svc_eep_device_t *dev)
{
    uint16_t computed_crc;
    uint16_t stored_crc;
    dev_err_t result;

    DEV_CHECK_RET((dev != NULL), DEV_ERR_NULL_PTR);

    result = dev_crc16_compute(&dev->mirror[SVC_EEP_CRC_START_OFFSET],
                               (size_t)SVC_EEP_CRC_DATA_LENGTH,
                               &computed_crc);
    DEV_CHECK_OK_RET(result);

    (void)memcpy(&stored_crc,
                 &dev->mirror[SVC_EEP_LAYOUT_CRC_OFFSET],
                 SVC_EEP_LAYOUT_CRC_SIZE);

    if (computed_crc != stored_crc)
    {
        return DEV_ERR_CRC;
    }

    return DEV_OK;
}

static void svc_eep_load_defaults(const svc_eep_device_t *dev)
{
    uint32_t magic;
    uint16_t version;
    uint32_t boot_count;
    uint8_t  name_buf[SVC_EEP_LAYOUT_DEVICE_NAME_SIZE];

    if (dev == NULL)
    {
        return;
    }

    magic = SVC_EEP_MAGIC_VALUE;
    (void)memcpy(&dev->mirror[SVC_EEP_LAYOUT_MAGIC_OFFSET],
                 &magic, SVC_EEP_LAYOUT_MAGIC_SIZE);

    version = SVC_EEP_LAYOUT_VERSION;
    (void)memcpy(&dev->mirror[SVC_EEP_LAYOUT_VERSION_OFFSET],
                 &version, SVC_EEP_LAYOUT_VERSION_SIZE);

    boot_count = 0U;
    (void)memcpy(&dev->mirror[SVC_EEP_LAYOUT_BOOT_COUNT_OFFSET],
                 &boot_count, SVC_EEP_LAYOUT_BOOT_COUNT_SIZE);

    (void)memset(name_buf, 0, SVC_EEP_LAYOUT_DEVICE_NAME_SIZE);
    (void)memcpy(&dev->mirror[SVC_EEP_LAYOUT_DEVICE_NAME_OFFSET],
                 name_buf, SVC_EEP_LAYOUT_DEVICE_NAME_SIZE);

    /* Mark all pages dirty so defaults get written on next flush */
    {
        svc_eep_size_t page;
        for (page = 0U; page < DEV_EEP_MAIN_PAGE_COUNT; page++)
        {
            svc_eep_set_page_dirty(dev, page);
        }
    }
}

/* ── Internal: read/write all via dev_eep ── */

static dev_err_t svc_eep_read_all(svc_eep_id_t eep_id)
{
    const svc_eep_device_t *dev;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = svc_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

#if (SVC_EEP_CFG_MIRROR_ENABLED == DEV_ON)
    {
        dev_err_t result;

        /* Read entire EEPROM into RAM mirror via dev_eep */
        result = dev_eep_read((dev_eep_id_t)eep_id, 0U,
                              dev->mirror,
                              (uint32_t)dev->mirror_size);
        if (result != DEV_OK)
        {
            return result;
        }

        (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);
    }
#endif

    return DEV_OK;
}

/* ── Lifecycle ── */

dev_err_t svc_eep_init(void)
{
    dev_err_t result;
    uint8_t i;

    if (s_initialized)
    {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    /* Validate device configuration */
    for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
    {
        const svc_eep_device_t *dev = &s_devices[i];

        if (dev->mirror == NULL)
        {
            return DEV_ERR_CONFIG;
        }
        if (dev->mirror_size != DEV_EEP_MAIN_TOTAL_SIZE)
        {
            return DEV_ERR_CONFIG;
        }
        if (dev->dirty_map == NULL)
        {
            return DEV_ERR_CONFIG;
        }
    }

    /* Validate layout (field overlap, bounds) */
    result = svc_eep_validate_layout();
    if (result != DEV_OK)
    {
        return result;
    }

    /* Initialize the underlying EEPROM device driver */
    for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
    {
        result = dev_eep_init((dev_eep_id_t)s_devices[i].eep_id);
        if (result != DEV_OK)
        {
            /* Clean up any dev_eep devices already initialized */
            if (i > 0U)
            {
                uint8_t j;
                for (j = 0U; j < i; j++)
                {
                    (void)dev_eep_deinit((dev_eep_id_t)s_devices[j].eep_id);
                }
            }
            return result;
        }
    }
    /* Clear dirty maps and mirrors */
    for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
    {
        const svc_eep_device_t *dev = &s_devices[i];
        (void)memset(dev->mirror, 0, (size_t)dev->mirror_size);
        (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);
    }

#if (SVC_EEP_CFG_AUTO_READ_ALL_ON_INIT == DEV_ON)
    s_initialized = true;
    {
        for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
        {
            result = svc_eep_read_all(s_devices[i].eep_id);
            if (result != DEV_OK)
            {
                s_initialized = false;
                /* Clean up dev_eep devices before returning */
                {
                    uint8_t j;
                    for (j = 0U; j < SVC_EEP_CFG_MAX_DEVICES; j++)
                    {
                        (void)dev_eep_deinit((dev_eep_id_t)s_devices[j].eep_id);
                    }
                }
                return result;
            }
        }
    }
#endif

    /* Validate CRC / magic / version, load defaults if needed */
#if (SVC_EEP_CFG_CRC_ENABLED == DEV_ON)
    {
        for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
        {
            const svc_eep_device_t *dev = &s_devices[i];
            uint32_t mirror_magic;

            (void)memcpy(&mirror_magic,
                         &dev->mirror[SVC_EEP_LAYOUT_MAGIC_OFFSET],
                         SVC_EEP_LAYOUT_MAGIC_SIZE);

#if (SVC_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC == DEV_ON)
            if (mirror_magic != SVC_EEP_MAGIC_VALUE)
            {
                svc_eep_load_defaults(dev);
            }
            else
            {
                result = svc_eep_check_crc(dev);
                if (result != DEV_OK)
                {
                    svc_eep_load_defaults(dev);
                }
            }
#else
            if (mirror_magic != SVC_EEP_MAGIC_VALUE)
            {
                s_initialized = false;
                /* Clean up dev_eep devices before returning */
                {
                    uint8_t j;
                    for (j = 0U; j < SVC_EEP_CFG_MAX_DEVICES; j++)
                    {
                        (void)dev_eep_deinit((dev_eep_id_t)s_devices[j].eep_id);
                    }
                }
                return DEV_ERR_CRC;
            }

            result = svc_eep_check_crc(dev);
            if (result != DEV_OK)
            {
                s_initialized = false;
                /* Clean up dev_eep devices before returning */
                {
                    uint8_t j;
                    for (j = 0U; j < SVC_EEP_CFG_MAX_DEVICES; j++)
                    {
                        (void)dev_eep_deinit((dev_eep_id_t)s_devices[j].eep_id);
                    }
                }
                return DEV_ERR_CRC;
            }
#endif
        }
    }
#endif

    s_initialized = true;
    return DEV_OK;
}

dev_err_t svc_eep_shutdown(void)
{
    uint8_t i;
    dev_err_t result;
    dev_err_t first_error = DEV_OK;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

#if (SVC_EEP_CFG_AUTO_FLUSH_ON_SHUTDOWN == DEV_ON)
    {
        for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
        {
#if (SVC_EEP_CFG_CRC_ENABLED == DEV_ON)
            result = svc_eep_update_crc(&s_devices[i]);
            if (result != DEV_OK)
            {
                if (first_error == DEV_OK)
                {
                    first_error = result;
                }
            }
            else
            {
#endif
                result = svc_eep_flush();
                if (result != DEV_OK)
                {
                    if (first_error == DEV_OK)
                    {
                        first_error = result;
                    }
                }
#if (SVC_EEP_CFG_CRC_ENABLED == DEV_ON)
            }
#endif
        }

        if (first_error != DEV_OK)
        {
            return first_error;
        }
    }
#endif

    /* Deinitialize dev_eep devices */
    for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
    {
        (void)dev_eep_deinit((dev_eep_id_t)s_devices[i].eep_id);
    }

    s_initialized = false;
    return DEV_OK;
}

dev_err_t svc_eep_deinit(void)
{
    uint8_t i;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Clear mirrors and dirty maps */
    for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
    {
        const svc_eep_device_t *dev = &s_devices[i];
        (void)memset(dev->mirror, 0, (size_t)dev->mirror_size);
        (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);
    }

    /* Deinitialize dev_eep devices without flushing */
    for (i = 0U; i < SVC_EEP_CFG_MAX_DEVICES; i++)
    {
        (void)dev_eep_deinit((dev_eep_id_t)s_devices[i].eep_id);
    }

    s_initialized = false;
    return DEV_OK;
}

bool svc_eep_is_initialized(void)
{
    return s_initialized;
}

/* ── Raw read/write (RAM mirror) ── */

dev_err_t svc_eep_read(svc_eep_id_t eep_id,
                       svc_eep_addr_t addr,
                       uint8_t *data,
                       svc_eep_size_t length)
{
    const svc_eep_device_t *dev;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = svc_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    DEV_CHECK_PTR_RET(data);

    result = svc_eep_validate_addr(dev, addr, length);
    DEV_CHECK_OK_RET(result);

    (void)memcpy(data, &dev->mirror[addr], (size_t)length);

    return DEV_OK;
}

dev_err_t svc_eep_write(svc_eep_id_t eep_id,
                        svc_eep_addr_t addr,
                        const uint8_t *data,
                        svc_eep_size_t length)
{
    const svc_eep_device_t *dev;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = svc_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    DEV_CHECK_PTR_RET(data);

    result = svc_eep_validate_addr(dev, addr, length);
    DEV_CHECK_OK_RET(result);

#if (SVC_EEP_CFG_WRITE_ONLY_IF_CHANGED == DEV_ON)
    {
        if (memcmp(&dev->mirror[addr], data, (size_t)length) == 0)
        {
            return DEV_OK;
        }
    }
#endif

    (void)memcpy(&dev->mirror[addr], data, (size_t)length);

    (void)svc_eep_mark_dirty(eep_id, addr, length);

    return DEV_OK;
}

/* ── Flush ── */

dev_err_t svc_eep_flush(void)
{
    const svc_eep_device_t *dev;
    svc_eep_size_t page_index;
    svc_eep_addr_t page_addr;
    svc_eep_size_t page_size;
    svc_eep_size_t chunk_size;
    dev_err_t result;
    bool any_dirty = false;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Use the main device */
    dev = svc_eep_find_device(SVC_EEP_MAIN);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    page_size = DEV_EEP_MAIN_PAGE_SIZE;

    for (page_index = 0U; page_index < DEV_EEP_MAIN_PAGE_COUNT; page_index++)
    {
        if (svc_eep_is_page_dirty(dev, page_index))
        {
            any_dirty = true;
            page_addr = page_index * page_size;

            /* Determine chunk size for this page */
            chunk_size = page_size;
            if ((page_addr + chunk_size) > dev->mirror_size)
            {
                chunk_size = dev->mirror_size - page_addr;
            }

            /* Write the page to EEPROM via dev_eep */
            result = dev_eep_write((dev_eep_id_t)dev->eep_id,
                                   page_addr,
                                   &dev->mirror[page_addr],
                                   (uint32_t)chunk_size);
            if (result != DEV_OK)
            {
                return result;
            }

            svc_eep_clear_page_dirty(dev, page_index);
        }
    }

    if (!any_dirty)
    {
        return DEV_OK;
    }

    return DEV_OK;
}

/* ── Field-based read/write ── */

dev_err_t svc_eep_read_field(svc_eep_field_id_t field_id,
                             void *data,
                             svc_eep_size_t length)
{
    const svc_eep_field_t *field;
    const svc_eep_device_t *dev;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    DEV_CHECK_PTR_RET(data);

    field = svc_eep_find_field(field_id);
    DEV_CHECK_RET((field != NULL), DEV_ERR_INVALID_ARG);

    if (length > field->size)
    {
        return DEV_ERR_INVALID_ARG;
    }

    dev = svc_eep_find_device(SVC_EEP_MAIN);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_CONFIG);

    (void)memcpy(data, &dev->mirror[field->offset], (size_t)length);

    return DEV_OK;
}

dev_err_t svc_eep_write_field(svc_eep_field_id_t field_id,
                              const void *data,
                              svc_eep_size_t length)
{
    const svc_eep_field_t *field;
    const svc_eep_device_t *dev;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    DEV_CHECK_PTR_RET(data);

    field = svc_eep_find_field(field_id);
    DEV_CHECK_RET((field != NULL), DEV_ERR_INVALID_ARG);

    if (length > field->size)
    {
        return DEV_ERR_INVALID_ARG;
    }

    dev = svc_eep_find_device(SVC_EEP_MAIN);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_CONFIG);

    return svc_eep_write(SVC_EEP_MAIN, field->offset,
                         (const uint8_t *)data, length);
}

dev_err_t svc_eep_get_field_info(svc_eep_field_id_t field_id,
                                 const svc_eep_field_t **field)
{
    DEV_CHECK_PTR_RET(field);

    *field = svc_eep_find_field(field_id);
    DEV_CHECK_RET(((*field) != NULL), DEV_ERR_INVALID_ARG);

    return DEV_OK;
}

/* ── Typed read/write ── */

dev_err_t svc_eep_read_u8(svc_eep_field_id_t field_id, uint8_t *value)
{
    DEV_CHECK_PTR_RET(value);
    return svc_eep_read_field(field_id, (void *)value, (svc_eep_size_t)sizeof(uint8_t));
}

dev_err_t svc_eep_write_u8(svc_eep_field_id_t field_id, uint8_t value)
{
    return svc_eep_write_field(field_id, (const void *)&value, (svc_eep_size_t)sizeof(uint8_t));
}

dev_err_t svc_eep_read_u16(svc_eep_field_id_t field_id, uint16_t *value)
{
    DEV_CHECK_PTR_RET(value);
    return svc_eep_read_field(field_id, (void *)value, (svc_eep_size_t)sizeof(uint16_t));
}

dev_err_t svc_eep_write_u16(svc_eep_field_id_t field_id, uint16_t value)
{
    return svc_eep_write_field(field_id, (const void *)&value, (svc_eep_size_t)sizeof(uint16_t));
}

dev_err_t svc_eep_read_u32(svc_eep_field_id_t field_id, uint32_t *value)
{
    DEV_CHECK_PTR_RET(value);
    return svc_eep_read_field(field_id, (void *)value, (svc_eep_size_t)sizeof(uint32_t));
}

dev_err_t svc_eep_write_u32(svc_eep_field_id_t field_id, uint32_t value)
{
    return svc_eep_write_field(field_id, (const void *)&value, (svc_eep_size_t)sizeof(uint32_t));
}

/* ── Dirty state ── */

bool svc_eep_is_dirty(svc_eep_id_t eep_id)
{
    const svc_eep_device_t *dev;
    svc_eep_size_t page_index;

    dev = svc_eep_find_device(eep_id);
    if (dev == NULL)
    {
        return false;
    }

    for (page_index = 0U; page_index < DEV_EEP_MAIN_PAGE_COUNT; page_index++)
    {
        if (svc_eep_is_page_dirty(dev, page_index))
        {
            return true;
        }
    }

    return false;
}

dev_err_t svc_eep_mark_dirty(svc_eep_id_t eep_id,
                             svc_eep_addr_t addr,
                             svc_eep_size_t length)
{
    const svc_eep_device_t *dev;
    svc_eep_size_t start_page;
    svc_eep_size_t end_page;
    svc_eep_size_t page;
    dev_err_t result;

    dev = svc_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    result = svc_eep_validate_addr(dev, addr, length);
    DEV_CHECK_OK_RET(result);

#if (SVC_EEP_CFG_DIRTY_TRACKING_ENABLED == DEV_ON)
    {
        start_page = svc_eep_addr_to_page(addr);
        end_page   = svc_eep_addr_to_page(addr + length - 1U);

        for (page = start_page; page <= end_page; page++)
        {
            svc_eep_set_page_dirty(dev, page);
        }
    }
#endif

    return DEV_OK;
}

dev_err_t svc_eep_clear_dirty(svc_eep_id_t eep_id)
{
    const svc_eep_device_t *dev;

    dev = svc_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);

    return DEV_OK;
}

uint16_t svc_eep_get_dirty_page_count(svc_eep_id_t eep_id)
{
    const svc_eep_device_t *dev;
    svc_eep_size_t page_index;
    uint16_t count = 0U;

    dev = svc_eep_find_device(eep_id);
    if (dev == NULL)
    {
        return 0U;
    }

    for (page_index = 0U; page_index < DEV_EEP_MAIN_PAGE_COUNT; page_index++)
    {
        if (svc_eep_is_page_dirty(dev, page_index))
        {
            count++;
        }
    }

    return count;
}
