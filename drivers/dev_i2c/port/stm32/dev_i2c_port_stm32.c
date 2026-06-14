#include "dev_i2c_port_stm32.h"
#include "dev_compiler.h"

#ifdef HAL_I2C_MODULE_ENABLED

/*
 * Cube-generated HAL handles — declare as extern.
 * If your handles are named differently, update the map below.
 */
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

static const dev_i2c_hw_bus_t s_i2c_map[DEV_I2C_CFG_MAX_BUSES] = {
    [DEV_I2C_BUS_SENSOR] = { DEV_I2C_BUS_SENSOR, &hi2c1, DEV_I2C_SPEED_FAST },
    [DEV_I2C_BUS_EEPROM] = { DEV_I2C_BUS_EEPROM, &hi2c2, DEV_I2C_SPEED_STANDARD },
};

static bool s_initialized = false;

static const dev_i2c_hw_bus_t *find_bus(dev_i2c_bus_t id)
    { return (id < DEV_I2C_CFG_MAX_BUSES && s_i2c_map[id].handle) ? &s_i2c_map[id] : NULL; }

static uint16_t stm32_hal_addr(dev_i2c_addr_t addr)
    { return (uint16_t)(addr << 1U); }

static uint16_t stm32_mem_size(dev_i2c_mem_addr_size_t s)
    { return (s == DEV_I2C_MEM_ADDR_SIZE_16BIT) ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT; }

static dev_err_t stm32_map(HAL_StatusTypeDef hal, I2C_HandleTypeDef *h)
{
    if (hal == HAL_OK) return DEV_OK;
    if (hal == HAL_TIMEOUT) return DEV_ERR_TIMEOUT;
    if (hal == HAL_BUSY) return DEV_ERR_BUSY;
    /* Check for NACK or bus error */
    if (h != NULL) {
        uint32_t err = HAL_I2C_GetError(h);
        if (err & HAL_I2C_ERROR_AF)   return DEV_ERR_NO_ACK;
        if (err & HAL_I2C_ERROR_BERR) return DEV_ERR_BUS;
    }
    return DEV_ERR_HW_FAILURE;
}

/* ── Port API ── */

dev_err_t dev_i2c_port_init(void)
{
    /* Cube has already initialized hardware. Just mark as ready. */
    uint16_t valid = 0U;
    for (uint16_t i = 0U; i < DEV_I2C_CFG_MAX_BUSES; i++)
        if (s_i2c_map[i].handle) valid++;
    if (valid == 0U) return DEV_ERR_HW_FAILURE;
    s_initialized = true;
    return DEV_OK;
}

dev_err_t dev_i2c_port_deinit(void)
    { s_initialized = false; return DEV_OK; }

dev_err_t dev_i2c_port_set_speed(dev_i2c_bus_t bus, dev_i2c_speed_t speed)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (!b) return DEV_ERR_INVALID_ARG;
    /* Cube manages timing — speed change not supported in managed mode */
    DEV_UNUSED(speed);
    DEV_UNUSED(b);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_i2c_port_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                             const uint8_t *data, uint16_t len, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (!b) return DEV_ERR_INVALID_ARG;
    return stm32_map(HAL_I2C_Master_Transmit(b->handle, stm32_hal_addr(addr),
                     (uint8_t *)data, len, (uint32_t)to), b->handle);
}

dev_err_t dev_i2c_port_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                            uint8_t *data, uint16_t len, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (!b) return DEV_ERR_INVALID_ARG;
    return stm32_map(HAL_I2C_Master_Receive(b->handle, stm32_hal_addr(addr),
                     data, len, (uint32_t)to), b->handle);
}

dev_err_t dev_i2c_port_write_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                  const uint8_t *wd, uint16_t wl,
                                  uint8_t *rd, uint16_t rl, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (!b) return DEV_ERR_INVALID_ARG;

    HAL_StatusTypeDef hal;
    hal = HAL_I2C_Master_Transmit(b->handle, stm32_hal_addr(addr),
                                  (uint8_t *)wd, wl, (uint32_t)to);
    if (hal != HAL_OK) return stm32_map(hal, b->handle);

    hal = HAL_I2C_Master_Receive(b->handle, stm32_hal_addr(addr),
                                 rd, rl, (uint32_t)to);
    return stm32_map(hal, b->handle);
}

