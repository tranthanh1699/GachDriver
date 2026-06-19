#ifndef DEV_I2C_CFG_H
#define DEV_I2C_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_i2c_types.h"

#define DEV_I2C_CFG_MAX_BUSES              (2U)
#define DEV_I2C_CFG_RUNTIME_CHECK_ENABLED  DEV_ON
#define DEV_I2C_CFG_10BIT_ADDR_ENABLED     DEV_OFF
#define DEV_I2C_CFG_MEM_ACCESS_ENABLED     DEV_ON
#define DEV_I2C_CFG_BUS_RECOVERY_ENABLED   DEV_OFF
#define DEV_I2C_STM32_CUBE_MANAGED_HW_INIT DEV_ON
#define DEV_I2C_TIMEOUT_DEFAULT_MS         (100U)
#define DEV_I2C_ADDR_7BIT_MAX              (0x7FU)

/* Logical I2C bus IDs */
#define DEV_I2C_BUS_SENSOR                 ((dev_i2c_bus_t)0U)
#define DEV_I2C_BUS_EEPROM                 ((dev_i2c_bus_t)1U)

#ifdef __cplusplus
}
#endif

#endif /* DEV_I2C_CFG_H */
