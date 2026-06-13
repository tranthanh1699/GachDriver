#ifndef DEV_GPIO_PORT_ESP32_H
#define DEV_GPIO_PORT_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"

typedef struct {
    dev_gpio_pin_t pin_id;
    int            gpio_num;
} dev_gpio_hw_pin_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_ESP32_H */
