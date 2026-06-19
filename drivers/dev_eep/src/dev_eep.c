#include "dev_eep.h"
#include "dev_i2c.h"
#include "dev_crc.h"
#include "dev_assert.h"
#include "dev_common.h"
#include <string.h>

/* ── Static RAM buffers ── */

static uint8_t s_eep_main_mirror[DEV_EEP_MAIN_TOTAL_SIZE];
static uint8_t s_eep_main_dirty_map[DEV_EEP_MAIN_DIRTY_MAP_SIZE];

/* ── Device table ── */

static const dev_eep_device_t s_devices[DEV_EEP_CFG_MAX_DEVICES] =
{
    {
        DEV_EEP_MAIN,
        DEV_I2C_BUS_EEPROM,
        ((dev_i2c_addr_t)0x50U),
        DEV_EEP_MAIN_TOTAL_SIZE,
        DEV_EEP_MAIN_PAGE_SIZE,
        DEV_EEP_MEM_ADDR_SIZE_16BIT,
        DEV_EEP_CFG_DEFAULT_WRITE_CYCLE_TIME_MS,
        s_eep_main_mirror,
        DEV_EEP_MAIN_TOTAL_SIZE,
        s_eep_main_dirty_map,
        DEV_EEP_MAIN_DIRTY_MAP_SIZE
    },
};

/* ── Internal state ── */

static bool s_initialized = false;

/* ── Internal helpers: forward declarations ── */

static const dev_eep_device_t *dev_eep_find_device(dev_eep_id_t eep_id);
static const dev_eep_field_t  *dev_eep_find_field(dev_eep_field_id_t field_id);
static dev_err_t dev_eep_validate_addr(const dev_eep_device_t *dev,
                                       dev_eep_addr_t addr,
                                       dev_eep_size_t length);
static dev_err_t dev_eep_validate_layout(void);
static bool dev_eep_is_page_dirty(const dev_eep_device_t *dev,
                                  dev_eep_size_t page_index);
static void dev_eep_set_page_dirty(const dev_eep_device_t *dev,
                                   dev_eep_size_t page_index);
static void dev_eep_clear_page_dirty(const dev_eep_device_t *dev,
                                     dev_eep_size_t page_index);
static dev_eep_size_t dev_eep_addr_to_page(const dev_eep_device_t *dev,
                                           dev_eep_addr_t addr);
static dev_err_t dev_eep_i2c_read(const dev_eep_device_t *dev,
                                  dev_eep_addr_t addr,
                                  uint8_t *data,
                                  dev_eep_size_t length);
static dev_err_t dev_eep_i2c_write_page(const dev_eep_device_t *dev,
                                        dev_eep_addr_t addr,
                                        const uint8_t *data,
                                        dev_eep_size_t length);
static dev_err_t dev_eep_wait_write_cycle(const dev_eep_device_t *dev);
static dev_err_t dev_eep_update_crc(const dev_eep_device_t *dev);
static dev_err_t dev_eep_check_crc(const dev_eep_device_t *dev);
static void dev_eep_load_defaults(const dev_eep_device_t *dev);

/* ── Internal helper implementations ── */

static const dev_eep_device_t *dev_eep_find_device(dev_eep_id_t eep_id)
{
    uint8_t i;

    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        if (s_devices[i].eep_id == eep_id)
        {
            return &s_devices[i];
        }
    }
    return NULL;
}

static const dev_eep_field_t *dev_eep_find_field(dev_eep_field_id_t field_id)
{
    uint16_t i;

    for (i = 0U; i < g_dev_eep_field_count; i++)
    {
        if (g_dev_eep_fields[i].field_id == field_id)
        {
            return &g_dev_eep_fields[i];
        }
    }
    return NULL;
}

static dev_err_t dev_eep_validate_addr(const dev_eep_device_t *dev,
                                       dev_eep_addr_t addr,
                                       dev_eep_size_t length)
{
    if (dev == NULL)
    {
        return DEV_ERR_NULL_PTR;
    }
    if (length == 0U)
    {
        return DEV_ERR_INVALID_ARG;
    }
    if (addr >= dev->total_size)
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    /* Check overflow: addr + length must not wrap or exceed total_size */
    if ((addr + length) > dev->total_size)
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    if ((addr + length) < addr) /* overflow check */
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    return DEV_OK;
}

