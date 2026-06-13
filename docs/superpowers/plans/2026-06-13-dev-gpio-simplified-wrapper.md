# dev_gpio Simplified Wrapper — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rewrite dev_gpio from configuration-heavy driver to a simplified wrapper API (17 functions, no board folders, pin map in port).

**Architecture:** Thin common wrapper (lifecycle + callback table + error mapping + dispatch) → port layer (pin mapping + vendor HAL). dev_common unchanged.

**Tech Stack:** C11, GCC/Clang on Linux host for mock testing, arm-none-eabi-gcc for STM32 target.

---

### Task 1: Remove old code

**Files:**
- Delete: `boards/board_mock/` (entire directory)
- Delete: `boards/board_stm32h743/` (entire directory)
- Delete: `drivers/dev_gpio/include/dev_gpio.h`
- Delete: `drivers/dev_gpio/include/dev_gpio_types.h`
- Delete: `drivers/dev_gpio/include/dev_gpio_cfg.h`
- Delete: `drivers/dev_gpio/include/dev_gpio_port.h`
- Delete: `drivers/dev_gpio/src/dev_gpio.c`
- Delete: `drivers/dev_gpio/port/mock/dev_gpio_port_mock.c`
- Delete: `drivers/dev_gpio/port/mock/dev_gpio_port_mock.h`
- Delete: `drivers/dev_gpio/port/stm32/dev_gpio_port_stm32.c`
- Delete: `drivers/dev_gpio/port/stm32/dev_gpio_port_stm32.h`
- Delete: `tests/dev_gpio/test_gpio.c`
- Delete: `tests/dev_gpio/CMakeLists.txt`
- Delete: `docs/dev_gpio_library.md`

- [ ] **Step 1: Remove all old files**

```bash
cd /home/victor/Desktop/Work/Personal/GachDriver
rm -rf boards/board_mock boards/board_stm32h743
rm -f drivers/dev_gpio/include/dev_gpio.h
rm -f drivers/dev_gpio/include/dev_gpio_types.h
rm -f drivers/dev_gpio/include/dev_gpio_cfg.h
rm -f drivers/dev_gpio/include/dev_gpio_port.h
rm -f drivers/dev_gpio/src/dev_gpio.c
rm -rf drivers/dev_gpio/port/mock
rm -rf drivers/dev_gpio/port/stm32
rm -f tests/dev_gpio/test_gpio.c tests/dev_gpio/CMakeLists.txt
rm -f docs/dev_gpio_library.md
```

- [ ] **Step 2: Commit**

```bash
git add -A && git commit -m "chore: remove old dev_gpio, board folders, and tests

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 2: dev_gpio_types.h + dev_gpio_cfg.h

**Files:**
- Create: `drivers/dev_gpio/include/dev_gpio_types.h`
- Create: `drivers/dev_gpio/include/dev_gpio_cfg.h`

- [ ] **Step 1: Write dev_gpio_types.h**

```c
#ifndef DEV_GPIO_TYPES_H
#define DEV_GPIO_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef uint16_t dev_gpio_pin_t;

typedef enum {
    DEV_GPIO_LEVEL_LOW = 0,
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
```

- [ ] **Step 2: Write dev_gpio_cfg.h**

```c
#ifndef DEV_GPIO_CFG_H
#define DEV_GPIO_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

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
```

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_gpio/include/dev_gpio_types.h drivers/dev_gpio/include/dev_gpio_cfg.h
git commit -m "feat: add dev_gpio_types.h and dev_gpio_cfg.h for simplified wrapper

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 3: dev_gpio_port.h (port interface)

**Files:**
- Create: `drivers/dev_gpio/include/dev_gpio_port.h`

- [ ] **Step 1: Write dev_gpio_port.h**

```c
#ifndef DEV_GPIO_PORT_H
#define DEV_GPIO_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "dev_gpio_cfg.h"
#include "dev_error.h"

/* ── Port-implemented functions (called by common driver) ── */

dev_err_t dev_gpio_port_init(void);
dev_err_t dev_gpio_port_deinit(void);
dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull);
dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t initial_level);
dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level);
dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t level);
dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin);
dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull);
dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr);
dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin);
dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin);

/* ── Common-driver service (called by port ISR handlers) ── */

