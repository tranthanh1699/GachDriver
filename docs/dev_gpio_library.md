# dev_gpio — Hardware-Independent GPIO Driver

**Version:** 0.1.0  
**Language:** C99/C11  
**Depends on:** `dev_common`  
**MISRA-C oriented:** Yes  
**Available ports:** mock (host testing), STM32H7

---

## 1. Architecture

```
Application (main.c)
   │  #include "dev_gpio.h"
   │  #include "dev_gpio_board_cfg.h"
   │  dev_gpio_init(&g_dev_gpio_config);
   │  dev_gpio_write(DEV_GPIO_CHANNEL_PB0, DEV_GPIO_LEVEL_HIGH);
   ▼
┌─────────────────────────────────────────────┐
│  dev_gpio.c  (common driver)                │
│                                             │
│  • Validates inputs (null, range, state)    │
│  • Manages module lifecycle (init/deinit)   │
│  • Owns callback tables                     │
│  • Owns interrupt enable state              │
│  • Maps port errors consistently            │
│  • Dispatches to port interface             │
└─────────────────────────────────────────────┘
   │  dev_gpio_port_write(channel, level)
   ▼
┌─────────────────────────────────────────────┐
│  dev_gpio_port_<vendor>.c  (port layer)    │
│                                             │
│  • Includes vendor HAL headers (ONLY here)  │
│  • Maps logical channels → hardware pins    │
│  • Translates dev_gpio types → HAL types    │
│  • Calls vendor HAL functions               │
│  • Calls dev_gpio_dispatch_isr() from ISRs  │
└─────────────────────────────────────────────┘
   │  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, ...)
   ▼
          STM32 HAL / ESP-IDF / nrfx / ...
```

**Key principle:** Application code never touches vendor headers. All hardware-specific code lives in one port file. To change MCUs, you swap the port file and board config — application code stays identical.

---

## 2. Quick Start

### 2.1 Minimal Application

```c
#include "dev_gpio.h"
#include "dev_gpio_board_cfg.h"

int main(void)
{
    dev_err_t err;

    /* 1. Initialize the driver with your board config */
    err = dev_gpio_init(&g_dev_gpio_config);
    if (err != DEV_OK) {
        Error_Handler();
    }

    /* 2. Use logical channels — no raw GPIOB, PIN_0 anywhere */
    (void)dev_gpio_write(DEV_GPIO_CHANNEL_PB0, DEV_GPIO_LEVEL_HIGH);

    /* 3. Alternate blink PB0 and PB1 */
    for (;;) {
        (void)dev_gpio_toggle(DEV_GPIO_CHANNEL_PB0);
        (void)dev_gpio_toggle(DEV_GPIO_CHANNEL_PB1);
        for (volatile uint32_t d = 0U; d < 5000000U; d++) { /* delay */ }
    }

    return 0;
}
```

This compiles on STM32H743 with `dev_gpio_port_stm32.c` linked, or on Linux with `dev_gpio_port_mock.c` for testing — no code changes.

---

## 3. Core Types

### 3.1 Channel ID

```c
typedef uint16_t dev_gpio_channel_t;
```

Logical channel IDs are `#define`d in the board config header. They may be sparse — the driver maps them via linear scan, not array index.

### 3.2 Logic Level

```c
typedef enum { DEV_GPIO_LEVEL_LOW = 0, DEV_GPIO_LEVEL_HIGH = 1 } dev_gpio_level_t;
```

### 3.3 Direction

```c
typedef enum {
    DEV_GPIO_DIRECTION_INPUT  = 0,
    DEV_GPIO_DIRECTION_OUTPUT,
    DEV_GPIO_DIRECTION_INPUT_OUTPUT   /* not all ports support this */
} dev_gpio_direction_t;
```

### 3.4 Pull Mode

```c
typedef enum {
    DEV_GPIO_PULL_NONE = 0,
    DEV_GPIO_PULL_UP,
    DEV_GPIO_PULL_DOWN                /* not all ports support this */
} dev_gpio_pull_t;
```

### 3.5 Interrupt Type

