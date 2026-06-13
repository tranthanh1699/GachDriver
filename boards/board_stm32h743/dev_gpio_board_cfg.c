#include "dev_gpio_board_cfg.h"
#include "dev_compiler.h"

/*
 * STM32H743 GPIO channel configuration.
 *
 * Pin mapping (port layer responsibility):
 *   PB0 -> GPIOB, Pin 0  (AF: GPIO)
 *   PB1 -> GPIOB, Pin 1  (AF: GPIO)
 */
static dev_gpio_channel_config_t m_channels[] = {
    {
        .channel       = DEV_GPIO_CHANNEL_PB0,
        .direction     = DEV_GPIO_DIRECTION_OUTPUT,
        .pull          = DEV_GPIO_PULL_NONE,
        .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt     = DEV_GPIO_INTR_DISABLE,
        .callback      = NULL,
        .callback_arg  = NULL,
    },
    {
        .channel       = DEV_GPIO_CHANNEL_PB1,
        .direction     = DEV_GPIO_DIRECTION_OUTPUT,
        .pull          = DEV_GPIO_PULL_NONE,
        .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt     = DEV_GPIO_INTR_DISABLE,
        .callback      = NULL,
        .callback_arg  = NULL,
    },
};

const dev_gpio_config_t g_dev_gpio_config = {
    .channels      = m_channels,
    .channel_count = (uint16_t)DEV_ARRAY_SIZE(m_channels),
};
