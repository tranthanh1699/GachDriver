#include "dev_eep.h"
#include "dev_i2c.h"
#include "dev_common.h"
#include "dev_assert.h"
#include <string.h>

/* Maximum internal buffer size for I2C write operations.
 *
 * DEV_EEP_MAX_PAGE_SIZE: the largest page_size across all configured devices.
 * When adding a new device with a larger page, update this macro accordingly:
 *   #define DEV_EEP_MAX_PAGE_SIZE \
 *       ((DEV_EEP_MAIN_PAGE_SIZE) > (DEV_EEP_OTHER_PAGE_SIZE) ? \
 *        (DEV_EEP_MAIN_PAGE_SIZE) : (DEV_EEP_OTHER_PAGE_SIZE))
 *
 * Maximum memory address bytes: 4 (for 32-bit addressing).
 */
#define DEV_EEP_MAX_PAGE_SIZE   DEV_EEP_MAIN_PAGE_SIZE
#define DEV_EEP_MAX_ADDR_BYTES  (4U)
#define DEV_EEP_MAX_BUF_SIZE    (DEV_EEP_MAX_PAGE_SIZE + DEV_EEP_MAX_ADDR_BYTES)

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
static bool s_fault_inject_deinit = false;

/* ── Fault injection (test support) ── */

void dev_eep_set_deinit_fault(bool enable)
{
    s_fault_inject_deinit = enable;
}

/* ── Internal helpers ── */

/**
 * @brief Find a device config entry and return its table index.
 *
 * @param eep_id    Logical EEPROM device ID to search for.
 * @param out_index Output: table index (valid only when non-NULL returned).
 *
 * @return Pointer to config, or NULL if not found.
 */
static const dev_eep_config_t *dev_eep_find_config(dev_eep_id_t eep_id,
                                                   uint8_t *out_index)
{
    uint8_t i;

    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        if (s_configs[i].eep_id == eep_id)
        {
            if (out_index != NULL)
            {
                *out_index = i;
            }
            return &s_configs[i];
        }
    }
    return NULL;
}

/**
 * @brief Validate a device configuration entry.
 *
 * Checks page_size, mem_addr_size, total_size constraints, and
 * that the total size fits the address width.
 *
 * @param cfg Config to validate.
 * @return DEV_OK if valid, DEV_ERR_CONFIG otherwise.
 */
