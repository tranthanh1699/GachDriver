#include "dev_eep.h"
#include "dev_i2c.h"
#include "dev_common.h"
#include "dev_assert.h"
#include <string.h>

/* Maximum internal buffer size for I2C write operations:
 * largest page size + maximum memory address bytes (4) */
#define DEV_EEP_MAX_BUF_SIZE (DEV_EEP_MAIN_PAGE_SIZE + 4U)

/* ── Device table ── */

static const dev_eep_config_t s_configs[DEV_EEP_CFG_MAX_DEVICES] =
{
    {
        DEV_EEP_MAIN,
        DEV_I2C_BUS_EEPROM,
        ((dev_i2c_addr_t)0x50U),
        DEV_EEP_MAIN_TOTAL_SIZE,
        DEV_EEP_MAIN_PAGE_SIZE,
        DEV_EEP_MEM_ADDR_SIZE_8BIT,
        DEV_EEP_CFG_DEFAULT_WRITE_CYCLE_TIME_MS
    },
};

/* ── Per-device state ── */

static bool s_initialized[DEV_EEP_CFG_MAX_DEVICES];

/* ── Internal helpers ── */

static const dev_eep_config_t *dev_eep_find_config(dev_eep_id_t eep_id)
{
    uint8_t i;

    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        if (s_configs[i].eep_id == eep_id)
        {
            return &s_configs[i];
        }
    }
    return NULL;
}

static dev_err_t dev_eep_validate_params(const dev_eep_config_t *cfg,
                                         dev_eep_addr_t addr,
                                         dev_eep_size_t length)
{
    if (cfg == NULL)
    {
        return DEV_ERR_NULL_PTR;
    }
    if (length == 0U)
    {
        return DEV_ERR_INVALID_ARG;
    }
    if (addr >= cfg->total_size)
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    if ((addr + length) > cfg->total_size)
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    if ((addr + length) < addr) /* overflow check */
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    return DEV_OK;
}

static dev_err_t dev_eep_wait_write_cycle(const dev_eep_config_t *cfg)
{
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_NULL_PTR);

#if (DEV_EEP_CFG_ACK_POLLING_ENABLED == DEV_ON)
    {
        uint32_t elapsed_ms = 0U;
        dev_err_t probe_result;

        while (elapsed_ms < DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS)
        {
            probe_result = dev_i2c_probe(cfg->i2c_bus,
                                         cfg->i2c_addr,
                                         (dev_i2c_timeout_t)1U);
            if (probe_result == DEV_OK)
            {
                return DEV_OK;
            }

            dev_delay_ms(1U);
            elapsed_ms++;
        }

        return DEV_ERR_TIMEOUT;
    }
#else
    {
        dev_delay_ms(cfg->write_cycle_time_ms);
        return DEV_OK;
    }
#endif
}