/* PORT-ONLY: called from vendor ISR handlers. Do not call from application code. */
void dev_gpio_dispatch_isr(dev_gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_H */
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_gpio/include/dev_gpio_port.h
git commit -m "feat: add simplified dev_gpio_port.h port interface

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 4: dev_gpio.h (public API)

**Files:**
- Create: `drivers/dev_gpio/include/dev_gpio.h`

- [ ] **Step 1: Write dev_gpio.h**

```c
#ifndef DEV_GPIO_H
#define DEV_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "dev_gpio_cfg.h"
#include "dev_error.h"

/**
 * @brief Initialize the GPIO wrapper and port layer.
 *
 * Calls dev_gpio_port_init() to enable clocks and prepare hardware.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_ALREADY_INITIALIZED if already initialized.
 * @return DEV_ERR_HW_FAILURE if port initialization fails.
 *
 * @note Not reentrant. Not ISR-safe.
 */
dev_err_t dev_gpio_init(void);

/**
 * @brief De-initialize the GPIO wrapper.
 *
 * Clears all callbacks, disables interrupts, deinitializes the port.
 * Module is forced to UNINITIALIZED regardless of port errors.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_HW_FAILURE if port deinit fails.
 *
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_deinit(void);

/**
 * @brief Check if GPIO wrapper is initialized.
 *
 * @return true if initialized.
 *
 * @note ISR-safe. Reentrant.
 */
bool dev_gpio_is_initialized(void);

/**
 * @brief Configure pin as input without pull.
 *
 * @param pin Logical pin ID (0..DEV_GPIO_CFG_PIN_COUNT-1).
 * @return DEV_OK, DEV_ERR_NOT_INITIALIZED, DEV_ERR_INVALID_ARG, DEV_ERR_HW_FAILURE.
 */
dev_err_t dev_gpio_input(dev_gpio_pin_t pin);

/**
 * @brief Configure pin as input with pull-up.
 */
dev_err_t dev_gpio_input_pullup(dev_gpio_pin_t pin);

/**
 * @brief Configure pin as input with pull-down.
 */
dev_err_t dev_gpio_input_pulldown(dev_gpio_pin_t pin);

/**
 * @brief Configure pin as output (default LOW).
 */
dev_err_t dev_gpio_output(dev_gpio_pin_t pin);

/**
 * @brief Configure pin as output with specified initial level.
 *
 * Avoids output glitches where supported by hardware.
 *
 * @param pin Logical pin ID.
 * @param level Initial output level.
 * @return DEV_ERR_INVALID_ARG if level is not LOW or HIGH.
 */
dev_err_t dev_gpio_output_level(dev_gpio_pin_t pin, dev_gpio_level_t level);

/**
 * @brief Read the logic level of a pin.
 *
 * Reads into a local temporary; *level is written only on success.
 *
 * @param pin Logical pin ID.
 * @param level Output pointer (must not be NULL).
 * @return DEV_ERR_NULL_PTR if level is NULL.
 */
dev_err_t dev_gpio_read(dev_gpio_pin_t pin, dev_gpio_level_t *level);

/**
 * @brief Write a logic level to a pin.
 *
 * @return DEV_ERR_INVALID_ARG if level is not LOW or HIGH.
 */
dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level);

/**
 * @brief Toggle the output level of a pin.
 */
dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin);

/**
 * @brief Set pull mode of a pin at runtime.
 */
dev_err_t dev_gpio_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull);

/**
 * @brief Write HIGH to a pin (convenience).
 */
dev_err_t dev_gpio_high(dev_gpio_pin_t pin);

/**
 * @brief Write LOW to a pin (convenience).
 */
dev_err_t dev_gpio_low(dev_gpio_pin_t pin);

/**
 * @brief Configure interrupt mode and register callback.
 *
 * @param pin Logical pin ID.
 * @param intr Interrupt mode. Use DEV_GPIO_INTR_DISABLE with cb=NULL to clear.
 * @param callback ISR callback. Must not be NULL unless intr is DISABLE.
 * @param user_arg User argument passed to callback.
 *
 * @return DEV_ERR_NULL_PTR if intr != DISABLE and callback is NULL.
 * @return DEV_ERR_NOT_SUPPORTED if intr mode not supported by port.
 * @return DEV_ERR_NOT_SUPPORTED if DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U.
 */
dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t callback, void *user_arg);

/**
 * @brief Enable interrupt for a pin.
 *
 * Requires a callback to have been registered via dev_gpio_interrupt().
 * Marks enabled BEFORE port call; rolls back on port failure.
 *
 * @return DEV_ERR_INVALID_STATE if no callback registered or intr is DISABLE.
 */
dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin);

/**
 * @brief Disable interrupt for a pin.
 *
 * Marks disabled BEFORE port call. Safe to call when already disabled.
 */
dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_H */
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_gpio/include/dev_gpio.h
git commit -m "feat: add simplified dev_gpio.h public API header

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 5: dev_gpio.c (thin wrapper)

**Files:**
- Create: `drivers/dev_gpio/src/dev_gpio.c`

- [ ] **Step 1: Write dev_gpio.c**

```c
#include "dev_gpio.h"
#include "dev_gpio_port.h"
#include "dev_common.h"

/* ── Module state ── */

static bool g_initialized = false;

/* ── Callback table (indexed by pin ID) ── */

typedef struct {
    dev_gpio_callback_t callback;
    void               *user_arg;
    dev_gpio_intr_t     intr;
    bool                enabled;
} dev_gpio_callback_entry_t;

static dev_gpio_callback_entry_t g_callbacks[DEV_GPIO_CFG_MAX_PINS];

/* ── Port error mapping ── */

static dev_err_t dev_gpio_map_port_error(dev_err_t port_err)
{
    if (port_err == DEV_OK)                { return DEV_OK; }
    if (port_err == DEV_ERR_INVALID_ARG)   { return DEV_ERR_INVALID_ARG; }
    if (port_err == DEV_ERR_NULL_PTR)      { return DEV_ERR_NULL_PTR; }
    if (port_err == DEV_ERR_NOT_SUPPORTED) { return DEV_ERR_NOT_SUPPORTED; }
    return DEV_ERR_HW_FAILURE;
}

/* ── Validation helpers ── */

static bool dev_gpio_is_valid_pin(dev_gpio_pin_t pin)
{
    return (pin < DEV_GPIO_CFG_MAX_PINS);
}

static bool dev_gpio_is_valid_level(dev_gpio_level_t level)
{
    return (level == DEV_GPIO_LEVEL_LOW) || (level == DEV_GPIO_LEVEL_HIGH);
}

static bool dev_gpio_is_valid_pull(dev_gpio_pull_t pull)
{
    return (pull == DEV_GPIO_PULL_NONE) ||
           (pull == DEV_GPIO_PULL_UP)   ||
           (pull == DEV_GPIO_PULL_DOWN);
}

/* ── Public API ── */

dev_err_t dev_gpio_init(void)
{
    dev_err_t err;

    if (g_initialized) {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    err = dev_gpio_port_init();
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    g_initialized = true;
    return DEV_OK;
}

dev_err_t dev_gpio_deinit(void)
{
    uint16_t i;
    dev_err_t port_err;
    dev_err_t result = DEV_OK;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Clear all callback entries */
    for (i = 0U; i < DEV_GPIO_CFG_MAX_PINS; i++) {
        g_callbacks[i].callback  = NULL;
        g_callbacks[i].user_arg  = NULL;
        g_callbacks[i].intr      = DEV_GPIO_INTR_DISABLE;
        g_callbacks[i].enabled   = false;
    }

    port_err = dev_gpio_port_deinit();
    g_initialized = false;

    if (port_err != DEV_OK) {
        result = dev_gpio_map_port_error(port_err);
    }

    return result;
}

bool dev_gpio_is_initialized(void)
{
    return g_initialized;
}

dev_err_t dev_gpio_input(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_input(pin, DEV_GPIO_PULL_NONE);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_input_pullup(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_input(pin, DEV_GPIO_PULL_UP);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_input_pulldown(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_input(pin, DEV_GPIO_PULL_DOWN);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_output(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_output(pin, DEV_GPIO_LEVEL_LOW);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_output_level(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (!dev_gpio_is_valid_level(level)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_output(pin, level);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    dev_gpio_level_t temp;
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (level == NULL) { return DEV_ERR_NULL_PTR; }

    err = dev_gpio_port_read(pin, &temp);
    if (err != DEV_OK) { return dev_gpio_map_port_error(err); }

    *level = temp;
    return DEV_OK;
}

dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (!dev_gpio_is_valid_level(level)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_write(pin, level);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_toggle(pin);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (!dev_gpio_is_valid_pull(pull)) { return DEV_ERR_INVALID_ARG; }

    err = dev_gpio_port_set_pull(pin, pull);
    return dev_gpio_map_port_error(err);
}

dev_err_t dev_gpio_high(dev_gpio_pin_t pin)
{
    return dev_gpio_write(pin, DEV_GPIO_LEVEL_HIGH);
}

dev_err_t dev_gpio_low(dev_gpio_pin_t pin)
{
    return dev_gpio_write(pin, DEV_GPIO_LEVEL_LOW);
}

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 1U)

dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t callback, void *user_arg)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    if (intr == DEV_GPIO_INTR_DISABLE) {
        if (callback != NULL) { return DEV_ERR_INVALID_ARG; }

        /* Block dispatch FIRST, then configure hardware */
        g_callbacks[pin].enabled = false;

        err = dev_gpio_port_interrupt(pin, DEV_GPIO_INTR_DISABLE);
        if (err != DEV_OK) {
            /* On failure: enabled=false, intr=DISABLE — safe state */
            g_callbacks[pin].intr = DEV_GPIO_INTR_DISABLE;
            return dev_gpio_map_port_error(err);
        }

        g_callbacks[pin].callback  = NULL;
        g_callbacks[pin].user_arg  = NULL;
        g_callbacks[pin].intr      = DEV_GPIO_INTR_DISABLE;
        return DEV_OK;
    }

    /* intr != DISABLE */
    if (callback == NULL) { return DEV_ERR_NULL_PTR; }

    /* Configure hardware FIRST, store only on success */
    err = dev_gpio_port_interrupt(pin, intr);
    if (err != DEV_OK) { return dev_gpio_map_port_error(err); }

    g_callbacks[pin].callback  = callback;
    g_callbacks[pin].user_arg  = user_arg;
    g_callbacks[pin].intr      = intr;
    g_callbacks[pin].enabled   = false; /* stays disabled until enable */
    return DEV_OK;
}

dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }
    if (g_callbacks[pin].callback == NULL) { return DEV_ERR_INVALID_STATE; }
    if (g_callbacks[pin].intr == DEV_GPIO_INTR_DISABLE) { return DEV_ERR_INVALID_STATE; }

    g_callbacks[pin].enabled = true;

    err = dev_gpio_port_interrupt_enable(pin);
    if (err != DEV_OK) {
        g_callbacks[pin].enabled = false; /* rollback */
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin)
{
    dev_err_t err;

    if (!g_initialized) { return DEV_ERR_NOT_INITIALIZED; }
    if (!dev_gpio_is_valid_pin(pin)) { return DEV_ERR_INVALID_ARG; }

    g_callbacks[pin].enabled = false;

    err = dev_gpio_port_interrupt_disable(pin);
    if (err != DEV_OK) { return dev_gpio_map_port_error(err); }

    return DEV_OK;
}

void dev_gpio_dispatch_isr(dev_gpio_pin_t pin)
{
    dev_gpio_callback_t cb;
    void *arg;

    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return; }
    if (!g_callbacks[pin].enabled)    { return; }
    if (g_callbacks[pin].callback == NULL) { return; }

    cb  = g_callbacks[pin].callback;
    arg = g_callbacks[pin].user_arg;

    if (cb != NULL) {
        cb(pin, arg);
    }
}

#else /* DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U */

dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t callback, void *user_arg)
{
    DEV_UNUSED(pin); DEV_UNUSED(intr);
    DEV_UNUSED(callback); DEV_UNUSED(user_arg);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin)
{
    DEV_UNUSED(pin);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin)
{
    DEV_UNUSED(pin);
    return DEV_ERR_NOT_SUPPORTED;
}

void dev_gpio_dispatch_isr(dev_gpio_pin_t pin)
{
    DEV_UNUSED(pin);
    /* No-op when interrupts are compiled out */
}

#endif /* DEV_GPIO_CFG_INTERRUPT_ENABLED */
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_gpio/src/dev_gpio.c
git commit -m "feat: add simplified dev_gpio.c thin wrapper with callback table

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 6: Mock port

**Files:**
- Create: `drivers/dev_gpio/port/mock/dev_gpio_port_mock.h`
- Create: `drivers/dev_gpio/port/mock/dev_gpio_port_mock.c`

- [ ] **Step 1: Write dev_gpio_port_mock.h**

```c
#ifndef DEV_GPIO_PORT_MOCK_H
#define DEV_GPIO_PORT_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"

/* Error injection */
void dev_gpio_port_mock_set_error(dev_err_t error);
void dev_gpio_port_mock_clear_error(void);

/* ISR simulation */
void dev_gpio_port_mock_trigger_isr(dev_gpio_pin_t pin);

/* State inspection */
dev_gpio_level_t dev_gpio_port_mock_get_level(dev_gpio_pin_t pin);
bool             dev_gpio_port_mock_is_output(dev_gpio_pin_t pin);
bool             dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_MOCK_H */
```

- [ ] **Step 2: Write dev_gpio_port_mock.c**

```c
#include "dev_gpio_port_mock.h"

static dev_gpio_level_t m_levels[DEV_GPIO_CFG_MAX_PINS];
static bool             m_is_output[DEV_GPIO_CFG_MAX_PINS];
static bool             m_interrupt_enabled[DEV_GPIO_CFG_MAX_PINS];
static dev_err_t        m_injected_error = DEV_OK;

/* ── Error injection ── */

void dev_gpio_port_mock_set_error(dev_err_t error)
{
    m_injected_error = error;
}

void dev_gpio_port_mock_clear_error(void)
{
    m_injected_error = DEV_OK;
}

/* ── ISR simulation ── */

void dev_gpio_port_mock_trigger_isr(dev_gpio_pin_t pin)
{
    dev_gpio_dispatch_isr(pin);
}

/* ── State inspection ── */

dev_gpio_level_t dev_gpio_port_mock_get_level(dev_gpio_pin_t pin)
{
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_GPIO_LEVEL_LOW; }
    return m_levels[pin];
}

bool dev_gpio_port_mock_is_output(dev_gpio_pin_t pin)
{
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return false; }
    return m_is_output[pin];
}

bool dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_pin_t pin)
{
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return false; }
    return m_interrupt_enabled[pin];
}

/* ── Port API ── */

dev_err_t dev_gpio_port_init(void)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    DEV_UNUSED(pull);
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_is_output[pin] = false;
    m_levels[pin] = DEV_GPIO_LEVEL_LOW;
    return DEV_OK;
}

dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t initial_level)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_is_output[pin] = true;
    m_levels[pin] = initial_level;
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }
    if (level == NULL) { return DEV_ERR_NULL_PTR; }

    *level = m_levels[pin];
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_levels[pin] = level;
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    if (m_levels[pin] == DEV_GPIO_LEVEL_LOW) {
        m_levels[pin] = DEV_GPIO_LEVEL_HIGH;
    } else {
        m_levels[pin] = DEV_GPIO_LEVEL_LOW;
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    DEV_UNUSED(pull);
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    /* Simulate unsupported interrupt modes for negative testing */
    if ((intr == DEV_GPIO_INTR_BOTH_EDGES) ||
        (intr == DEV_GPIO_INTR_LOW_LEVEL) ||
        (intr == DEV_GPIO_INTR_HIGH_LEVEL)) {
        return DEV_ERR_NOT_SUPPORTED;
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_interrupt_enabled[pin] = true;
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin)
{
    if (m_injected_error != DEV_OK) { return m_injected_error; }
    if (pin >= DEV_GPIO_CFG_MAX_PINS) { return DEV_ERR_INVALID_ARG; }

    m_interrupt_enabled[pin] = false;
    return DEV_OK;
}
```

- [ ] **Step 3: Commit**

```bash
mkdir -p drivers/dev_gpio/port/mock
git add drivers/dev_gpio/port/mock/
git commit -m "feat: add simplified mock GPIO port

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 7: STM32 port (rewritten with pin map in .c)

**Files:**
- Create: `drivers/dev_gpio/port/stm32/dev_gpio_port_stm32.h`
- Create: `drivers/dev_gpio/port/stm32/dev_gpio_port_stm32.c`

- [ ] **Step 1: Write dev_gpio_port_stm32.h**

```c
#ifndef DEV_GPIO_PORT_STM32_H
#define DEV_GPIO_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"
#include "stm32h7xx_hal.h"

/* STM32-specific pin mapping type */
typedef struct {
    dev_gpio_pin_t pin_id;
    GPIO_TypeDef  *port;
    uint16_t       hal_pin;
} dev_gpio_hw_pin_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_STM32_H */
```

- [ ] **Step 2: Write dev_gpio_port_stm32.c**

```c
#include "dev_gpio_port_stm32.h"
#include "dev_gpio_cfg.h"
#include "dev_compiler.h"

/* ── Pin mapping table (designated initializers by pin ID) ── */

#define STM32_PORT_PIN_COUNT  DEV_GPIO_CFG_PIN_COUNT

static const dev_gpio_hw_pin_t s_gpio_map[STM32_PORT_PIN_COUNT] = {
    [DEV_GPIO_LED_STATUS]  = { DEV_GPIO_LED_STATUS,  GPIOB, GPIO_PIN_0 },
    [DEV_GPIO_BUTTON_USER] = { DEV_GPIO_BUTTON_USER, GPIOC, GPIO_PIN_13 },
};

static bool s_pin_valid(dev_gpio_pin_t pin)
{
    if (pin >= STM32_PORT_PIN_COUNT) { return false; }
    return (s_gpio_map[pin].port != NULL);
}

/* ── Clock helper ── */

static void stm32_enable_clocks(void)
{
    uint16_t i;
    for (i = 0U; i < STM32_PORT_PIN_COUNT; i++) {
        if (!s_pin_valid((dev_gpio_pin_t)i)) { continue; }
        if (s_gpio_map[i].port == GPIOA) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
        else if (s_gpio_map[i].port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
        else if (s_gpio_map[i].port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
    }
}

static GPIO_PinState stm32_map_level(dev_gpio_level_t level)
{
    return (level == DEV_GPIO_LEVEL_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

static dev_gpio_level_t stm32_map_pin_state(GPIO_PinState state)
{
    return (state == GPIO_PIN_SET) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW;
}

/* ── Port API ── */

dev_err_t dev_gpio_port_init(void)
{
    stm32_enable_clocks();
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    uint16_t i;
    for (i = 0U; i < STM32_PORT_PIN_COUNT; i++) {
        if (s_pin_valid((dev_gpio_pin_t)i)) {
            HAL_GPIO_DeInit(s_gpio_map[i].port, s_gpio_map[i].hal_pin);
        }
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    GPIO_InitTypeDef init = {0};

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    init.Pin   = s_gpio_map[pin].hal_pin;
    init.Mode  = GPIO_MODE_INPUT;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (pull) {
    case DEV_GPIO_PULL_NONE: init.Pull = GPIO_NOPULL;   break;
    case DEV_GPIO_PULL_UP:   init.Pull = GPIO_PULLUP;   break;
    case DEV_GPIO_PULL_DOWN: init.Pull = GPIO_PULLDOWN; break;
    default: return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_Init(s_gpio_map[pin].port, &init);
    return DEV_OK;
}

dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t initial_level)
{
    GPIO_InitTypeDef init = {0};

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    init.Pin   = s_gpio_map[pin].hal_pin;
    init.Mode  = GPIO_MODE_OUTPUT_PP;
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(s_gpio_map[pin].port, &init);
    HAL_GPIO_WritePin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin,
                      stm32_map_level(initial_level));
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    if (level == NULL) { return DEV_ERR_NULL_PTR; }

    *level = stm32_map_pin_state(
        HAL_GPIO_ReadPin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin));
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    HAL_GPIO_WritePin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin,
                      stm32_map_level(level));
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    HAL_GPIO_TogglePin(s_gpio_map[pin].port, s_gpio_map[pin].hal_pin);
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    GPIO_InitTypeDef init = {0};

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    init.Pin   = s_gpio_map[pin].hal_pin;
    init.Mode  = GPIO_MODE_INPUT;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (pull) {
    case DEV_GPIO_PULL_NONE: init.Pull = GPIO_NOPULL;   break;
    case DEV_GPIO_PULL_UP:   init.Pull = GPIO_PULLUP;   break;
    case DEV_GPIO_PULL_DOWN: init.Pull = GPIO_PULLDOWN; break;
    default: return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_Init(s_gpio_map[pin].port, &init);
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr)
{
    GPIO_InitTypeDef init = {0};

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    init.Pin   = s_gpio_map[pin].hal_pin;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (intr) {
    case DEV_GPIO_INTR_DISABLE:      init.Mode = GPIO_MODE_INPUT;           break;
    case DEV_GPIO_INTR_RISING_EDGE:  init.Mode = GPIO_MODE_IT_RISING;       break;
    case DEV_GPIO_INTR_FALLING_EDGE: init.Mode = GPIO_MODE_IT_FALLING;      break;
    case DEV_GPIO_INTR_BOTH_EDGES:   init.Mode = GPIO_MODE_IT_RISING_FALLING; break;
    default: return DEV_ERR_NOT_SUPPORTED;
    }

    init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(s_gpio_map[pin].port, &init);
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin)
{
    IRQn_Type irq;

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    switch (s_gpio_map[pin].hal_pin) {
    case GPIO_PIN_0:  irq = EXTI0_IRQn;  break;
    case GPIO_PIN_13: irq = EXTI15_10_IRQn; break;
    default: return DEV_ERR_NOT_SUPPORTED;
    }

    HAL_NVIC_SetPriority(irq, 0x0FU, 0x0FU);
    HAL_NVIC_EnableIRQ(irq);
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin)
{
    IRQn_Type irq;

    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }

    switch (s_gpio_map[pin].hal_pin) {
    case GPIO_PIN_0:  irq = EXTI0_IRQn;  break;
    case GPIO_PIN_13: irq = EXTI15_10_IRQn; break;
    default: return DEV_ERR_NOT_SUPPORTED;
    }

    HAL_NVIC_DisableIRQ(irq);
    return DEV_OK;
}

/* ── EXTI ISR dispatch ── */

void HAL_GPIO_EXTI_Callback(uint16_t hal_pin)
{
    dev_gpio_pin_t pin;

    switch (hal_pin) {
    case GPIO_PIN_0:  pin = DEV_GPIO_LED_STATUS;  break;
    case GPIO_PIN_13: pin = DEV_GPIO_BUTTON_USER; break;
    default: return;
    }

    dev_gpio_dispatch_isr(pin);
}
```

- [ ] **Step 3: Commit**

```bash
mkdir -p drivers/dev_gpio/port/stm32
git add drivers/dev_gpio/port/stm32/
git commit -m "feat: add simplified STM32 GPIO port with pin map in .c

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 8: ESP32 port (stub)

**Files:**
- Create: `drivers/dev_gpio/port/esp32/dev_gpio_port_esp32.h`
- Create: `drivers/dev_gpio/port/esp32/dev_gpio_port_esp32.c`

- [ ] **Step 1: Write dev_gpio_port_esp32.h**

```c
#ifndef DEV_GPIO_PORT_ESP32_H
#define DEV_GPIO_PORT_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"

/* ESP32-specific pin mapping type */
typedef struct {
    dev_gpio_pin_t pin_id;
    int            gpio_num;
} dev_gpio_hw_pin_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_ESP32_H */
```

- [ ] **Step 2: Write dev_gpio_port_esp32.c**

```c
#include "dev_gpio_port_esp32.h"
#include "dev_gpio_cfg.h"
#include "dev_compiler.h"

/* ── Pin mapping table ── */

static const dev_gpio_hw_pin_t s_gpio_map[DEV_GPIO_CFG_PIN_COUNT] = {
    [DEV_GPIO_LED_STATUS]  = { DEV_GPIO_LED_STATUS,  2 },
    [DEV_GPIO_BUTTON_USER] = { DEV_GPIO_BUTTON_USER, 0 },
};

static bool s_pin_valid(dev_gpio_pin_t pin)
{
    return (pin < DEV_GPIO_CFG_PIN_COUNT);
}

/* ── Port API — stubs ── */

dev_err_t dev_gpio_port_init(void)
{
    /* ESP32 does not require GPIO peripheral clock enable. */
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    uint16_t i;
    for (i = 0U; i < DEV_GPIO_CFG_PIN_COUNT; i++) {
        /* gpio_reset_pin(s_gpio_map[i].gpio_num); */
        DEV_UNUSED(s_gpio_map[i].gpio_num);
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    DEV_UNUSED(pull);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    /* ESP-IDF: gpio_set_direction(s_gpio_map[pin].gpio_num, GPIO_MODE_INPUT); */
    /* gpio_set_pull_mode(s_gpio_map[pin].gpio_num, ...); */
    return DEV_OK;
}

dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t initial_level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    /* ESP-IDF: gpio_set_direction(s_gpio_map[pin].gpio_num, GPIO_MODE_OUTPUT); */
    /* gpio_set_level(s_gpio_map[pin].gpio_num, initial_level); */
    DEV_UNUSED(initial_level);
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    if (level == NULL) { return DEV_ERR_NULL_PTR; }
    /* *level = gpio_get_level(s_gpio_map[pin].gpio_num) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW; */
    *level = DEV_GPIO_LEVEL_LOW;
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    /* gpio_set_level(s_gpio_map[pin].gpio_num, level); */
    DEV_UNUSED(level);
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    /* int cur = gpio_get_level(s_gpio_map[pin].gpio_num); */
    /* gpio_set_level(s_gpio_map[pin].gpio_num, !cur); */
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    DEV_UNUSED(pull);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr)
{
    DEV_UNUSED(intr);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    if (intr == DEV_GPIO_INTR_LOW_LEVEL || intr == DEV_GPIO_INTR_HIGH_LEVEL) {
        return DEV_ERR_NOT_SUPPORTED;
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}
```

- [ ] **Step 3: Commit**

```bash
mkdir -p drivers/dev_gpio/port/esp32
git add drivers/dev_gpio/port/esp32/
git commit -m "feat: add ESP32 GPIO port stub

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 9: nRF52 port (stub)

**Files:**
- Create: `drivers/dev_gpio/port/nrf52/dev_gpio_port_nrf52.h`
- Create: `drivers/dev_gpio/port/nrf52/dev_gpio_port_nrf52.c`

- [ ] **Step 1: Write dev_gpio_port_nrf52.h**

```c
#ifndef DEV_GPIO_PORT_NRF52_H
#define DEV_GPIO_PORT_NRF52_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"

/* nRF52-specific pin mapping type */
typedef struct {
    dev_gpio_pin_t pin_id;
    uint32_t       pin_number;
} dev_gpio_hw_pin_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_NRF52_H */
```

- [ ] **Step 2: Write dev_gpio_port_nrf52.c**

```c
#include "dev_gpio_port_nrf52.h"
#include "dev_gpio_cfg.h"
#include "dev_compiler.h"

/* ── Pin mapping table ── */

static const dev_gpio_hw_pin_t s_gpio_map[DEV_GPIO_CFG_PIN_COUNT] = {
    [DEV_GPIO_LED_STATUS]  = { DEV_GPIO_LED_STATUS,  17U },
    [DEV_GPIO_BUTTON_USER] = { DEV_GPIO_BUTTON_USER, 13U },
};

static bool s_pin_valid(dev_gpio_pin_t pin)
{
    return (pin < DEV_GPIO_CFG_PIN_COUNT);
}

/* ── Port API — stubs ── */

dev_err_t dev_gpio_port_init(void)
{
    /* nRF52: no GPIO peripheral clock enable needed. */
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    uint16_t i;
    for (i = 0U; i < DEV_GPIO_CFG_PIN_COUNT; i++) {
        /* nrf_gpio_cfg_default(s_gpio_map[i].pin_number); */
        DEV_UNUSED(s_gpio_map[i].pin_number);
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_input(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    /* nrf_gpio_cfg_input(s_gpio_map[pin].pin_number, map_pull(pull)); */
    DEV_UNUSED(pull);
    return DEV_OK;
}

dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t initial_level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    /* nrf_gpio_cfg_output(s_gpio_map[pin].pin_number); */
    /* nrf_gpio_pin_write(s_gpio_map[pin].pin_number, initial_level); */
    DEV_UNUSED(initial_level);
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_pin_t pin, dev_gpio_level_t *level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    if (level == NULL) { return DEV_ERR_NULL_PTR; }
    /* uint32_t val = nrf_gpio_pin_read(s_gpio_map[pin].pin_number); */
    /* *level = (val != 0U) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW; */
    *level = DEV_GPIO_LEVEL_LOW;
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    /* nrf_gpio_pin_write(s_gpio_map[pin].pin_number, (uint32_t)level); */
    DEV_UNUSED(level);
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    /* nrf_gpio_pin_toggle(s_gpio_map[pin].pin_number); */
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull)
{
    DEV_UNUSED(pull);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr)
{
    DEV_UNUSED(intr);
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    if (intr == DEV_GPIO_INTR_LOW_LEVEL || intr == DEV_GPIO_INTR_HIGH_LEVEL) {
        return DEV_ERR_NOT_SUPPORTED;
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_enable(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}

dev_err_t dev_gpio_port_interrupt_disable(dev_gpio_pin_t pin)
{
    if (!s_pin_valid(pin)) { return DEV_ERR_INVALID_ARG; }
    return DEV_OK;
}
```

- [ ] **Step 3: Commit**

```bash
mkdir -p drivers/dev_gpio/port/nrf52
git add drivers/dev_gpio/port/nrf52/
git commit -m "feat: add nRF52 GPIO port stub

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 10: Tests (34 tests) + CMakeLists.txt

**Files:**
- Create: `tests/dev_gpio/CMakeLists.txt`
- Create: `tests/dev_gpio/test_gpio.c`

- [ ] **Step 1: Write CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(gpio_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)

add_executable(gpio_test_host
    ${PROJECT_ROOT}/drivers/dev_common/src/dev_common.c
    ${PROJECT_ROOT}/drivers/dev_common/src/dev_assert.c
    ${PROJECT_ROOT}/drivers/dev_gpio/src/dev_gpio.c
    ${PROJECT_ROOT}/drivers/dev_gpio/port/mock/dev_gpio_port_mock.c
    test_gpio.c
)

target_include_directories(gpio_test_host PRIVATE
    ${PROJECT_ROOT}/drivers/dev_common/include
    ${PROJECT_ROOT}/drivers/dev_gpio/include
    ${PROJECT_ROOT}/drivers/dev_gpio/port/mock
)

target_compile_options(gpio_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
```

- [ ] **Step 2: Write test_gpio.c**

```c
#include <stdio.h>
#include "dev_gpio.h"
#include "dev_gpio_port_mock.h"
#include "dev_assert.h"

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)

#define CHECK(cond, msg) do {                              \
    if (!(cond)) {                                         \
        printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define CHECK_EQ(a, b, msg) do {                           \
    int _a = (int)(a); int _b = (int)(b);                   \
    if (_a != _b) {                                        \
        printf("  FAIL: %s (expected %d, got %d) (%s:%d)\n", \
               msg, _b, _a, __FILE__, __LINE__);            \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define CHECK_EQ_PTR(a, b, msg) do {                       \
    if ((a) != (b)) {                                      \
        printf("  FAIL: %s (%p vs %p) (%s:%d)\n",          \
               msg, (void*)(b), (void*)(a), __FILE__, __LINE__); \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define RUN_TEST(name) do {                                \
    printf("  test_%s...\n", #name);                       \
    dev_gpio_port_mock_clear_error();                       \
    setup();                                                \
    test_##name();                                         \
} while (false)

/* ── Setup ── */

static void setup(void)
{
    if (dev_gpio_is_initialized()) {
        (void)dev_gpio_deinit();
    }
    dev_gpio_port_mock_clear_error();
}

/* ── ISR test helpers ── */

static dev_gpio_pin_t g_last_isr_pin;
static void          *g_last_isr_arg;
static int            g_isr_call_count;

static void test_isr_callback(dev_gpio_pin_t pin, void *user_arg)
{
    g_last_isr_pin  = pin;
    g_last_isr_arg  = user_arg;
    g_isr_call_count++;
}

static void reset_isr_state(void)
{
    g_last_isr_pin  = (dev_gpio_pin_t)0xFFFFU;
    g_last_isr_arg  = NULL;
    g_isr_call_count = 0;
}

/* ── Test 1: init ── */

TEST(1_init_succeeds)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK(dev_gpio_is_initialized(), "is_initialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 2: double init ── */

TEST(2_double_init)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "first init");
    CHECK_EQ(dev_gpio_init(), DEV_ERR_ALREADY_INITIALIZED, "double init");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 3: output then high ── */

TEST(3_output_high)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_high(DEV_GPIO_LED_STATUS), DEV_OK, "high");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_HIGH, "level HIGH");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 4: output then low ── */

TEST(4_output_low)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_low(DEV_GPIO_LED_STATUS), DEV_OK, "low");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_LOW, "level LOW");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 5: output_level HIGH ── */

TEST(5_output_level_high)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output_level(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH), DEV_OK, "output_level");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_HIGH, "level HIGH");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 6: output_level LOW ── */

TEST(6_output_level_low)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output_level(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_LOW), DEV_OK, "output_level");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_LOW, "level LOW");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 7: input ── */

TEST(7_input)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK(!dev_gpio_port_mock_is_output(DEV_GPIO_BUTTON_USER), "is input");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 8: input_pullup ── */

TEST(8_input_pullup)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input_pullup(DEV_GPIO_BUTTON_USER), DEV_OK, "input_pullup");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 9: input_pulldown ── */

TEST(9_input_pulldown)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input_pulldown(DEV_GPIO_BUTTON_USER), DEV_OK, "input_pulldown");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 10: write valid ── */

TEST(10_write_valid)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH), DEV_OK, "write");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_HIGH, "level");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 11: write invalid level ── */

