#ifndef DEV_GPIO_PORT_STM32_H
#define DEV_GPIO_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"
#include "dev_gpio_cfg.h"
#include "stm32h7xx_hal.h"

/*
 * X-Macro pin list — ONE line per pin.
 * Format: X(NAME, PORT, PIN, MODE, PULL)
 *
 * Automatically generates s_gpio_map[] in the .c file.
 * Pin IDs (DEV_GPIO_LED_GREEN etc.) are defined in dev_gpio_cfg.h.
 */
#define DEV_GPIO_PIN_LIST(X)                                                     \
    X(LED_GREEN, GPIOB, GPIO_PIN_0, DEV_GPIO_PORT_MODE_OUTPUT, DEV_GPIO_PULL_NONE) \
    X(LED_RED,   GPIOB, GPIO_PIN_1, DEV_GPIO_PORT_MODE_OUTPUT, DEV_GPIO_PULL_NONE)

/* Pin mode enum — used internally by the port */
typedef enum {
    DEV_GPIO_PORT_MODE_INPUT = 0,
    DEV_GPIO_PORT_MODE_OUTPUT,
    DEV_GPIO_PORT_MODE_INPUT_PULLUP,
    DEV_GPIO_PORT_MODE_INPUT_PULLDOWN
} dev_gpio_port_mode_t;

/* STM32 hardware pin descriptor */
typedef struct {
    dev_gpio_pin_t       logical_id;
    GPIO_TypeDef        *port;
    uint16_t             hal_pin;
    dev_gpio_port_mode_t mode;
    dev_gpio_pull_t      pull;
} dev_gpio_hw_pin_t;

const dev_gpio_hw_pin_t * dev_gpio_port_get_hw_map(void);
uint16_t                  dev_gpio_port_get_pin_count(void);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_STM32_H */
