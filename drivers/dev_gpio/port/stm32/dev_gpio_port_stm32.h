#ifndef DEV_GPIO_PORT_STM32_H
#define DEV_GPIO_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_cfg.h"
#include "stm32h7xx_hal.h"

/* STM32 hardware pin descriptor */
typedef struct {
    dev_gpio_pin_t  logical_id;
    GPIO_TypeDef   *port;
    uint16_t        hal_pin;
    dev_gpio_mode_t mode;
    dev_gpio_pull_t pull;
} dev_gpio_hw_pin_t;

const dev_gpio_hw_pin_t * dev_gpio_get_hw_map(void);
uint16_t                  dev_gpio_get_pin_count(void);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_STM32_H */