TEST(11_write_invalid_level)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_write(DEV_GPIO_LED_STATUS, (dev_gpio_level_t)99U), DEV_ERR_INVALID_ARG, "invalid level");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 12: read NULL pointer ── */

TEST(12_read_null)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_read(DEV_GPIO_LED_STATUS, NULL), DEV_ERR_NULL_PTR, "null ptr");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 13: read valid ── */

TEST(13_read_valid)
{
    dev_gpio_level_t lvl;
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output_level(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH), DEV_OK, "output_level");
    CHECK_EQ(dev_gpio_read(DEV_GPIO_LED_STATUS, &lvl), DEV_OK, "read");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_HIGH, "level");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 14: toggle ── */

TEST(14_toggle)
{
    dev_gpio_level_t lvl;
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_toggle(DEV_GPIO_LED_STATUS), DEV_OK, "toggle");
    CHECK_EQ(dev_gpio_read(DEV_GPIO_LED_STATUS, &lvl), DEV_OK, "read");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_HIGH, "HIGH after toggle from LOW");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 15: set_pull ── */

TEST(15_set_pull)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_set_pull(DEV_GPIO_BUTTON_USER, DEV_GPIO_PULL_UP), DEV_OK, "set_pull UP");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 16: high convenience ── */

TEST(16_high)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_high(DEV_GPIO_LED_STATUS), DEV_OK, "high");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_HIGH, "HIGH");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 17: low convenience ── */

