#ifndef DEV_I2C_PORT_STM32_H
#define DEV_I2C_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_i2c_port.h"
#include "stm32h7xx_hal.h"

/* STM32 I2C timing constants (HCLK-dependent, adjust for your clock tree) */
#define DEV_I2C_STM32_TIMING_100KHZ   (0x10909CECU)
#define DEV_I2C_STM32_TIMING_400KHZ   (0x00C0BAECU)
#define DEV_I2C_STM32_TIMING_1MHZ     (0x00300B25U)

typedef struct {
    dev_i2c_bus_t     bus_id;
    I2C_TypeDef      *instance;
    GPIO_TypeDef     *scl_port;
    uint16_t          scl_pin;
    GPIO_TypeDef     *sda_port;
    uint16_t          sda_pin;
    uint32_t          gpio_alternate;
    dev_i2c_speed_t   default_speed;
    uint32_t          timing;
} dev_i2c_hw_bus_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_I2C_PORT_STM32_H */
