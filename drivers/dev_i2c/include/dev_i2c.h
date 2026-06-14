#ifndef DEV_I2C_H
#define DEV_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_i2c_types.h"
#include "dev_i2c_cfg.h"
#include "dev_error.h"

dev_err_t dev_i2c_init(void);
dev_err_t dev_i2c_deinit(void);
bool     dev_i2c_is_initialized(void);

dev_err_t dev_i2c_set_speed(dev_i2c_bus_t bus, dev_i2c_speed_t speed);

dev_err_t dev_i2c_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                        const uint8_t *data, uint16_t length,
                        dev_i2c_timeout_t timeout_ms);

dev_err_t dev_i2c_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                       uint8_t *data, uint16_t length,
                       dev_i2c_timeout_t timeout_ms);

dev_err_t dev_i2c_write_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                             const uint8_t *write_data, uint16_t write_length,
                             uint8_t *read_data, uint16_t read_length,
                             dev_i2c_timeout_t timeout_ms);

dev_err_t dev_i2c_mem_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                            uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_addr_size,
                            const uint8_t *data, uint16_t length,
                            dev_i2c_timeout_t timeout_ms);

dev_err_t dev_i2c_mem_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                           uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_addr_size,
                           uint8_t *data, uint16_t length,
                           dev_i2c_timeout_t timeout_ms);

dev_err_t dev_i2c_probe(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                        dev_i2c_timeout_t timeout_ms);

dev_err_t dev_i2c_recover_bus(dev_i2c_bus_t bus);

#ifdef __cplusplus
}
#endif

#endif /* DEV_I2C_H */
