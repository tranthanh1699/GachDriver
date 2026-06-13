#ifndef DEV_GPIO_TYPES_H
#define DEV_GPIO_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef uint16_t dev_gpio_channel_t;

typedef enum {
    DEV_GPIO_LEVEL_LOW = 0,
    DEV_GPIO_LEVEL_HIGH = 1
} dev_gpio_level_t;

typedef enum {
    DEV_GPIO_DIRECTION_INPUT = 0,
    DEV_GPIO_DIRECTION_OUTPUT,
    DEV_GPIO_DIRECTION_INPUT_OUTPUT
} dev_gpio_direction_t;

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
} dev_gpio_intr_type_t;

typedef void (*dev_gpio_isr_callback_t)(dev_gpio_channel_t channel, void *user_arg);

typedef struct {
    dev_gpio_channel_t      channel;
    dev_gpio_direction_t    direction;
    dev_gpio_pull_t         pull;
    dev_gpio_level_t        default_level;
    dev_gpio_intr_type_t    interrupt;
    dev_gpio_isr_callback_t callback;
    void                   *callback_arg;
} dev_gpio_channel_config_t;

typedef struct {
    const dev_gpio_channel_config_t *channels;
    uint16_t                         channel_count;
} dev_gpio_config_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_TYPES_H */
