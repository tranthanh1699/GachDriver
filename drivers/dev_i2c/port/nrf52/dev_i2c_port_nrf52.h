#ifndef DEV_I2C_PORT_NRF52_H
#define DEV_I2C_PORT_NRF52_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_i2c_port.h"

typedef struct {
    dev_i2c_bus_t   bus_id;
    uint8_t         instance_id;
    uint32_t        sda_pin;
    uint32_t        scl_pin;
    dev_i2c_speed_t default_speed;
} dev_i2c_hw_bus_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_I2C_PORT_NRF52_H */
