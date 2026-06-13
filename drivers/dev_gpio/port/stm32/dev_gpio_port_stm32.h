#ifndef DEV_GPIO_PORT_STM32_H
#define DEV_GPIO_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"

/* STM32-specific port pin mapping type */
typedef struct {
    dev_gpio_channel_t channel;
    GPIO_TypeDef      *port;
    uint16_t           pin;
} dev_gpio_port_pin_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_STM32_H */