```c
typedef enum {
    DEV_GPIO_INTR_DISABLE      = 0,
    DEV_GPIO_INTR_RISING_EDGE,
    DEV_GPIO_INTR_FALLING_EDGE,
    DEV_GPIO_INTR_BOTH_EDGES,         /* not all ports support this */
    DEV_GPIO_INTR_LOW_LEVEL,          /* not all ports support this */
    DEV_GPIO_INTR_HIGH_LEVEL          /* not all ports support this */
} dev_gpio_intr_type_t;
```

### 3.6 ISR Callback

```c
typedef void (*dev_gpio_isr_callback_t)(dev_gpio_channel_t channel, void *user_arg);
```

Executed from ISR context — must not block, must not call non-ISR-safe APIs.

### 3.7 Configuration Structs

```c
typedef struct {
    dev_gpio_channel_t      channel;        /* Logical channel ID */
    dev_gpio_direction_t    direction;      /* INPUT / OUTPUT / INPUT_OUTPUT */
    dev_gpio_pull_t         pull;           /* NONE / UP / DOWN */
    dev_gpio_level_t        default_level;  /* Initial output level */
    dev_gpio_intr_type_t    interrupt;      /* Interrupt mode */
    dev_gpio_isr_callback_t callback;       /* Initial ISR callback (may be NULL) */
    void                   *callback_arg;   /* User argument for callback */
} dev_gpio_channel_config_t;

typedef struct {
    const dev_gpio_channel_config_t *channels;
    uint16_t                         channel_count;
} dev_gpio_config_t;
```

---

## 4. Configuration

`drivers/dev_gpio/include/dev_gpio_cfg.h`:

```c
#define DEV_GPIO_CFG_MAX_CHANNELS          (32U)
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)
#define DEV_GPIO_CFG_VALIDATE_DUPLICATES   (1U)
#define DEV_GPIO_CFG_ENABLE_RUNTIME_CHECKS (1U)
```

| Switch | `1U` | `0U` |
|--------|------|------|
| `INTERRUPT_ENABLED` | Interrupt APIs functional | APIs compile but return `DEV_ERR_NOT_SUPPORTED`; init rejects non-DISABLE interrupts |
| `VALIDATE_DUPLICATES` | Init scans for duplicate channel IDs | Scan skipped |

Set `INTERRUPT_ENABLED` to `0U` on MCUs that don't need interrupts — the interrupt API surface stays identical, avoiding `#ifdef` in application code.

---

## 5. Public API

### 5.1 Lifecycle

```
UNINITIALIZED ──[dev_gpio_init()]──> INITIALIZED
     ^                                  │
     └──────[dev_gpio_deinit()]─────────┘
```

- State becomes INITIALIZED only after ALL channels are configured.
- On init failure: cleanup runs, state stays UNINITIALIZED.
- On deinit: state is FORCED to UNINITIALIZED even if port deinit fails.

### 5.2 Functions

```c
dev_err_t dev_gpio_init(const dev_gpio_config_t *config);
dev_err_t dev_gpio_deinit(void);
dev_err_t dev_gpio_read(dev_gpio_channel_t channel, dev_gpio_level_t *level);
dev_err_t dev_gpio_write(dev_gpio_channel_t channel, dev_gpio_level_t level);
dev_err_t dev_gpio_toggle(dev_gpio_channel_t channel);
dev_err_t dev_gpio_set_direction(dev_gpio_channel_t channel, dev_gpio_direction_t direction);
dev_err_t dev_gpio_set_pull(dev_gpio_channel_t channel, dev_gpio_pull_t pull);
dev_err_t dev_gpio_config_interrupt(dev_gpio_channel_t channel, dev_gpio_intr_type_t interrupt);
dev_err_t dev_gpio_register_callback(dev_gpio_channel_t channel,
                                     dev_gpio_isr_callback_t callback, void *user_arg);
dev_err_t dev_gpio_enable_interrupt(dev_gpio_channel_t channel);
dev_err_t dev_gpio_disable_interrupt(dev_gpio_channel_t channel);
bool     dev_gpio_is_initialized(void);                         /* ISR-safe */
```

