#ifndef DEV_GPIO_CFG_H
#define DEV_GPIO_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"

/* ── Global config ── */

#define DEV_GPIO_CFG_MAX_PINS              (32U)
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     DEV_ON
#define DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED DEV_ON

/*
 * X-Macro: define ALL pins here — ONE line per pin.
 *
 * Format: X(NAME, PORT, PIN, MODE, PULL)
 *
 * The enum below extracts only NAME via variadic macro (...).
 * Hardware fields (PORT, PIN, MODE, PULL) are consumed by the
 * port layer's s_gpio_map[] builder.
 *
 * To add a pin: add one X(...) line. Enum, count, and map update automatically.
 */
#define DEV_GPIO_PIN_LIST(X)                                                     \
    X(LED_GREEN, GPIOB, GPIO_PIN_0, DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE)    \
    X(LED_RED,   GPIOB, GPIO_PIN_1, DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE)

/* ── Pin mode ── */

typedef enum {
    DEV_GPIO_MODE_INPUT = 0,
    DEV_GPIO_MODE_OUTPUT,
    DEV_GPIO_MODE_INPUT_PULLUP,
    DEV_GPIO_MODE_INPUT_PULLDOWN
} dev_gpio_mode_t;

/* ── Auto-generated pin ID enum ──
 *
 * DEV_GPIO_DECLARE_ID(name, ...) extracts only 'name'.
 * The hardware fields are ignored here — they're used by the port.
 */

typedef enum {
#define DEV_GPIO_DECLARE_ID(name, ...) \
    DEV_GPIO_##name,

    DEV_GPIO_PIN_LIST(DEV_GPIO_DECLARE_ID)

#undef DEV_GPIO_DECLARE_ID

    DEV_GPIO_CFG_PIN_COUNT
} dev_gpio_logical_pin_id_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_CFG_H */