static dev_err_t dev_eep_i2c_read(const dev_eep_config_t *cfg,
                                   dev_eep_addr_t addr,
                                   uint8_t *data,
                                   dev_eep_size_t length)
{
    dev_err_t result;

    DEV_CHECK_RET((cfg != NULL),  DEV_ERR_NULL_PTR);
    DEV_CHECK_RET((data != NULL), DEV_ERR_NULL_PTR);

    if ((cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT) ||
        (cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_16BIT))
    {
        dev_i2c_mem_addr_size_t i2c_addr_size;

        i2c_addr_size = (cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT)
                        ? DEV_I2C_MEM_ADDR_SIZE_8BIT
                        : DEV_I2C_MEM_ADDR_SIZE_16BIT;

        result = dev_i2c_mem_read(cfg->i2c_bus,
                                   cfg->i2c_addr,
                                   (uint16_t)addr,
                                   i2c_addr_size,
                                   data,
                                   (uint16_t)length,
                                   DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }
    else
    {
        uint8_t addr_bytes[4U];
        uint8_t addr_len;

        if (cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_24BIT)
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

        result = dev_i2c_write_read(cfg->i2c_bus,
                                     cfg->i2c_addr,
                                     addr_bytes, addr_len,
                                     data, (uint16_t)length,
                                     DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }

    return result;
}

static dev_err_t dev_eep_i2c_write_page(const dev_eep_config_t *cfg,
                                         dev_eep_addr_t addr,
                                         const uint8_t *data,
                                         dev_eep_size_t length)
{
    dev_err_t result;

    DEV_CHECK_RET((cfg != NULL),  DEV_ERR_NULL_PTR);
    DEV_CHECK_RET((data != NULL), DEV_ERR_NULL_PTR);
    DEV_CHECK_RET((length <= cfg->page_size), DEV_ERR_OUT_OF_RANGE);

    if ((cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT) ||
        (cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_16BIT))
    {
        dev_i2c_mem_addr_size_t i2c_addr_size;

        i2c_addr_size = (cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT)
                        ? DEV_I2C_MEM_ADDR_SIZE_8BIT
                        : DEV_I2C_MEM_ADDR_SIZE_16BIT;

        result = dev_i2c_mem_write(cfg->i2c_bus,
                                    cfg->i2c_addr,
                                    (uint16_t)addr,
                                    i2c_addr_size,
                                    data,
                                    (uint16_t)length,
                                    DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }
    else
    {
        uint8_t buf[DEV_EEP_MAX_BUF_SIZE];
        uint8_t addr_len;

        if (cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_24BIT)
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

        if ((size_t)((size_t)addr_len + (size_t)length) > sizeof(buf))
        {
            return DEV_ERR_OUT_OF_RANGE;
        }

        (void)memcpy(&buf[addr_len], data, (size_t)length);

        result = dev_i2c_write(cfg->i2c_bus,
                                cfg->i2c_addr,
                                buf,
                                (uint16_t)(addr_len + length),
                                DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }

    return result;
}

/* ── Public API ── */

dev_err_t dev_eep_init(dev_eep_id_t eep_id)
{
    const dev_eep_config_t *cfg;
    dev_err_t result;

    cfg = dev_eep_find_config(eep_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (s_initialized[eep_id])
    {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    /* Verify device is present on the I2C bus */
    result = dev_i2c_probe(cfg->i2c_bus,
                           cfg->i2c_addr,
                           DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    if (result != DEV_OK)
    {
        return result;
    }

    s_initialized[eep_id] = true;
    return DEV_OK;
}

dev_err_t dev_eep_deinit(dev_eep_id_t eep_id)
{
    const dev_eep_config_t *cfg;

    cfg = dev_eep_find_config(eep_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (!s_initialized[eep_id])
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    s_initialized[eep_id] = false;
    return DEV_OK;
}

dev_err_t dev_eep_read(dev_eep_id_t eep_id,
                       uint32_t address,
                       uint8_t *data,
                       uint32_t length)
{
    const dev_eep_config_t *cfg;
    dev_err_t result;

    cfg = dev_eep_find_config(eep_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (!s_initialized[eep_id])
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    DEV_CHECK_PTR_RET(data);

    result = dev_eep_validate_params(cfg, (dev_eep_addr_t)address,
                                     (dev_eep_size_t)length);
    DEV_CHECK_OK_RET(result);

    /* Reads are not page-constrained — read the full range */
    result = dev_eep_i2c_read(cfg, (dev_eep_addr_t)address, data,
                               (dev_eep_size_t)length);

    return result;
}

dev_err_t dev_eep_write(dev_eep_id_t eep_id,
                        uint32_t address,
                        const uint8_t *data,
                        uint32_t length)
{
    const dev_eep_config_t *cfg;
    dev_eep_addr_t addr;
    dev_eep_size_t remaining;
    dev_eep_size_t chunk;
    dev_err_t result;

    cfg = dev_eep_find_config(eep_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (!s_initialized[eep_id])
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    DEV_CHECK_PTR_RET(data);

    result = dev_eep_validate_params(cfg, (dev_eep_addr_t)address,
                                     (dev_eep_size_t)length);
    DEV_CHECK_OK_RET(result);

    /* Split write at page boundaries */
    addr      = (dev_eep_addr_t)address;
    remaining = (dev_eep_size_t)length;

    while (remaining > 0U)
    {
        /* Determine chunk size: up to page_size, don't cross page boundary */
        chunk = cfg->page_size - (addr % cfg->page_size);
        if (chunk > remaining)
        {
            chunk = remaining;
        }

        result = dev_eep_i2c_write_page(cfg, addr, data, chunk);
        if (result != DEV_OK)
        {
            return result;
        }

        /* Wait for write cycle after each page */
        result = dev_eep_wait_write_cycle(cfg);
        if (result != DEV_OK)
        {
            return result;
        }

        data      += chunk;
        addr      += chunk;
        remaining -= chunk;
    }

    return DEV_OK;
}

dev_err_t dev_eep_is_ready(dev_eep_id_t eep_id)
{
    const dev_eep_config_t *cfg;

    cfg = dev_eep_find_config(eep_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (!s_initialized[eep_id])
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    return dev_i2c_probe(cfg->i2c_bus,
                         cfg->i2c_addr,
                         DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
}

dev_err_t dev_eep_get_info(dev_eep_id_t eep_id,
                           dev_eep_info_t *info)
{
    const dev_eep_config_t *cfg;

    cfg = dev_eep_find_config(eep_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    DEV_CHECK_PTR_RET(info);

    info->total_size    = cfg->total_size;
    info->page_size     = cfg->page_size;
    info->mem_addr_size = cfg->mem_addr_size;

    return DEV_OK;
}
