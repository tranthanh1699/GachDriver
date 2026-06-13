#ifndef DEV_GPIO_BOARD_CFG_H
#define DEV_GPIO_BOARD_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio.h"

/* ── Logical channel IDs for STM32H743 ── */

/*
 * PB0 — GPIO Port B, Pin 0
 * Example: LED output or general-purpose digital output
 */
#define DEV_GPIO_CHANNEL_PB0              ((dev_gpio_channel_t)0U)

/*
 * PB1 — GPIO Port B, Pin 1
 * Example: LED output or general-purpose digital output
 */
#define DEV_GPIO_CHANNEL_PB1              ((dev_gpio_channel_t)1U)

/* Extern the board GPIO config for application use */
extern const dev_gpio_config_t g_dev_gpio_config;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_BOARD_CFG_H */