TEST(17_low)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output_level(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH), DEV_OK, "out_lvl");
    CHECK_EQ(dev_gpio_low(DEV_GPIO_LED_STATUS), DEV_OK, "low");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_LOW, "LOW");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 18: interrupt register ── */

TEST(18_interrupt_register)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, (void*)0x42U), DEV_OK, "interrupt");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 19: interrupt clear with DISABLE + NULL ── */

TEST(19_interrupt_clear)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    /* Register then clear */
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_DISABLE,
                                NULL, NULL), DEV_OK, "clear");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 20: interrupt NULL callback + non-DISABLE → error ── */

TEST(20_interrupt_null_cb_error)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                NULL, NULL), DEV_ERR_NULL_PTR, "null cb");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 21: interrupt enable + trigger ── */

TEST(21_interrupt_enable_trigger)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, (void*)0xDEADU), DEV_OK, "register");
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER), DEV_OK, "enable");

    dev_gpio_port_mock_trigger_isr(DEV_GPIO_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 1, "call count");
    CHECK_EQ(g_last_isr_pin, DEV_GPIO_BUTTON_USER, "correct pin");
    CHECK_EQ_PTR(g_last_isr_arg, (void*)0xDEADU, "correct arg");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 22: interrupt disable ── */

TEST(22_interrupt_disable)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER), DEV_OK, "enable");
    CHECK_EQ(dev_gpio_interrupt_disable(DEV_GPIO_BUTTON_USER), DEV_OK, "disable");

    g_isr_call_count = 0;
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "NOT called when disabled");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 23: deinit ── */

