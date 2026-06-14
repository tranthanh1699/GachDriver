#ifndef DEV_GPIO_TYPES_H
#define DEV_GPIO_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef uint16_t dev_gpio_pin_t;

typedef enum {
    DEV_GPIO_LEVEL_LOW  = 0,
    DEV_GPIO_LEVEL_HIGH = 1
} dev_gpio_level_t;

typedef enum {
    DEV_GPIO_PULL_NONE = 0,
    DEV_GPIO_PULL_UP,
    DEV_GPIO_PULL_DOWN
} dev_gpio_pull_t;

typedef enum {
    DEV_GPIO_INTR_DISABLE = 0,
    DEV_GPIO_INTR_RISING_EDGE,
    DEV_GPIO_INTR_FALLING_EDGE,
    DEV_GPIO_INTR_BOTH_EDGES,
    DEV_GPIO_INTR_LOW_LEVEL,
    DEV_GPIO_INTR_HIGH_LEVEL
} dev_gpio_intr_t;

typedef void (*dev_gpio_callback_t)(dev_gpio_pin_t pin, void *user_arg);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_TYPES_H */
