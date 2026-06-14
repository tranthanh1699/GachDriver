#ifndef DEV_I2C_PORT_ESP32_H
#define DEV_I2C_PORT_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_i2c_port.h"

typedef struct {
    dev_i2c_bus_t   bus_id;
    int             port_num;
    int             sda_gpio;
    int             scl_gpio;
    dev_i2c_speed_t default_speed;
} dev_i2c_hw_bus_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_I2C_PORT_ESP32_H */