### 5.3 Validation and Error Returns

| Condition | Error |
|-----------|-------|
| Config or channels is NULL | `DEV_ERR_NULL_PTR` |
| channel_count is 0 or > MAX_CHANNELS | `DEV_ERR_INVALID_ARG` |
| Invalid direction/pull/interrupt enum | `DEV_ERR_INVALID_ARG` |
| Interrupts compiled out but channel requests interrupt | `DEV_ERR_NOT_SUPPORTED` |
| Duplicate channel IDs | `DEV_ERR_CONFIG` |
| Not initialized (runtime APIs) | `DEV_ERR_NOT_INITIALIZED` |
| Already initialized (init) | `DEV_ERR_ALREADY_INITIALIZED` |
| Write/toggle on input-only channel | `DEV_ERR_INVALID_STATE` |
| Port-level failure | `DEV_ERR_HW_FAILURE` |
| Feature not supported by port | `DEV_ERR_NOT_SUPPORTED` |

### 5.4 Error Mapping

| Port returns | Common driver passes through as |
|-------------|--------------------------------|
| `DEV_OK` | `DEV_OK` |
| `DEV_ERR_NOT_SUPPORTED` | `DEV_ERR_NOT_SUPPORTED` |
| Any other non-OK | `DEV_ERR_HW_FAILURE` |

Application code only needs to check `DEV_OK`, `DEV_ERR_NOT_INITIALIZED`, `DEV_ERR_INVALID_ARG`, `DEV_ERR_INVALID_STATE`, `DEV_ERR_HW_FAILURE`, and `DEV_ERR_NOT_SUPPORTED`.

---

## 6. Board Configuration

### 6.1 Header — `dev_gpio_board_cfg.h`

Define one `#define` per GPIO pin your application uses:

```c
/* boards/board_stm32h743/dev_gpio_board_cfg.h */
#include "dev_gpio.h"

#define DEV_GPIO_CHANNEL_PB0              ((dev_gpio_channel_t)0U)
#define DEV_GPIO_CHANNEL_PB1              ((dev_gpio_channel_t)1U)

extern const dev_gpio_config_t g_dev_gpio_config;
```

Channel numbers are arbitrary — use any `uint16_t` value.

### 6.2 Implementation — `dev_gpio_board_cfg.c`

```c
/* boards/board_stm32h743/dev_gpio_board_cfg.c */
#include "dev_gpio_board_cfg.h"
#include "dev_compiler.h"

static dev_gpio_channel_config_t m_channels[] = {
    /* PB0: output, drives LED */
    {
        .channel       = DEV_GPIO_CHANNEL_PB0,
        .direction     = DEV_GPIO_DIRECTION_OUTPUT,
        .pull          = DEV_GPIO_PULL_NONE,
        .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt     = DEV_GPIO_INTR_DISABLE,
        .callback      = NULL,
        .callback_arg  = NULL,
    },
    /* PB1: output, second LED */
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
```

---

## 7. Port Layer — How to Port to New Hardware

Each hardware target needs ONE port file that implements 11 functions. This section walks through creating a port using the STM32H7 implementation as reference.

### 7.1 Port Interface (what you must implement)

Declared in `drivers/dev_gpio/include/dev_gpio_port.h`:

```c
dev_err_t dev_gpio_port_init(const dev_gpio_config_t *config);
dev_err_t dev_gpio_port_deinit(void);
dev_err_t dev_gpio_port_config_channel(const dev_gpio_channel_config_t *channel_config);
dev_err_t dev_gpio_port_read(dev_gpio_channel_t channel, dev_gpio_level_t *level);
dev_err_t dev_gpio_port_write(dev_gpio_channel_t channel, dev_gpio_level_t level);
dev_err_t dev_gpio_port_toggle(dev_gpio_channel_t channel);
dev_err_t dev_gpio_port_set_direction(dev_gpio_channel_t channel, dev_gpio_direction_t direction);
dev_err_t dev_gpio_port_set_pull(dev_gpio_channel_t channel, dev_gpio_pull_t pull);
dev_err_t dev_gpio_port_config_interrupt(dev_gpio_channel_t channel, dev_gpio_intr_type_t interrupt);
dev_err_t dev_gpio_port_enable_interrupt(dev_gpio_channel_t channel);
dev_err_t dev_gpio_port_disable_interrupt(dev_gpio_channel_t channel);
```