TEST(23_deinit)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_deinit(), DEV_OK, "deinit");
    CHECK(!dev_gpio_is_initialized(), "uninitialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 24: reinit after deinit ── */

TEST(24_reinit)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_deinit(), DEV_OK, "deinit");
    CHECK_EQ(dev_gpio_init(), DEV_OK, "reinit");
    CHECK(dev_gpio_is_initialized(), "initialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 25: pin >= MAX_PINS ── */

TEST(25_pin_out_of_range)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output((dev_gpio_pin_t)DEV_GPIO_CFG_MAX_PINS),
             DEV_ERR_INVALID_ARG, "pin >= MAX");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 26: operation before init ── */

TEST(26_before_init)
{
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_ERR_NOT_INITIALIZED, "not init");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 27: mock error injection ── */

TEST(27_error_injection)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");

    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    CHECK_EQ(dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH),
             DEV_ERR_HW_FAILURE, "injected error");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 28: enable interrupt port fails, rollback ── */

TEST(28_enable_rollback)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");

    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_HW_FAILURE, "enable fails");

    /* ISR should NOT fire because enabled was rolled back */
    g_isr_call_count = 0;
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "NOT called after rollback");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 29: interrupt with INTERRUPT_ENABLED=0U ── */

TEST(29_interrupt_disabled_build)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U)
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL),
             DEV_ERR_NOT_SUPPORTED, "interrupt disabled build");