static dev_err_t dev_eep_validate_layout(void)
{
    uint16_t i;
    uint16_t j;
    const dev_eep_device_t *dev;
    dev_eep_addr_t field_end;
    dev_eep_addr_t other_end;

    dev = dev_eep_find_device(DEV_EEP_MAIN);
    if (dev == NULL)
    {
        return DEV_ERR_CONFIG;
    }

    for (i = 0U; i < g_dev_eep_field_count; i++)
    {
        const dev_eep_field_t *field = &g_dev_eep_fields[i];

        /* Check field fits in device */
        field_end = field->offset + field->size;
        if ((field_end < field->offset) || (field_end > dev->total_size))
        {
            return DEV_ERR_CONFIG; /* field out of bounds or overflow */
        }

        /* Check for overlap with other fields */
        for (j = (uint16_t)(i + 1U); j < g_dev_eep_field_count; j++)
        {
            const dev_eep_field_t *other = &g_dev_eep_fields[j];
            other_end = other->offset + other->size;

            /* Two fields overlap if neither is entirely before the other */
            if (!((field_end <= other->offset) || (other_end <= field->offset)))
            {
                return DEV_ERR_CONFIG; /* overlap detected */
            }
        }
    }

    return DEV_OK;
}

static bool dev_eep_is_page_dirty(const dev_eep_device_t *dev,
                                  dev_eep_size_t page_index)
{
    uint8_t byte_index;
    uint8_t bit_mask;

    if ((dev == NULL) || (dev->dirty_map == NULL) ||
        (page_index >= (dev->total_size / dev->page_size)))
    {
        return false;
    }

    byte_index = (uint8_t)(page_index / 8U);
    bit_mask   = (uint8_t)(1U << (page_index % 8U));

    return ((dev->dirty_map[byte_index] & bit_mask) != 0U);
}

static void dev_eep_set_page_dirty(const dev_eep_device_t *dev,
                                   dev_eep_size_t page_index)
{
    uint8_t byte_index;
    uint8_t bit_mask;

    if ((dev == NULL) || (dev->dirty_map == NULL) ||
        (page_index >= (dev->total_size / dev->page_size)))
    {
        return;
    }

    byte_index = (uint8_t)(page_index / 8U);
    bit_mask   = (uint8_t)(1U << (page_index % 8U));

    dev->dirty_map[byte_index] |= bit_mask;
}

static void dev_eep_clear_page_dirty(const dev_eep_device_t *dev,
                                     dev_eep_size_t page_index)
{
    uint8_t byte_index;
    uint8_t bit_mask;

    if ((dev == NULL) || (dev->dirty_map == NULL) ||
        (page_index >= (dev->total_size / dev->page_size)))
    {
        return;
    }

    byte_index = (uint8_t)(page_index / 8U);
    bit_mask   = (uint8_t)(~((uint8_t)(1U << (page_index % 8U))));

    dev->dirty_map[byte_index] &= bit_mask;
}

static dev_eep_size_t dev_eep_addr_to_page(const dev_eep_device_t *dev,
                                           dev_eep_addr_t addr)
{
    if ((dev == NULL) || (dev->page_size == 0U))
    {
        return 0U;
    }
    return (addr / dev->page_size);
}

/* ── I2C communication helpers ── */

