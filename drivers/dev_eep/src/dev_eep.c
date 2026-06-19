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

/* ── Stub implementations (filled in later tasks) ── */

static dev_err_t dev_eep_i2c_read(const dev_eep_device_t *dev,
                                  dev_eep_addr_t addr,
                                  uint8_t *data,
                                  dev_eep_size_t length)
{
    /* Will be implemented in Task 9 */
    (void)dev; (void)addr; (void)data; (void)length;
    return DEV_ERR_NOT_SUPPORTED;
}

static dev_err_t dev_eep_i2c_write_page(const dev_eep_device_t *dev,
                                        dev_eep_addr_t addr,
                                        const uint8_t *data,
                                        dev_eep_size_t length)
{
    /* Will be implemented in Task 9 */
    (void)dev; (void)addr; (void)data; (void)length;
    return DEV_ERR_NOT_SUPPORTED;
}

static dev_err_t dev_eep_wait_write_cycle(const dev_eep_device_t *dev)
{
    /* Will be implemented in Task 9 */
    (void)dev;
    return DEV_ERR_NOT_SUPPORTED;
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
