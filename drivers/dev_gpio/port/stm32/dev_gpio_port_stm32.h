#ifndef DEV_GPIO_PORT_STM32_H
#define DEV_GPIO_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"
#include "stm32h7xx_hal.h"

/* STM32-specific pin mapping type */
typedef struct {
    dev_gpio_pin_t pin_id;
    GPIO_TypeDef  *port;
    uint16_t       hal_pin;
} dev_gpio_hw_pin_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_STM32_H */