static dev_err_t dev_eep_i2c_read(const dev_eep_device_t *dev,
                                  dev_eep_addr_t addr,
                                  uint8_t *data,
                                  dev_eep_size_t length)
{
    dev_err_t result;

    DEV_CHECK_RET((dev != NULL),  DEV_ERR_NULL_PTR);
    DEV_CHECK_RET((data != NULL), DEV_ERR_NULL_PTR);

    /* For 8/16-bit memory address sizes, use mem_read */
    if ((dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT) ||
        (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_16BIT))
    {
        dev_i2c_mem_addr_size_t i2c_addr_size;

        i2c_addr_size = (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT)
                        ? DEV_I2C_MEM_ADDR_SIZE_8BIT
                        : DEV_I2C_MEM_ADDR_SIZE_16BIT;

        result = dev_i2c_mem_read(dev->i2c_bus,
                                  dev->i2c_addr,
                                  (uint16_t)addr,
                                  i2c_addr_size,
                                  data,
                                  (uint16_t)length,
                                  DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }
    else
    {
        /* 24-bit or 32-bit: construct address bytes manually */
        uint8_t addr_bytes[4U];
        uint8_t addr_len;

        if (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_24BIT)
        {
            addr_bytes[0U] = (uint8_t)((addr >> 16U) & 0xFFU);
            addr_bytes[1U] = (uint8_t)((addr >> 8U)  & 0xFFU);
            addr_bytes[2U] = (uint8_t)(addr & 0xFFU);
            addr_len = 3U;
        }
        else /* DEV_EEP_MEM_ADDR_SIZE_32BIT */
        {
            addr_bytes[0U] = (uint8_t)((addr >> 24U) & 0xFFU);
            addr_bytes[1U] = (uint8_t)((addr >> 16U) & 0xFFU);
            addr_bytes[2U] = (uint8_t)((addr >> 8U)  & 0xFFU);
            addr_bytes[3U] = (uint8_t)(addr & 0xFFU);
            addr_len = 4U;
        }

        /* write address, then read data */
        result = dev_i2c_write_read(dev->i2c_bus,
                                    dev->i2c_addr,
                                    addr_bytes, addr_len,
                                    data, (uint16_t)length,
                                    DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }

    return result;
}

static dev_err_t dev_eep_i2c_write_page(const dev_eep_device_t *dev,
                                        dev_eep_addr_t addr,
                                        const uint8_t *data,
                                        dev_eep_size_t length)
{
    dev_err_t result;

    DEV_CHECK_RET((dev != NULL),  DEV_ERR_NULL_PTR);
    DEV_CHECK_RET((data != NULL), DEV_ERR_NULL_PTR);
    DEV_CHECK_RET((length <= dev->page_size), DEV_ERR_OUT_OF_RANGE);

    /* For 8/16-bit memory address sizes, use mem_write */
    if ((dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT) ||
        (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_16BIT))
    {
        dev_i2c_mem_addr_size_t i2c_addr_size;

        i2c_addr_size = (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT)
                        ? DEV_I2C_MEM_ADDR_SIZE_8BIT
                        : DEV_I2C_MEM_ADDR_SIZE_16BIT;

        result = dev_i2c_mem_write(dev->i2c_bus,
                                   dev->i2c_addr,
                                   (uint16_t)addr,
                                   i2c_addr_size,
                                   data,
                                   (uint16_t)length,
                                   DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }
    else
    {
        /* 24-bit or 32-bit: buffer address + data into single write */
        uint8_t buf[128U]; /* max page size + 4 bytes address */
        uint8_t addr_len;

        if (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_24BIT)
        {
            buf[0U] = (uint8_t)((addr >> 16U) & 0xFFU);
            buf[1U] = (uint8_t)((addr >> 8U)  & 0xFFU);
            buf[2U] = (uint8_t)(addr & 0xFFU);
            addr_len = 3U;
        }
        else /* DEV_EEP_MEM_ADDR_SIZE_32BIT */
        {
            buf[0U] = (uint8_t)((addr >> 24U) & 0xFFU);
            buf[1U] = (uint8_t)((addr >> 16U) & 0xFFU);
            buf[2U] = (uint8_t)((addr >> 8U)  & 0xFFU);
            buf[3U] = (uint8_t)(addr & 0xFFU);
            addr_len = 4U;
        }

        /* Ensure address + data fits in the stack buffer */
        if ((size_t)((size_t)addr_len + (size_t)length) > sizeof(buf))
        {
            return DEV_ERR_OUT_OF_RANGE;
        }

        (void)memcpy(&buf[addr_len], data, (size_t)length);

        result = dev_i2c_write(dev->i2c_bus,
                               dev->i2c_addr,
                               buf,
                               (uint16_t)(addr_len + length),
                               DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }

    return result;
}

static dev_err_t dev_eep_wait_write_cycle(const dev_eep_device_t *dev)
{
    DEV_CHECK_RET((dev != NULL), DEV_ERR_NULL_PTR);

#if (DEV_EEP_CFG_ACK_POLLING_ENABLED == DEV_ON)
    {
        uint32_t elapsed_ms = 0U;
        dev_err_t probe_result;

        while (elapsed_ms < DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS)
        {
            probe_result = dev_i2c_probe(dev->i2c_bus,
                                         dev->i2c_addr,
                                         (dev_i2c_timeout_t)1U);
            if (probe_result == DEV_OK)
            {
                /* Device ACKed — write cycle complete */
                return DEV_OK;
            }

            /* Small delay between probes */
            dev_delay_ms(1U);
            elapsed_ms++;
        }

        return DEV_ERR_TIMEOUT;
    }
#else
    {
        /* Fallback: use write cycle time delay */
        dev_delay_ms(dev->write_cycle_time_ms);
        return DEV_OK;
    }
#endif
}

static dev_err_t dev_eep_update_crc(const dev_eep_device_t *dev)
{
    /* Will be implemented in Task 12 */
    (void)dev;
    return DEV_ERR_NOT_SUPPORTED;
}

static dev_err_t dev_eep_check_crc(const dev_eep_device_t *dev)
{
    /* Will be implemented in Task 12 */
    (void)dev;
    return DEV_ERR_NOT_SUPPORTED;
}

static void dev_eep_load_defaults(const dev_eep_device_t *dev)
{
    uint32_t magic;
    uint16_t version;
    uint32_t boot_count;
    uint8_t  name_buf[DEV_EEP_LAYOUT_DEVICE_NAME_SIZE];

    if (dev == NULL)
    {
        return;
    }

    /* Write magic value */
    magic = DEV_EEP_MAGIC_VALUE;
    (void)memcpy(&dev->mirror[DEV_EEP_LAYOUT_MAGIC_OFFSET],
                 &magic, DEV_EEP_LAYOUT_MAGIC_SIZE);

    /* Write version */
    version = DEV_EEP_LAYOUT_VERSION;
    (void)memcpy(&dev->mirror[DEV_EEP_LAYOUT_VERSION_OFFSET],
                 &version, DEV_EEP_LAYOUT_VERSION_SIZE);

    /* Zero boot count */
    boot_count = 0U;
    (void)memcpy(&dev->mirror[DEV_EEP_LAYOUT_BOOT_COUNT_OFFSET],
                 &boot_count, DEV_EEP_LAYOUT_BOOT_COUNT_SIZE);

    /* Clear device name */
    (void)memset(name_buf, 0, DEV_EEP_LAYOUT_DEVICE_NAME_SIZE);
    (void)memcpy(&dev->mirror[DEV_EEP_LAYOUT_DEVICE_NAME_OFFSET],
                 name_buf, DEV_EEP_LAYOUT_DEVICE_NAME_SIZE);

    /* Mark all pages dirty so defaults get written on next flush */
    {
        dev_eep_size_t page;
        dev_eep_size_t page_count = dev->total_size / dev->page_size;
        for (page = 0U; page < page_count; page++)
        {
            dev_eep_set_page_dirty(dev, page);
        }
    }
}

/* ── Lifecycle ── */

dev_err_t dev_eep_init(void)
{
    dev_err_t result;
    uint8_t i;

    if (s_initialized)
    {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    /* Validate device configuration */
    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        const dev_eep_device_t *dev = &s_devices[i];

        if (dev->mirror == NULL)
        {
            return DEV_ERR_CONFIG;
        }
        if (dev->mirror_size != dev->total_size)
        {
            return DEV_ERR_CONFIG;
        }
        if (dev->page_size == 0U)
        {
            return DEV_ERR_CONFIG;
        }
        if (dev->dirty_map == NULL)
        {
            return DEV_ERR_CONFIG;
        }
    }

    /* Validate layout (field overlap, bounds) */
    result = dev_eep_validate_layout();
    if (result != DEV_OK)
    {
        return result;
    }

    /* Clear dirty maps and mirrors */
    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        const dev_eep_device_t *dev = &s_devices[i];
        (void)memset(dev->mirror, 0, (size_t)dev->mirror_size);
        (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);
    }

    /* Read all EEPROM data into RAM mirror */
#if (DEV_EEP_CFG_AUTO_READ_ALL_ON_INIT == DEV_ON)
    {
        for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
        {
            result = dev_eep_read_all(s_devices[i].eep_id);
            if (result != DEV_OK)
            {
                return result;
            }
        }
    }
#endif

    /* Validate CRC / magic / version, load defaults if needed */
#if (DEV_EEP_CFG_CRC_ENABLED == DEV_ON)
    {
        for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
        {
            const dev_eep_device_t *dev = &s_devices[i];
            uint32_t mirror_magic;

            /* Check magic first */
            (void)memcpy(&mirror_magic,
                         &dev->mirror[DEV_EEP_LAYOUT_MAGIC_OFFSET],
                         DEV_EEP_LAYOUT_MAGIC_SIZE);

            if (mirror_magic != DEV_EEP_MAGIC_VALUE)
            {
#if (DEV_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC == DEV_ON)
                dev_eep_load_defaults(dev);
                continue;
#else
                return DEV_ERR_CRC;
#endif
            }

            /* Check CRC */
            result = dev_eep_check_crc(dev);
            if (result != DEV_OK)
            {
#if (DEV_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC == DEV_ON)
                dev_eep_load_defaults(dev);
                continue;
#else
                return DEV_ERR_CRC;
#endif
            }
        }
    }
#endif

    s_initialized = true;
    return DEV_OK;
}

dev_err_t dev_eep_shutdown(void)
{
    uint8_t i;
    dev_err_t result;
    dev_err_t first_error = DEV_OK;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

#if (DEV_EEP_CFG_AUTO_FLUSH_ON_SHUTDOWN == DEV_ON)
    {
        for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
        {
#if (DEV_EEP_CFG_CRC_ENABLED == DEV_ON)
            /* Update CRC before flushing */
            result = dev_eep_update_crc(&s_devices[i]);
            if (result != DEV_OK)
            {
                if (first_error == DEV_OK)
                {
                    first_error = result;
                }
                continue;
            }
#endif
            result = dev_eep_flush(s_devices[i].eep_id);
            if (result != DEV_OK)
            {
                if (first_error == DEV_OK)
                {
                    first_error = result;
                }
                /* Don't clear initialized state on partial failure */
                continue;
            }
        }

        if (first_error != DEV_OK)
        {
            return first_error;
        }
    }
#endif

    s_initialized = false;
    return DEV_OK;
}

dev_err_t dev_eep_deinit(void)
{
    uint8_t i;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Clear mirrors and dirty maps */
    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        const dev_eep_device_t *dev = &s_devices[i];
        (void)memset(dev->mirror, 0, (size_t)dev->mirror_size);
        (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);
    }

    s_initialized = false;
    return DEV_OK;
}

bool dev_eep_is_initialized(void)
{
    return s_initialized;
}

/* ── Raw read/write (RAM mirror) ── */

dev_err_t dev_eep_read(dev_eep_id_t eep_id,
                       dev_eep_addr_t addr,
                       uint8_t *data,
                       dev_eep_size_t length)
{
    const dev_eep_device_t *dev;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    DEV_CHECK_PTR_RET(data);

    result = dev_eep_validate_addr(dev, addr, length);
    DEV_CHECK_OK_RET(result);

    /* Copy from RAM mirror to output buffer */
    (void)memcpy(data, &dev->mirror[addr], (size_t)length);

    return DEV_OK;
}

dev_err_t dev_eep_write(dev_eep_id_t eep_id,
                        dev_eep_addr_t addr,
                        const uint8_t *data,
                        dev_eep_size_t length)
{
    const dev_eep_device_t *dev;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    DEV_CHECK_PTR_RET(data);

    result = dev_eep_validate_addr(dev, addr, length);
    DEV_CHECK_OK_RET(result);

#if (DEV_EEP_CFG_WRITE_ONLY_IF_CHANGED == DEV_ON)
    {
        /* Compare: if identical, skip */
        if (memcmp(&dev->mirror[addr], data, (size_t)length) == 0)
        {
            return DEV_OK; /* No change, no dirty marking */
        }
    }
#endif

    /* Copy to mirror */
    (void)memcpy(&dev->mirror[addr], data, (size_t)length);

    /* Mark dirty pages */
    (void)dev_eep_mark_dirty(eep_id, addr, length);

    return DEV_OK;
}

dev_err_t dev_eep_read_all(dev_eep_id_t eep_id)
{
    const dev_eep_device_t *dev;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

#if (DEV_EEP_CFG_MIRROR_ENABLED == DEV_ON)
    {
        dev_err_t result;

        result = dev_eep_i2c_read(dev, 0U,
                                  dev->mirror,
                                  dev->total_size);
        if (result != DEV_OK)
        {
            return result;
        }

        /* Clear dirty map — data is fresh from EEPROM */
        (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);
    }
#endif

    return DEV_OK;
}

dev_err_t dev_eep_write_all(dev_eep_id_t eep_id)
{
    const dev_eep_device_t *dev;
    dev_eep_size_t offset;
    dev_eep_size_t remaining;
    dev_eep_size_t chunk;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    offset    = 0U;
    remaining = dev->total_size;

    while (remaining > 0U)
    {
        /* Determine chunk size: up to page_size, and don't cross page boundary */
        chunk = dev->page_size - (offset % dev->page_size);
        if (chunk > remaining)
        {
            chunk = remaining;
        }

        result = dev_eep_i2c_write_page(dev, offset,
                                        &dev->mirror[offset], chunk);
        if (result != DEV_OK)
        {
            return result;
        }

        /* Wait for write cycle */
        result = dev_eep_wait_write_cycle(dev);
        if (result != DEV_OK)
        {
            return result;
        }

        offset    += chunk;
        remaining -= chunk;
    }

#if (DEV_EEP_CFG_CRC_ENABLED == DEV_ON)
    {
        result = dev_eep_update_crc(dev);
        if (result != DEV_OK)
        {
            return result;
        }

        /* Write CRC field to EEPROM */
        {
            dev_eep_size_t crc_chunk;
            dev_eep_addr_t crc_addr = DEV_EEP_LAYOUT_CRC_OFFSET;

            crc_chunk = dev->page_size - (crc_addr % dev->page_size);
            if (crc_chunk > DEV_EEP_LAYOUT_CRC_SIZE)
            {
                crc_chunk = DEV_EEP_LAYOUT_CRC_SIZE;
            }

            result = dev_eep_i2c_write_page(dev, crc_addr,
                                            &dev->mirror[crc_addr], crc_chunk);
            if (result != DEV_OK)
            {
                return result;
            }

            result = dev_eep_wait_write_cycle(dev);
            if (result != DEV_OK)
            {
                return result;
            }
        }
    }
#endif

    /* Clear dirty map — all data written */
    (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);

    return DEV_OK;
}

dev_err_t dev_eep_flush(dev_eep_id_t eep_id)
{
    const dev_eep_device_t *dev;
    dev_eep_size_t page_index;
    dev_eep_size_t page_count;
    dev_eep_addr_t page_addr;
    dev_eep_size_t chunk_size;
    dev_err_t result;
    bool any_dirty = false;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    page_count = dev->total_size / dev->page_size;

    for (page_index = 0U; page_index < page_count; page_index++)
    {
        if (!dev_eep_is_page_dirty(dev, page_index))
        {
            continue;
        }

        any_dirty = true;
        page_addr = page_index * dev->page_size;

        /* Determine chunk size for this page */
        chunk_size = dev->page_size;
        if ((page_addr + chunk_size) > dev->total_size)
        {
            chunk_size = dev->total_size - page_addr;
        }

        /* Write the page to EEPROM */
        result = dev_eep_i2c_write_page(dev, page_addr,
                                        &dev->mirror[page_addr], chunk_size);
        if (result != DEV_OK)
        {
            /* Dirty bit remains set on failure */
            return result;
        }

        /* Wait for write cycle */
        result = dev_eep_wait_write_cycle(dev);
        if (result != DEV_OK)
        {
            return result;
        }

        /* Clear dirty bit only after successful write */
        dev_eep_clear_page_dirty(dev, page_index);
    }

    if (!any_dirty)
    {
        /* Nothing to flush — success */
        return DEV_OK;
    }

    return DEV_OK;
}
