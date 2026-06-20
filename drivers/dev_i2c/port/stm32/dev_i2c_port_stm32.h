#ifndef DEV_I2C_PORT_STM32_H
#define DEV_I2C_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_i2c_port.h"
#include "stm32h7xx_hal_conf.h"
/*
 * STM32 Cube-managed I2C port.
 *
 * Assumes CubeMX/CubeIDE has already configured:
 *   - SDA/SCL GPIO alternate function
 *   - I2C peripheral clock
 *   - I2C timing, filters
 *   - NVIC priority
 *   - HAL I2C init (hi2c1, hi2c2, etc.)
 *
 * This port only wraps Cube HAL handles. No GPIO/clock/NVIC/timing
 * configuration is performed.
 *
 * To enable:
 *   1. Uncomment #define HAL_I2C_MODULE_ENABLED in stm32h7xx_hal_conf.h
 *   2. Ensure hi2cX handles are declared extern in your project
 */

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32h7xx_hal.h"

typedef struct {
    dev_i2c_bus_t   bus_id;
    I2C_HandleTypeDef *handle;
    dev_i2c_speed_t default_speed;
} dev_i2c_hw_bus_t;

#endif /* HAL_I2C_MODULE_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* DEV_I2C_PORT_STM32_H */