dev_err_t dev_i2c_port_mem_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                 uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_size,
                                 const uint8_t *data, uint16_t len, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (!b) return DEV_ERR_INVALID_ARG;
    return stm32_map(HAL_I2C_Mem_Write(b->handle, stm32_hal_addr(addr),
                     mem_addr, stm32_mem_size(mem_size), (uint8_t *)data, len, (uint32_t)to),
                     b->handle);
}

dev_err_t dev_i2c_port_mem_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_size,
                                uint8_t *data, uint16_t len, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (!b) return DEV_ERR_INVALID_ARG;
    return stm32_map(HAL_I2C_Mem_Read(b->handle, stm32_hal_addr(addr),
                     mem_addr, stm32_mem_size(mem_size), data, len, (uint32_t)to),
                     b->handle);
}

dev_err_t dev_i2c_port_probe(dev_i2c_bus_t bus, dev_i2c_addr_t addr, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (!b) return DEV_ERR_INVALID_ARG;
    HAL_StatusTypeDef hal = HAL_I2C_IsDeviceReady(b->handle, stm32_hal_addr(addr), 1U, (uint32_t)to);
    if (hal == HAL_OK) return DEV_OK;
    if (hal == HAL_TIMEOUT) return DEV_ERR_TIMEOUT;
    return DEV_ERR_NO_ACK;
}

dev_err_t dev_i2c_port_recover_bus(dev_i2c_bus_t bus)
{
    /* Cube manages GPIO config — bus recovery not supported in managed mode */
    DEV_UNUSED(bus);
    return DEV_ERR_NOT_SUPPORTED;
}

#else /* HAL_I2C_MODULE_ENABLED not defined */

dev_err_t dev_i2c_port_init(void)                { return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_deinit(void)              { return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_set_speed(dev_i2c_bus_t b, dev_i2c_speed_t s)
    { DEV_UNUSED(b); DEV_UNUSED(s); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_write(dev_i2c_bus_t b, dev_i2c_addr_t a,
                             const uint8_t *d, uint16_t l, dev_i2c_timeout_t t)
    { DEV_UNUSED(b);DEV_UNUSED(a);DEV_UNUSED(d);DEV_UNUSED(l);DEV_UNUSED(t); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_read(dev_i2c_bus_t b, dev_i2c_addr_t a,
                            uint8_t *d, uint16_t l, dev_i2c_timeout_t t)
    { DEV_UNUSED(b);DEV_UNUSED(a);DEV_UNUSED(d);DEV_UNUSED(l);DEV_UNUSED(t); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_write_read(dev_i2c_bus_t b, dev_i2c_addr_t a,
                                  const uint8_t *wd, uint16_t wl,
                                  uint8_t *rd, uint16_t rl, dev_i2c_timeout_t t)
    { DEV_UNUSED(b);DEV_UNUSED(a);DEV_UNUSED(wd);DEV_UNUSED(wl);DEV_UNUSED(rd);DEV_UNUSED(rl);DEV_UNUSED(t); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_mem_write(dev_i2c_bus_t b, dev_i2c_addr_t a, uint16_t ma,
                                 dev_i2c_mem_addr_size_t ms, const uint8_t *d,
                                 uint16_t l, dev_i2c_timeout_t t)
    { DEV_UNUSED(b);DEV_UNUSED(a);DEV_UNUSED(ma);DEV_UNUSED(ms);DEV_UNUSED(d);DEV_UNUSED(l);DEV_UNUSED(t); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_mem_read(dev_i2c_bus_t b, dev_i2c_addr_t a, uint16_t ma,
                                dev_i2c_mem_addr_size_t ms, uint8_t *d,
                                uint16_t l, dev_i2c_timeout_t t)
    { DEV_UNUSED(b);DEV_UNUSED(a);DEV_UNUSED(ma);DEV_UNUSED(ms);DEV_UNUSED(d);DEV_UNUSED(l);DEV_UNUSED(t); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_probe(dev_i2c_bus_t b, dev_i2c_addr_t a, dev_i2c_timeout_t t)
    { DEV_UNUSED(b);DEV_UNUSED(a);DEV_UNUSED(t); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_recover_bus(dev_i2c_bus_t b)
    { DEV_UNUSED(b); return DEV_ERR_NOT_SUPPORTED; }

#endif /* HAL_I2C_MODULE_ENABLED */