#endif
    printf("    PASS\n"); g_passes++;
}

/* ── Test 30: enable/disable with INTERRUPT_ENABLED=0U ── */

TEST(30_enable_disable_disabled_build)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U)
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_NOT_SUPPORTED, "enable disabled build");
    CHECK_EQ(dev_gpio_interrupt_disable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_NOT_SUPPORTED, "disable disabled build");
#endif
    printf("    PASS\n"); g_passes++;
}

/* ── Test 31: pin in range but absent from port map ── */

TEST(31_pin_unmapped)
{
    /* Use a pin ID that is < MAX_PINS but >= PIN_COUNT (not in mock map) */
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output((dev_gpio_pin_t)(DEV_GPIO_CFG_PIN_COUNT + 1U)),
             DEV_ERR_INVALID_ARG, "unmapped pin");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 32: deinit clears callbacks ── */

TEST(32_deinit_clears_callbacks)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER), DEV_OK, "enable");

    /* Deinit clears callbacks */
    CHECK_EQ(dev_gpio_deinit(), DEV_OK, "deinit");

    /* Reinit — callbacks gone */
    CHECK_EQ(dev_gpio_init(), DEV_OK, "reinit");
    g_isr_call_count = 0;
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "callback NOT invoked after reinit");

    /* Trying to enable should fail (no callback) */
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_INVALID_STATE, "enable fails without callback");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 33: interrupt_enable without callback ── */

