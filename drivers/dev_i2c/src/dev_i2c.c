#include "dev_i2c.h"
#include "dev_i2c_port.h"
#include "dev_common.h"

static bool g_initialized = false;

/* ── Helpers ── */

static bool bus_ok(dev_i2c_bus_t b)       { return (b < DEV_I2C_CFG_MAX_BUSES); }
static bool addr_ok(dev_i2c_addr_t a)     { return (a <= DEV_I2C_ADDR_7BIT_MAX); }
static bool speed_ok(dev_i2c_speed_t s)
    { return (s == DEV_I2C_SPEED_STANDARD) || (s == DEV_I2C_SPEED_FAST)
          || (s == DEV_I2C_SPEED_FAST_PLUS) || (s == DEV_I2C_SPEED_HIGH); }
static bool mem_ok(dev_i2c_mem_addr_size_t m)
    { return (m == DEV_I2C_MEM_ADDR_SIZE_8BIT) || (m == DEV_I2C_MEM_ADDR_SIZE_16BIT); }

static dev_err_t map_err(dev_err_t e)
{
    if (e == DEV_OK)                return DEV_OK;
    if (e == DEV_ERR_INVALID_ARG)   return DEV_ERR_INVALID_ARG;
    if (e == DEV_ERR_NULL_PTR)      return DEV_ERR_NULL_PTR;
    if (e == DEV_ERR_NOT_SUPPORTED) return DEV_ERR_NOT_SUPPORTED;
    if (e == DEV_ERR_TIMEOUT)       return DEV_ERR_TIMEOUT;
    if (e == DEV_ERR_NO_ACK)        return DEV_ERR_NO_ACK;
    if (e == DEV_ERR_BUS)           return DEV_ERR_BUS;
    if (e == DEV_ERR_BUSY)          return DEV_ERR_BUSY;
    if (e == DEV_ERR_OUT_OF_RANGE)  return DEV_ERR_OUT_OF_RANGE;
    return DEV_ERR_HW_FAILURE;
}

/* ── Lifecycle ── */

dev_err_t dev_i2c_init(void)
{
    if (g_initialized) return DEV_ERR_ALREADY_INITIALIZED;
    g_initialized = true;
    dev_err_t e = dev_i2c_port_init();
    if (e != DEV_OK) { g_initialized = false; return map_err(e); }
    return DEV_OK;
}

dev_err_t dev_i2c_deinit(void)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    g_initialized = false;
    dev_err_t e = dev_i2c_port_deinit();
    return (e != DEV_OK) ? map_err(e) : DEV_OK;
}

bool dev_i2c_is_initialized(void) { return g_initialized; }

/* ── Speed ── */

dev_err_t dev_i2c_set_speed(dev_i2c_bus_t bus, dev_i2c_speed_t speed)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!bus_ok(bus))   return DEV_ERR_INVALID_ARG;
    if (!speed_ok(speed)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_i2c_port_set_speed(bus, speed));
}

/* ── Write / Read ── */

dev_err_t dev_i2c_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                        const uint8_t *data, uint16_t length, dev_i2c_timeout_t timeout_ms)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!bus_ok(bus))   return DEV_ERR_INVALID_ARG;
    if (!addr_ok(addr)) return DEV_ERR_INVALID_ARG;
    if (data == NULL)   return DEV_ERR_NULL_PTR;
    if (length == 0U)   return DEV_ERR_INVALID_ARG;
    return map_err(dev_i2c_port_write(bus, addr, data, length, timeout_ms));
}

dev_err_t dev_i2c_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                       uint8_t *data, uint16_t length, dev_i2c_timeout_t timeout_ms)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!bus_ok(bus))   return DEV_ERR_INVALID_ARG;
    if (!addr_ok(addr)) return DEV_ERR_INVALID_ARG;
    if (data == NULL)   return DEV_ERR_NULL_PTR;
    if (length == 0U)   return DEV_ERR_INVALID_ARG;
    return map_err(dev_i2c_port_read(bus, addr, data, length, timeout_ms));
}