static dev_err_t dev_eep_validate_config(const dev_eep_config_t *cfg)
{
    if (cfg == NULL)
    {
        return DEV_ERR_CONFIG;
    }

    /* page_size is used as divisor — must be non-zero */
    if (cfg->page_size == 0U)
    {
        return DEV_ERR_CONFIG;
    }

    /* mem_addr_size must be a valid enum value */
    if ((cfg->mem_addr_size != DEV_EEP_MEM_ADDR_SIZE_8BIT)  &&
        (cfg->mem_addr_size != DEV_EEP_MEM_ADDR_SIZE_16BIT) &&
        (cfg->mem_addr_size != DEV_EEP_MEM_ADDR_SIZE_24BIT) &&
        (cfg->mem_addr_size != DEV_EEP_MEM_ADDR_SIZE_32BIT))
    {
        return DEV_ERR_CONFIG;
    }

    /* total_size must fit the configured address width */
    if ((cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT) &&
        (cfg->total_size > 256U))
    {
        return DEV_ERR_CONFIG;
    }
    if ((cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_16BIT) &&
        (cfg->total_size > 65536U))
    {
        return DEV_ERR_CONFIG;
    }
    if ((cfg->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_24BIT) &&
        (cfg->total_size > 16777216U))
    {
        return DEV_ERR_CONFIG;
    }

    /* page_size must fit in uint16_t for I2C operations */
    if (cfg->page_size > (dev_eep_size_t)UINT16_MAX)
    {
        return DEV_ERR_CONFIG;
    }

    /* page_size must not exceed the internal write buffer */
    if (cfg->page_size > (dev_eep_size_t)DEV_EEP_MAX_PAGE_SIZE)
    {
        return DEV_ERR_CONFIG;
    }

    return DEV_OK;
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

/* Polling interval converted from µs to ms (minimum 1 ms) */
#define DEV_EEP_ACK_POLL_INTERVAL_MS \
    (((DEV_EEP_CFG_ACK_POLL_INTERVAL_US) + 999U) / 1000U)
#if (DEV_EEP_ACK_POLL_INTERVAL_MS < 1U)
#undef  DEV_EEP_ACK_POLL_INTERVAL_MS
#define DEV_EEP_ACK_POLL_INTERVAL_MS (1U)
#endif

static dev_err_t dev_eep_wait_write_cycle(const dev_eep_config_t *cfg)
{
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_NULL_PTR);

#if (DEV_EEP_CFG_ACK_POLLING_ENABLED == DEV_ON)
    {
        /* Elapsed time is approximate: counted in delay increments.
         * Probe execution time is not included — acceptable because
         * polling timeouts are generous relative to probe latency. */
        uint32_t elapsed_ms = 0U;
        dev_err_t probe_result;

        while (elapsed_ms < DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS)
        {
            probe_result = dev_i2c_probe(cfg->i2c_bus,
                                         cfg->i2c_addr,
                                         (dev_i2c_timeout_t)DEV_EEP_ACK_POLL_INTERVAL_MS);
            if (probe_result == DEV_OK)
            {
                return DEV_OK;
            }

            /* Only NACK/timeout means "device still busy with write cycle".
             * Bus errors, state errors, etc. propagate immediately. */
            if ((probe_result != DEV_ERR_TIMEOUT) &&
                (probe_result != DEV_ERR_NO_ACK))
            {
                return probe_result;
            }

            dev_delay_ms(DEV_EEP_ACK_POLL_INTERVAL_MS);
            elapsed_ms += DEV_EEP_ACK_POLL_INTERVAL_MS;
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
    uint8_t idx;
    dev_err_t result;

    cfg = dev_eep_find_config(eep_id, &idx);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    /* Validate device configuration */
    result = dev_eep_validate_config(cfg);
    if (result != DEV_OK)
    {
        return result;
    }

    if (s_initialized[idx])
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

    s_initialized[idx] = true;
    return DEV_OK;
}

dev_err_t dev_eep_deinit(dev_eep_id_t eep_id)
{
    const dev_eep_config_t *cfg;
    uint8_t idx;

    cfg = dev_eep_find_config(eep_id, &idx);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (!s_initialized[idx])
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Test hook: inject a failure without touching state */
    if (s_fault_inject_deinit)
    {
        s_fault_inject_deinit = false;
        return DEV_ERR_FAIL;
    }

    s_initialized[idx] = false;
    return DEV_OK;
}

dev_err_t dev_eep_read(dev_eep_id_t eep_id,
                       uint32_t address,
                       uint8_t *data,
                       uint32_t length)
{
    const dev_eep_config_t *cfg;
    uint8_t idx;
    dev_err_t result;

    cfg = dev_eep_find_config(eep_id, &idx);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (!s_initialized[idx])
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    DEV_CHECK_PTR_RET(data);

    result = dev_eep_validate_params(cfg, (dev_eep_addr_t)address,
                                     (dev_eep_size_t)length);
    DEV_CHECK_OK_RET(result);

    /* Split reads into UINT16_MAX chunks for the I2C layer */
    {
        dev_eep_addr_t addr      = (dev_eep_addr_t)address;
        dev_eep_size_t remaining = (dev_eep_size_t)length;

        while (remaining > 0U)
        {
            dev_eep_size_t chunk = remaining;
            if (chunk > (dev_eep_size_t)UINT16_MAX)
            {
                chunk = (dev_eep_size_t)UINT16_MAX;
            }

            result = dev_eep_i2c_read(cfg, addr, data, chunk);
            if (result != DEV_OK)
            {
                return result;
            }

            data      += chunk;
            addr      += chunk;
            remaining -= chunk;
        }
    }

    return DEV_OK;
}

dev_err_t dev_eep_write(dev_eep_id_t eep_id,
                        uint32_t address,
                        const uint8_t *data,
                        uint32_t length)
{
    const dev_eep_config_t *cfg;
    uint8_t idx;
    dev_eep_addr_t addr;
    dev_eep_size_t remaining;
    dev_eep_size_t chunk;
    dev_err_t result;

    cfg = dev_eep_find_config(eep_id, &idx);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (!s_initialized[idx])
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    DEV_CHECK_PTR_RET(data);

    /* No top-level UINT16_MAX check — writes are page-split below.
     * Each page chunk is bounded by page_size, which is validated
     * at init to fit in both uint16_t and the internal buffer. */

    result = dev_eep_validate_params(cfg, (dev_eep_addr_t)address,
                                     (dev_eep_size_t)length);
    DEV_CHECK_OK_RET(result);

    /* page_size validated at init — guaranteed non-zero here */

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
    uint8_t idx;

    cfg = dev_eep_find_config(eep_id, &idx);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (!s_initialized[idx])
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

    cfg = dev_eep_find_config(eep_id, NULL);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    DEV_CHECK_PTR_RET(info);

    info->total_size    = cfg->total_size;
    info->page_size     = cfg->page_size;
    info->mem_addr_size = cfg->mem_addr_size;

    return DEV_OK;
}
