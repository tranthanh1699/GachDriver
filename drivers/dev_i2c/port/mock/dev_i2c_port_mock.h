#ifndef DEV_I2C_PORT_MOCK_H
#define DEV_I2C_PORT_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_i2c_port.h"

void dev_i2c_port_mock_reset(void);
void dev_i2c_port_mock_set_error(dev_err_t error);
void dev_i2c_port_mock_clear_error(void);

void dev_i2c_port_mock_attach_device(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                     uint8_t *memory, uint16_t memory_size);
void dev_i2c_port_mock_detach_device(dev_i2c_bus_t bus, dev_i2c_addr_t addr);

#ifdef __cplusplus
}
#endif

#endif /* DEV_I2C_PORT_MOCK_H */