### 7.2 Common-Driver Service (call this from your ISRs)

```c
/* PORT-ONLY: called from vendor ISR handlers. Do not call from application code. */
void dev_gpio_dispatch_isr(dev_gpio_channel_t channel);
```

### 7.3 Step-by-Step Porting Guide

#### Step 1: Create port files

```
drivers/dev_gpio/port/<vendor>/
  dev_gpio_port_<vendor>.h    ← pin mapping type
  dev_gpio_port_<vendor>.c    ← all 11 port functions
```

#### Step 2: Define the pin mapping type (header)

```c
/* dev_gpio_port_stm32.h */
#ifndef DEV_GPIO_PORT_STM32_H
#define DEV_GPIO_PORT_STM32_H

#include "dev_gpio_port.h"
#include "stm32h7xx_hal.h"    /* vendor header — allowed in port layer */

typedef struct {
    dev_gpio_channel_t channel;   /* logical ID from board config */
    GPIO_TypeDef      *port;      /* STM32 GPIO port (GPIOA, GPIOB, ...) */
    uint16_t           pin;       /* STM32 pin mask (GPIO_PIN_0, ...) */
} dev_gpio_port_pin_t;

#endif
```

#### Step 3: Build the pin mapping table

In the `.c` file, create a static const table mapping each logical channel to its hardware pin:

```c
static const dev_gpio_port_pin_t m_pin_map[] = {
    { DEV_GPIO_CHANNEL_PB0, GPIOB, GPIO_PIN_0 },
    { DEV_GPIO_CHANNEL_PB1, GPIOB, GPIO_PIN_1 },
    /* Add entries for each channel your board uses */
};
#define PORT_CHANNEL_COUNT  ((uint16_t)DEV_ARRAY_SIZE(m_pin_map))
```

#### Step 4: Implement channel lookup helper

```c
static uint16_t port_find_index(dev_gpio_channel_t channel)
{
    uint16_t i;
    for (i = 0U; i < PORT_CHANNEL_COUNT; i++) {
        if (m_pin_map[i].channel == channel) { return i; }
    }
    return PORT_CHANNEL_COUNT;  /* not found */
}
```

#### Step 5: Implement level translation helpers

```c
static VendorPinState port_map_level(dev_gpio_level_t level)
{
    return (level == DEV_GPIO_LEVEL_HIGH) ? VENDOR_PIN_SET : VENDOR_PIN_RESET;
}

static dev_gpio_level_t port_map_pin_state(VendorPinState state)
{
    return (state == VENDOR_PIN_SET) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW;
}
```

#### Step 6: Implement `port_init`

1. Enable the GPIO peripheral clock(s)
2. For each entry in `m_pin_map`, find its config in `config->channels[]`
3. Call `dev_gpio_port_config_channel()` for each

```c
dev_err_t dev_gpio_port_init(const dev_gpio_config_t *config)
{
    DEV_UNUSED(config);
    __HAL_RCC_GPIOB_CLK_ENABLE();       /* Enable clocks for your ports */

    for (uint16_t i = 0U; i < PORT_CHANNEL_COUNT; i++) {
        /* Find this pin's config in the common config array */
        const dev_gpio_channel_config_t *ch_cfg = NULL;
        for (uint16_t j = 0U; j < config->channel_count; j++) {
            if (config->channels[j].channel == m_pin_map[i].channel) {
                ch_cfg = &config->channels[j];
                break;
            }
        }
        if (ch_cfg == NULL) { continue; }

        dev_err_t err = dev_gpio_port_config_channel(ch_cfg);
        if (err != DEV_OK) { return err; }
    }
    return DEV_OK;
}
```

#### Step 7: Implement `port_config_channel`

This is the central function — it translates `dev_gpio_channel_config_t` into the vendor's pin initialization struct:

```c
dev_err_t dev_gpio_port_config_channel(const dev_gpio_channel_config_t *ch)
{
    if (ch == NULL) { return DEV_ERR_NULL_PTR; }

    uint16_t idx = port_find_index(ch->channel);
    if (idx >= PORT_CHANNEL_COUNT) { return DEV_ERR_INVALID_ARG; }

    GPIO_InitTypeDef init = {0};
    init.Pin   = m_pin_map[idx].pin;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    /* Map direction */
    switch (ch->direction) {
    case DEV_GPIO_DIRECTION_INPUT:       init.Mode = GPIO_MODE_INPUT;      break;
    case DEV_GPIO_DIRECTION_OUTPUT:      init.Mode = GPIO_MODE_OUTPUT_PP;  break;
    case DEV_GPIO_DIRECTION_INPUT_OUTPUT: return DEV_ERR_NOT_SUPPORTED;
    default:                              return DEV_ERR_INVALID_ARG;
    }

    /* Map pull */
    switch (ch->pull) {
    case DEV_GPIO_PULL_NONE: init.Pull = GPIO_NOPULL;    break;
    case DEV_GPIO_PULL_UP:   init.Pull = GPIO_PULLUP;    break;
    case DEV_GPIO_PULL_DOWN: init.Pull = GPIO_PULLDOWN;  break;
    default:                  return DEV_ERR_INVALID_ARG;
    }

    HAL_GPIO_Init(m_pin_map[idx].port, &init);

    /* Set initial output level */
    if (ch->direction == DEV_GPIO_DIRECTION_OUTPUT) {
        HAL_GPIO_WritePin(m_pin_map[idx].port, m_pin_map[idx].pin,
                          (ch->default_level == DEV_GPIO_LEVEL_HIGH)
                              ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    return DEV_OK;
}
```

#### Step 8: Implement `port_read`, `port_write`, `port_toggle`

These are thin wrappers around vendor HAL calls:

```c
dev_err_t dev_gpio_port_read(dev_gpio_channel_t channel, dev_gpio_level_t *level)
{
    if (level == NULL) { return DEV_ERR_NULL_PTR; }
    uint16_t idx = port_find_index(channel);
    if (idx >= PORT_CHANNEL_COUNT) { return DEV_ERR_INVALID_ARG; }

    GPIO_PinState state = HAL_GPIO_ReadPin(m_pin_map[idx].port, m_pin_map[idx].pin);
    *level = (state == GPIO_PIN_SET) ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW;
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_channel_t channel, dev_gpio_level_t level)
{
    uint16_t idx = port_find_index(channel);
    if (idx >= PORT_CHANNEL_COUNT) { return DEV_ERR_INVALID_ARG; }

    HAL_GPIO_WritePin(m_pin_map[idx].port, m_pin_map[idx].pin,
                      (level == DEV_GPIO_LEVEL_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_channel_t channel)
{
    uint16_t idx = port_find_index(channel);
    if (idx >= PORT_CHANNEL_COUNT) { return DEV_ERR_INVALID_ARG; }

    HAL_GPIO_TogglePin(m_pin_map[idx].port, m_pin_map[idx].pin);
    return DEV_OK;
}
```

#### Step 9: Implement `port_deinit`

```c
dev_err_t dev_gpio_port_deinit(void)
{
    for (uint16_t i = 0U; i < PORT_CHANNEL_COUNT; i++) {
        HAL_GPIO_DeInit(m_pin_map[i].port, m_pin_map[i].pin);
    }
    __HAL_RCC_GPIOB_CLK_DISABLE();
    return DEV_OK;
}
```

#### Step 10: Implement interrupt functions

Map `dev_gpio_intr_type_t` to vendor interrupt modes:

```c
dev_err_t dev_gpio_port_config_interrupt(dev_gpio_channel_t channel,
                                         dev_gpio_intr_type_t interrupt)
{
    uint16_t idx = port_find_index(channel);
    if (idx >= PORT_CHANNEL_COUNT) { return DEV_ERR_INVALID_ARG; }

    uint32_t mode;
    switch (interrupt) {
    case DEV_GPIO_INTR_DISABLE:      mode = GPIO_MODE_INPUT;          break;
    case DEV_GPIO_INTR_RISING_EDGE:  mode = GPIO_MODE_IT_RISING;      break;
    case DEV_GPIO_INTR_FALLING_EDGE: mode = GPIO_MODE_IT_FALLING;     break;
    case DEV_GPIO_INTR_BOTH_EDGES:   mode = GPIO_MODE_IT_RISING_FALLING; break;
    default:                          return DEV_ERR_NOT_SUPPORTED;
    }

    GPIO_InitTypeDef init = {0};
    init.Pin   = m_pin_map[idx].pin;
    init.Mode  = mode;
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(m_pin_map[idx].port, &init);
    return DEV_OK;
}

dev_err_t dev_gpio_port_enable_interrupt(dev_gpio_channel_t channel)
{
    uint16_t idx = port_find_index(channel);
    /* Map pin to EXTI IRQ number and enable via NVIC */
    IRQn_Type irq = pin_to_exti_irq(m_pin_map[idx].pin);
    HAL_NVIC_SetPriority(irq, 0x0FU, 0x0FU);
    HAL_NVIC_EnableIRQ(irq);
    return DEV_OK;
}

dev_err_t dev_gpio_port_disable_interrupt(dev_gpio_channel_t channel)
{
    /* ... */
    HAL_NVIC_DisableIRQ(irq);
    return DEV_OK;
}
```

#### Step 11: Wire up ISR dispatch

In your vendor ISR handler, map the hardware pin back to a logical channel and call `dev_gpio_dispatch_isr()`:

```c
/* Called by STM32 HAL when any EXTI line fires */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    dev_gpio_channel_t channel;
    switch (pin) {
    case GPIO_PIN_0: channel = DEV_GPIO_CHANNEL_PB0; break;
    case GPIO_PIN_1: channel = DEV_GPIO_CHANNEL_PB1; break;
    default: return;
    }
    dev_gpio_dispatch_isr(channel);   /* common driver invokes the callback */
}
```

### 7.4 Porting Checklist

| # | Step | File |
|---|------|------|
| 1 | Define pin mapping type | `dev_gpio_port_<vendor>.h` |
| 2 | Build pin mapping table (channel → port+pin) | `.c` |
| 3 | Implement channel lookup helper | `.c` |
| 4 | Implement level translation helpers | `.c` |
| 5 | Implement `port_init` (enable clocks, configure all pins) | `.c` |
| 6 | Implement `port_config_channel` (translate config → vendor init struct) | `.c` |
| 7 | Implement `port_read` / `port_write` / `port_toggle` | `.c` |
| 8 | Implement `port_set_direction` / `port_set_pull` | `.c` |
| 9 | Implement `port_config_interrupt` (map interrupt modes) | `.c` |
| 10 | Implement `port_enable_interrupt` / `port_disable_interrupt` (NVIC) | `.c` |
| 11 | Implement `port_deinit` | `.c` |
| 12 | Wire up `dev_gpio_dispatch_isr()` in vendor ISR handlers | `.c` |
| 13 | Map vendor errors to `dev_err_t` | `.c` |
| 14 | Return `DEV_ERR_NOT_SUPPORTED` for unsupported features | `.c` |
| 15 | Create board config (channel `#define`s + config struct) | `boards/` |
| 16 | Add sources + include paths to CMake | `CMakeLists.txt` |
| 17 | Test with mock port on host first | `tests/dev_gpio/` |
| 18 | Test on target hardware | — |

### 7.5 Port Rules

- **Callbacks:** Port does NOT own callbacks. It calls `dev_gpio_dispatch_isr()` from ISR handlers.
- **Interrupt gating:** Port does NOT gate callback dispatch. The common driver owns the enable state. Port tracks hardware/simulated state for inspection only.
- **Error mapping:** Return vendor-agnostic `dev_err_t` values. Common driver maps non-OK, non-NOT_SUPPORTED to `DEV_ERR_HW_FAILURE`.
- **Vendor headers:** Allowed in port `.h` and `.c` files — they are inside the port layer, NOT public.

