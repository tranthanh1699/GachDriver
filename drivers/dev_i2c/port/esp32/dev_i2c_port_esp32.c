/*
 * ESP32 I2C port — PLACEHOLDER STUB.
 *
 * This file provides skeleton implementations that return DEV_ERR_NOT_SUPPORTED
 * for all transfer operations. To complete this port:
 *   1. Include "driver/i2c.h" (ESP-IDF)
 *   2. Configure i2c_param_config() + i2c_driver_install() in port_init()
 *   3. Replace stubs with i2c_master_write_to_device() etc.
 *   4. Implement probe using i2c_master_probe() or a short write
 *   5. Remove this comment block
 */

#include "dev_i2c_port_esp32.h"
#include "dev_common.h"

static const dev_i2c_hw_bus_t s_i2c_map[DEV_I2C_CFG_MAX_BUSES] = {
    [DEV_I2C_BUS_SENSOR] = { DEV_I2C_BUS_SENSOR, 0, 21, 22, DEV_I2C_SPEED_FAST },
    [DEV_I2C_BUS_EEPROM] = { DEV_I2C_BUS_EEPROM, 1, 18, 19, DEV_I2C_SPEED_STANDARD },
};

dev_err_t dev_i2c_port_init(void)      { return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_i2c_port_deinit(void)    { return DEV_ERR_NOT_SUPPORTED; }
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
