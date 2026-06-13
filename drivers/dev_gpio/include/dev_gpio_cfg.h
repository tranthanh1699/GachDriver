#ifndef DEV_GPIO_CFG_H
#define DEV_GPIO_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"

#define DEV_GPIO_CFG_MAX_PINS              (32U)
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)
#define DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED (1U)

/* Logical pin IDs — dense, 0..DEV_GPIO_CFG_PIN_COUNT-1 */
#define DEV_GPIO_LED_STATUS       ((dev_gpio_pin_t)0U)
#define DEV_GPIO_BUTTON_USER      ((dev_gpio_pin_t)1U)
#define DEV_GPIO_CFG_PIN_COUNT    (2U)

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_CFG_H */