TEST(33_enable_without_callback)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_INVALID_STATE, "enable without callback");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 34: interrupt(DISABLE) port fails, enable blocked ── */

TEST(34_disable_fail_blocks_enable)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");

    /* Inject error so port disable fails */
    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_DISABLE, NULL, NULL),
             DEV_ERR_HW_FAILURE, "disable fails");
    dev_gpio_port_mock_clear_error();

    /* intr is now DISABLE — enable should be blocked */
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_INVALID_STATE, "enable blocked after failed disable");
    printf("    PASS\n"); g_passes++;
}

/* ── Main ── */

int main(void)
{
    dev_assert_config_t assert_cfg = {
        .backend = DEV_ASSERT_BACKEND_NONE,
        .output_hook = NULL, .user_hook = NULL, .reset_hook = NULL,
        .text_buffer = NULL, .text_buffer_size = 0U
    };
    dev_assert_init(&assert_cfg);

    printf("=== GPIO Simplified Wrapper Test Suite ===\n\n");

    RUN_TEST(1_init_succeeds);
    RUN_TEST(2_double_init);
    RUN_TEST(3_output_high);
    RUN_TEST(4_output_low);
    RUN_TEST(5_output_level_high);
    RUN_TEST(6_output_level_low);
    RUN_TEST(7_input);
    RUN_TEST(8_input_pullup);
    RUN_TEST(9_input_pulldown);
    RUN_TEST(10_write_valid);
    RUN_TEST(11_write_invalid_level);
    RUN_TEST(12_read_null);
    RUN_TEST(13_read_valid);
    RUN_TEST(14_toggle);
    RUN_TEST(15_set_pull);
    RUN_TEST(16_high);
    RUN_TEST(17_low);
    RUN_TEST(18_interrupt_register);
    RUN_TEST(19_interrupt_clear);
    RUN_TEST(20_interrupt_null_cb_error);
    RUN_TEST(21_interrupt_enable_trigger);
    RUN_TEST(22_interrupt_disable);
    RUN_TEST(23_deinit);
    RUN_TEST(24_reinit);
    RUN_TEST(25_pin_out_of_range);
    RUN_TEST(26_before_init);
    RUN_TEST(27_error_injection);
    RUN_TEST(28_enable_rollback);
    RUN_TEST(29_interrupt_disabled_build);
    RUN_TEST(30_enable_disable_disabled_build);
    RUN_TEST(31_pin_unmapped);
    RUN_TEST(32_deinit_clears_callbacks);
    RUN_TEST(33_enable_without_callback);
    RUN_TEST(34_disable_fail_blocks_enable);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
```

- [ ] **Step 3: Commit**

```bash
mkdir -p tests/dev_gpio
git add tests/dev_gpio/
git commit -m "feat: add 34-test suite for simplified GPIO wrapper

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 11: Update main.c

**Files:**
- Modify: `Core/Src/main.c`

- [ ] **Step 1: Update includes and init**

Remove old includes and init. Replace with simplified API.

Read the current file, then replace the USER CODE sections:

In `/* USER CODE BEGIN Includes */`:
```c
#include "dev_gpio.h"
#include "dev_common.h"
```

In `/* USER CODE BEGIN 2 */`:
```c
  /* ── dev_gpio simplified wrapper example ── */
  {
      dev_err_t err;

      err = dev_gpio_init();
      if (err != DEV_OK)
      {
          Error_Handler();
      }

      /* Configure PB0 as output (LED) and PB1 as output (second LED) */
      (void)dev_gpio_output(DEV_GPIO_LED_STATUS);
      (void)dev_gpio_output(DEV_GPIO_BUTTON_USER);
  }
```

In `/* USER CODE BEGIN 3 */`:
```c
    {
        (void)dev_gpio_toggle(DEV_GPIO_LED_STATUS);
        (void)dev_gpio_toggle(DEV_GPIO_BUTTON_USER);

        for (volatile uint32_t d = 0U; d < 5000000U; d++) { /* ~500ms */ }
    }
```

- [ ] **Step 2: Commit**

```bash
git add Core/Src/main.c
git commit -m "feat: update main.c for simplified dev_gpio wrapper API

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 12: Update project CMakeLists.txt

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Update sources and includes**

Replace the user sources and includes sections with:

```cmake
# ── GPIO port selection ──
set(DEV_GPIO_PORT "stm32")

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    drivers/dev_common/src/dev_common.c
    drivers/dev_common/src/dev_assert.c
    drivers/dev_gpio/src/dev_gpio.c
    drivers/dev_gpio/port/${DEV_GPIO_PORT}/dev_gpio_port_${DEV_GPIO_PORT}.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    drivers/dev_common/include
    drivers/dev_gpio/include
    drivers/dev_gpio/port/${DEV_GPIO_PORT}
)
```

- [ ] **Step 2: Commit**

```bash
git add CMakeLists.txt
git commit -m "feat: add port-based CMake selection for dev_gpio

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 13: Documentation