---

## 8. Build Integration

### 8.1 STM32 Target

```cmake
# CMakeLists.txt
target_include_directories(${PROJECT_NAME} PRIVATE
    drivers/dev_common/include
    drivers/dev_gpio/include
    drivers/dev_gpio/port/stm32
    boards/board_stm32h743
)

target_sources(${PROJECT_NAME} PRIVATE
    drivers/dev_common/src/dev_common.c
    drivers/dev_common/src/dev_assert.c
    drivers/dev_gpio/src/dev_gpio.c
    drivers/dev_gpio/port/stm32/dev_gpio_port_stm32.c
    boards/board_stm32h743/dev_gpio_board_cfg.c
)
```

### 8.2 Host Test (Mock Port)

```cmake
# tests/dev_gpio/CMakeLists.txt
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)

add_executable(gpio_test_host
    ${PROJECT_ROOT}/drivers/dev_common/src/dev_common.c
    ${PROJECT_ROOT}/drivers/dev_common/src/dev_assert.c
    ${PROJECT_ROOT}/drivers/dev_gpio/src/dev_gpio.c
    ${PROJECT_ROOT}/drivers/dev_gpio/port/mock/dev_gpio_port_mock.c
    ${PROJECT_ROOT}/boards/board_mock/dev_gpio_board_cfg.c
    test_gpio.c
)

target_include_directories(gpio_test_host PRIVATE
    ${PROJECT_ROOT}/drivers/dev_common/include
    ${PROJECT_ROOT}/drivers/dev_gpio/include
    ${PROJECT_ROOT}/drivers/dev_gpio/port/mock
    ${PROJECT_ROOT}/boards/board_mock
)

target_compile_options(gpio_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
```

---

## 9. Mock Port (Testing)

The mock port simulates GPIO behavior on a Linux host — no hardware needed.

### 9.1 Error Injection

```c
/* All operations fail */
void dev_gpio_port_mock_set_error(dev_err_t error);

/* Only one operation type fails */
void dev_gpio_port_mock_set_error_for_op(dev_gpio_port_mock_op_t op, dev_err_t error);

/* The Nth call to any operation fails */
void dev_gpio_port_mock_set_fail_after(uint16_t call_count, dev_err_t error);

/* Clear all injected errors */
void dev_gpio_port_mock_clear_error(void);
```

### 9.2 ISR Simulation

```c
void dev_gpio_port_mock_trigger_isr(dev_gpio_channel_t channel);
```

### 9.3 State Inspection

```c
dev_gpio_level_t     dev_gpio_port_mock_get_level(dev_gpio_channel_t channel);
dev_gpio_direction_t dev_gpio_port_mock_get_direction(dev_gpio_channel_t channel);
dev_gpio_pull_t      dev_gpio_port_mock_get_pull(dev_gpio_channel_t channel);
bool                 dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_channel_t channel);
```

### 9.4 Test Example

```c
#include "dev_gpio.h"
#include "dev_gpio_board_cfg.h"
#include "dev_gpio_port_mock.h"

void test_write_fails(void)
{
    dev_gpio_init(&g_dev_gpio_config);

    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);

    dev_err_t err = dev_gpio_write(DEV_GPIO_CHANNEL_PB0, DEV_GPIO_LEVEL_HIGH);
    /* err == DEV_ERR_HW_FAILURE */

    dev_gpio_port_mock_clear_error();
    dev_gpio_deinit();
}
```

---

## 10. Usage Examples

### 10.1 Blink Two LEDs (no interrupts)

```c
#include "dev_gpio.h"
#include "dev_gpio_board_cfg.h"

int main(void)
{
    if (dev_gpio_init(&g_dev_gpio_config) != DEV_OK) { Error_Handler(); }

    for (;;) {
        (void)dev_gpio_toggle(DEV_GPIO_CHANNEL_PB0);
        (void)dev_gpio_toggle(DEV_GPIO_CHANNEL_PB1);

        for (volatile uint32_t d = 0U; d < 5000000U; d++) { /* ~500ms */ }
    }
    return 0;
}
```