dev_err_t dev_i2c_write_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                             const uint8_t *write_data, uint16_t write_length,
                             uint8_t *read_data, uint16_t read_length,
                             dev_i2c_timeout_t timeout_ms)
{
    if (!g_initialized)   return DEV_ERR_NOT_INITIALIZED;
    if (!bus_ok(bus))     return DEV_ERR_INVALID_ARG;
    if (!addr_ok(addr))   return DEV_ERR_INVALID_ARG;
    if (write_data == NULL) return DEV_ERR_NULL_PTR;
    if (read_data == NULL)  return DEV_ERR_NULL_PTR;
    if ((write_length == 0U) || (read_length == 0U)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_i2c_port_write_read(bus, addr,
                    write_data, write_length, read_data, read_length, timeout_ms));
}

/* ── Memory access ── */

#if (DEV_I2C_CFG_MEM_ACCESS_ENABLED == DEV_ON)

dev_err_t dev_i2c_mem_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                            uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_addr_size,
                            const uint8_t *data, uint16_t length, dev_i2c_timeout_t timeout_ms)
{
    if (!g_initialized)    return DEV_ERR_NOT_INITIALIZED;
    if (!bus_ok(bus))      return DEV_ERR_INVALID_ARG;
    if (!addr_ok(addr))    return DEV_ERR_INVALID_ARG;
    if (!mem_ok(mem_addr_size)) return DEV_ERR_INVALID_ARG;
    if (data == NULL)      return DEV_ERR_NULL_PTR;
    if (length == 0U)      return DEV_ERR_INVALID_ARG;
    return map_err(dev_i2c_port_mem_write(bus, addr, mem_addr, mem_addr_size,
                                           data, length, timeout_ms));
}

dev_err_t dev_i2c_mem_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                           uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_addr_size,
                           uint8_t *data, uint16_t length, dev_i2c_timeout_t timeout_ms)
{
    if (!g_initialized)    return DEV_ERR_NOT_INITIALIZED;
    if (!bus_ok(bus))      return DEV_ERR_INVALID_ARG;
    if (!addr_ok(addr))    return DEV_ERR_INVALID_ARG;
    if (!mem_ok(mem_addr_size)) return DEV_ERR_INVALID_ARG;
    if (data == NULL)      return DEV_ERR_NULL_PTR;
    if (length == 0U)      return DEV_ERR_INVALID_ARG;
    return map_err(dev_i2c_port_mem_read(bus, addr, mem_addr, mem_addr_size,
                                          data, length, timeout_ms));
}

#else

dev_err_t dev_i2c_mem_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                            uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_addr_size,
                            const uint8_t *data, uint16_t length, dev_i2c_timeout_t timeout_ms)
{
    DEV_UNUSED(bus); DEV_UNUSED(addr); DEV_UNUSED(mem_addr);
    DEV_UNUSED(mem_addr_size); DEV_UNUSED(data); DEV_UNUSED(length); DEV_UNUSED(timeout_ms);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_i2c_mem_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                           uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_addr_size,
                           uint8_t *data, uint16_t length, dev_i2c_timeout_t timeout_ms)
{
    DEV_UNUSED(bus); DEV_UNUSED(addr); DEV_UNUSED(mem_addr);
    DEV_UNUSED(mem_addr_size); DEV_UNUSED(data); DEV_UNUSED(length); DEV_UNUSED(timeout_ms);
    return DEV_ERR_NOT_SUPPORTED;
}

#endif /* DEV_I2C_CFG_MEM_ACCESS_ENABLED */

/* ── Probe / Recover ── */

dev_err_t dev_i2c_probe(dev_i2c_bus_t bus, dev_i2c_addr_t addr, dev_i2c_timeout_t timeout_ms)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!bus_ok(bus))   return DEV_ERR_INVALID_ARG;
    if (!addr_ok(addr)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_i2c_port_probe(bus, addr, timeout_ms));
}

#if (DEV_I2C_CFG_BUS_RECOVERY_ENABLED == DEV_ON)

dev_err_t dev_i2c_recover_bus(dev_i2c_bus_t bus)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!bus_ok(bus))   return DEV_ERR_INVALID_ARG;
    return map_err(dev_i2c_port_recover_bus(bus));
}

#else

dev_err_t dev_i2c_recover_bus(dev_i2c_bus_t bus)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!bus_ok(bus))   return DEV_ERR_INVALID_ARG;
    DEV_UNUSED(bus);
    return DEV_ERR_NOT_SUPPORTED;
}

#endif /* DEV_I2C_CFG_BUS_RECOVERY_ENABLED */