**Files:**
- Create: `docs/dev_common/README.md`
- Create: `docs/dev_gpio/README.md`
- Create: `README.md` (root)

- [ ] **Step 1: Create docs/dev_common/README.md** — brief reference

```markdown
# dev_common — Foundation Library

Common types, error codes, assert system, and compiler abstraction used by all dev_* drivers.

## Quick Reference

| File | Purpose |
|------|---------|
| `dev_types.h` | `<stdint.h>`, `<stdbool.h>`, `<stddef.h>` |
| `dev_error.h` | `dev_err_t` enum (DEV_OK=0 through DEV_ERR_CONFIG) |
| `dev_assert.h` | Check macros + 6 configurable backends |
| `dev_compiler.h` | GCC/Clang attribute macros |
| `dev_version.h` | Version 0.1.0 |

## Error Codes

| Code | Meaning |
|------|---------|
| `DEV_OK` (0) | Success |
| `DEV_ERR_FAIL` | Generic failure |
| `DEV_ERR_INVALID_ARG` | Invalid argument |
| `DEV_ERR_NULL_PTR` | NULL pointer |
| `DEV_ERR_INVALID_STATE` | Wrong state for operation |
| `DEV_ERR_NOT_INITIALIZED` | Module not initialized |
| `DEV_ERR_ALREADY_INITIALIZED` | Already initialized |
| `DEV_ERR_NOT_SUPPORTED` | Feature unsupported |
| `DEV_ERR_TIMEOUT` | Operation timed out |
| `DEV_ERR_BUSY` | Resource busy |
| `DEV_ERR_OUT_OF_RANGE` | Value out of range |
| `DEV_ERR_HW_FAILURE` | Hardware/port failure |
| `DEV_ERR_CONFIG` | Configuration error |

## Check Macros

```c
DEV_CHECK_RET(condition, error_code)   // validate + return error on fail
DEV_CHECK_PTR_RET(pointer)             // null check + return DEV_ERR_NULL_PTR
DEV_CHECK_OK_RET(expression)           // call + propagate error
DEV_ASSERT(condition)                  // fatal assert (never returns)
```
```

- [ ] **Step 2: Create docs/dev_gpio/README.md** — full reference

Write the complete dev_gpio documentation covering: architecture, types, configuration, API reference, porting guide with STM32/ESP32/nRF52 examples, callback semantics, build integration, and usage examples. (Content from the spec sections 2-16, condensed into a readable reference.)

- [ ] **Step 3: Create root README.md**

```markdown
# GachDriver — Embedded Driver Abstraction

Hardware-independent driver layer for STM32, ESP32, nRF52, and future MCUs.

## Architecture

Application → dev_* public API → common implementation → port interface → vendor HAL

## Components

| Component | Description | Documentation |
|-----------|-------------|---------------|
| `dev_common` | Types, errors, assert, compiler helpers | [docs/dev_common/README.md](docs/dev_common/README.md) |
| `dev_gpio` | Simplified GPIO wrapper | [docs/dev_gpio/README.md](docs/dev_gpio/README.md) |

## Quick Start

```c
#include "dev_gpio.h"

int main(void) {
    dev_gpio_init();
    dev_gpio_output(DEV_GPIO_LED_STATUS);
    dev_gpio_high(DEV_GPIO_LED_STATUS);
    for (;;) { dev_gpio_toggle(DEV_GPIO_LED_STATUS); }
}
```

## Building

```cmake
set(DEV_GPIO_PORT "stm32")  # or "esp32", "nrf52", "mock"
```

## Porting

Add a port file under `drivers/dev_gpio/port/<target>/` with a pin mapping table.
See [docs/dev_gpio/README.md](docs/dev_gpio/README.md) for the full guide.
```

- [ ] **Step 4: Commit**

```bash
mkdir -p docs/dev_common docs/dev_gpio
git add docs/dev_common/ docs/dev_gpio/ README.md
git commit -m "docs: add dev_common, dev_gpio references and root README

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 14: Build and verify (host mock)

- [ ] **Step 1: Build**

```bash
cd tests/dev_gpio && mkdir -p build && cd build && cmake .. && cmake --build .
```

Expected: 0 errors, 0 warnings.

- [ ] **Step 2: Run tests**

```bash
./gpio_test_host
```

Expected:

```
=== GPIO Simplified Wrapper Test Suite ===
  ...
=== Results: 34 passed, 0 failed ===
```

- [ ] **Step 3: Verify no vendor leaks**

```bash
grep -r 'stm32\|esp_idf\|nrfx' drivers/dev_gpio/include/ drivers/dev_gpio/src/ || echo "No vendor leaks"
```

- [ ] **Step 4: Commit final verification**

```bash
git add -A && git commit -m "verify: all 34 tests pass, no vendor leaks in common layer

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

## Plan Summary

| Task | Files | Description |
|------|-------|-------------|
| 1 | — | Remove old boards, old dev_gpio, old tests |
| 2 | 2 | dev_gpio_types.h + dev_gpio_cfg.h |
| 3 | 1 | dev_gpio_port.h |
| 4 | 1 | dev_gpio.h (public API) |
| 5 | 1 | dev_gpio.c (thin wrapper + callback table) |
| 6 | 2 | Mock port |
| 7 | 2 | STM32 port (pin map in .c) |
| 8 | 2 | ESP32 stub |
| 9 | 2 | nRF52 stub |
| 10 | 2 | 34 tests + CMakeLists.txt |
| 11 | 1 | Update main.c |
| 12 | 1 | Update project CMakeLists.txt |
| 13 | 3 | Documentation |
| 14 | — | Build, run, verify 34 tests pass |
| **Total** | **~20 files** | **14 tasks** |

Key differences from old driver:
- `dev_gpio_init()` takes no arguments
- No `boards/`, no `g_dev_gpio_config`, no `dev_gpio_channel_config_t`
- Pin IDs from `dev_gpio_cfg.h`, pin mapping in port `.c`
- Convenience APIs: `high()`, `low()`, `output()`, `input()`, `input_pullup()`
- Interrupt: one `dev_gpio_interrupt()` call replaces config+register+enable
- Error mapping preserves INVALID_ARG, NULL_PTR, NOT_SUPPORTED; rest → HW_FAILURE
- Callback table tracks `intr` field to prevent re-enabling stale callbacks
