#ifndef DEV_GPIO_PORT_STM32_H
#define DEV_GPIO_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"

/*
 * Include STM32 HAL for GPIO_TypeDef.
 * This is allowed here because this header is INSIDE the port layer
 * (not a public header) — only port .c files and this header include it.
 */
#include "stm32h7xx_hal.h"

/* STM32-specific port pin mapping type (internal to port layer) */
typedef struct {
    dev_gpio_channel_t channel;
    GPIO_TypeDef      *port;
    uint16_t           pin;
} dev_gpio_port_pin_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_STM32_H */
