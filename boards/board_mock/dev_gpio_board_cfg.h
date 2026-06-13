#ifndef DEV_GPIO_BOARD_CFG_H
#define DEV_GPIO_BOARD_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio.h"

/* Logical channel IDs — sparse to test channel->index mapping */
#define DEV_GPIO_CHANNEL_LED_STATUS      ((dev_gpio_channel_t)0U)
#define DEV_GPIO_CHANNEL_BUTTON_USER     ((dev_gpio_channel_t)10U)

/* Extern the board GPIO config for application use */
extern const dev_gpio_config_t g_dev_gpio_config;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_BOARD_CFG_H */