### 10.2 Button Triggers LED via Interrupt

```c
static volatile bool g_button_pressed = false;

static void button_isr(dev_gpio_channel_t ch, void *arg)
{
    (void)ch; (void)arg;
    g_button_pressed = true;
}

int main(void)
{
    dev_gpio_init(&g_dev_gpio_config);
    dev_gpio_register_callback(BTN_CHANNEL, button_isr, NULL);
    dev_gpio_enable_interrupt(BTN_CHANNEL);

    for (;;) {
        if (g_button_pressed) {
            g_button_pressed = false;
            (void)dev_gpio_toggle(LED_CHANNEL);
        }
    }
}
```

### 10.3 Using Check Macros for Robust Code

```c
dev_err_t set_led_if_button_low(void)
{
    dev_gpio_level_t level;

    DEV_CHECK_OK_RET(dev_gpio_read(BTN_CHANNEL, &level));

    if (level == DEV_GPIO_LEVEL_LOW) {
        DEV_CHECK_OK_RET(dev_gpio_write(LED_CHANNEL, DEV_GPIO_LEVEL_HIGH));
    }

    return DEV_OK;
}
```

### 10.4 Runtime Direction Change

```c
/* Reconfigure a pin from output to input at runtime */
dev_err_t reconfigure_as_input(dev_gpio_channel_t ch)
{
    DEV_CHECK_OK_RET(dev_gpio_set_direction(ch, DEV_GPIO_DIRECTION_INPUT));
    DEV_CHECK_OK_RET(dev_gpio_set_pull(ch, DEV_GPIO_PULL_UP));
    return DEV_OK;
}
```

---

## 11. File Reference

| File | Responsibility | Public? |
|------|---------------|---------|
| `drivers/dev_gpio/include/dev_gpio.h` | Public API declarations + Doxygen | ✅ Application includes |
| `drivers/dev_gpio/include/dev_gpio_types.h` | Channel, level, direction, pull, interrupt types | ✅ |
| `drivers/dev_gpio/include/dev_gpio_cfg.h` | Compile-time configuration switches | ✅ |
| `drivers/dev_gpio/include/dev_gpio_port.h` | Port interface + `dev_gpio_dispatch_isr()` | ✅ Port includes |
| `drivers/dev_gpio/src/dev_gpio.c` | Common driver: validation, dispatch, callback tables | ❌ internal |
| `drivers/dev_gpio/port/mock/*` | Mock port for host testing | ❌ test only |
| `drivers/dev_gpio/port/stm32/*` | STM32H7 port using HAL | ❌ port layer |
| `boards/board_<target>/dev_gpio_board_cfg.h` | Channel `#define`s | ✅ |
| `boards/board_<target>/dev_gpio_board_cfg.c` | Config struct | ❌ internal |
| `tests/dev_gpio/test_gpio.c` | 36-test suite | ❌ |
| `tests/dev_gpio/CMakeLists.txt` | Host test build | ❌ |

---

## 12. API Quick Reference

| Function | Purpose | ISR-Safe |
|----------|---------|----------|
| `dev_gpio_init(config)` | Initialize driver | No |
| `dev_gpio_deinit()` | De-initialize (force UNINIT) | No |
| `dev_gpio_read(ch, *lvl)` | Read level; `*lvl` unchanged on failure | Port-dep. |
| `dev_gpio_write(ch, lvl)` | Write level; rejects input-only | Port-dep. |
| `dev_gpio_toggle(ch)` | Toggle; rejects input-only | Port-dep. |
| `dev_gpio_set_direction(ch, dir)` | Runtime direction change | No |
| `dev_gpio_set_pull(ch, pull)` | Runtime pull change | No |
| `dev_gpio_config_interrupt(ch, intr)` | Configure interrupt mode | No |
| `dev_gpio_register_callback(ch, cb, arg)` | Register/clear ISR callback | No |
| `dev_gpio_enable_interrupt(ch)` | Enable + rollback on port fail | No |
| `dev_gpio_disable_interrupt(ch)` | Disable; idempotent | No |
| `dev_gpio_is_initialized()` | Check state | **Yes** |
