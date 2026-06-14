#ifndef DEV_GPIO_CFG_H
#define DEV_GPIO_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "stm32h7xx_hal.h"

/* ── Global configuration ── */

#define DEV_GPIO_CFG_MAX_PINS              (32U)
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)
#define DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED (1U)

/* ── X-Macro: pin list ──
 *
 * Format: X(NAME, PORT, PIN, MODE, PULL)
 *
 * Add a new pin by adding one line here. Enum, map, and count are
 * generated automatically — no other file needs to change.
 */
#define DEV_GPIO_PIN_LIST(X)                                                     \
    X(LED_STATUS,  GPIOB, GPIO_PIN_0,  DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE) \
    X(BUTTON_USER, GPIOC, GPIO_PIN_13, DEV_GPIO_MODE_INPUT,  DEV_GPIO_PULL_UP)

/* ── Auto-generated logical pin IDs ── */

typedef enum {
#define DEV_GPIO_DECLARE_PIN_ID(name, port, pin, mode, pull) \
    DEV_GPIO_##name,

    DEV_GPIO_PIN_LIST(DEV_GPIO_DECLARE_PIN_ID)

#undef DEV_GPIO_DECLARE_PIN_ID

    DEV_GPIO_CFG_PIN_COUNT
} dev_gpio_logical_pin_id_t;

/* ── STM32 hardware pin descriptor ── */

typedef struct {
    dev_gpio_pin_t  logical_id;
    GPIO_TypeDef   *port;
    uint16_t        pin;
    dev_gpio_mode_t mode;
    dev_gpio_pull_t pull;
} dev_gpio_hw_pin_t;

/* ── Map accessors (implemented in dev_gpio_cfg.c) ── */

const dev_gpio_hw_pin_t * DevGpio_GetHwMap(void);
uint16_t                  DevGpio_GetPinCount(void);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_CFG_H */
