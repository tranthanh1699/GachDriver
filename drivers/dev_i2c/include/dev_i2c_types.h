#ifndef DEV_I2C_TYPES_H
#define DEV_I2C_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef uint8_t  dev_i2c_bus_t;
typedef uint16_t dev_i2c_addr_t;
typedef uint32_t dev_i2c_timeout_t;

typedef enum {
    DEV_I2C_SPEED_STANDARD  = 0,
    DEV_I2C_SPEED_FAST,
    DEV_I2C_SPEED_FAST_PLUS,
    DEV_I2C_SPEED_HIGH
} dev_i2c_speed_t;

typedef enum {
    DEV_I2C_MEM_ADDR_SIZE_8BIT  = 0,
    DEV_I2C_MEM_ADDR_SIZE_16BIT
} dev_i2c_mem_addr_size_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_I2C_TYPES_H */
