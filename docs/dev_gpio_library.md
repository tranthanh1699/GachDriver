# dev_gpio — Hardware-Independent GPIO Driver

**Version:** 0.1.0  
**Language:** C99/C11  
**Depends on:** `dev_common`  
**MISRA-C oriented:** Yes  
**Hardware ports:** mock (host), STM32, ESP32, nRF52 (planned)

---

## 1. Overview

`dev_gpio` is a hardware-independent digital I/O driver. Application code uses **logical channel IDs** instead of raw MCU pins. The driver validates all inputs, then dispatches to a **port layer** that contains all vendor-specific code.

**Key principles:**
- Application code never includes vendor HAL headers
- Logical channels abstract away pin numbers
- Port layer isolates all hardware dependencies
- No dynamic memory allocation
- Configuration is compile-time static

---

## 2. Include

```c
#include "dev_gpio.h"            /* Public API */
#include "dev_gpio_board_cfg.h"  /* Board channel definitions */
```

---

## 3. Core Types — `dev_gpio_types.h`

### 3.1 Channel ID

```c
typedef uint16_t dev_gpio_channel_t;
```

Logical channel IDs are defined by the board configuration. They may be sparse (any `uint16_t` value). The driver maps channel IDs to config indices internally via linear scan.

### 3.2 Logic Level

```c
typedef enum {
    DEV_GPIO_LEVEL_LOW  = 0,
    DEV_GPIO_LEVEL_HIGH = 1
} dev_gpio_level_t;
```

### 3.3 Direction

```c
typedef enum {
    DEV_GPIO_DIRECTION_INPUT  = 0,
    DEV_GPIO_DIRECTION_OUTPUT,
    DEV_GPIO_DIRECTION_INPUT_OUTPUT
} dev_gpio_direction_t;
```

### 3.4 Pull Mode

```c
typedef enum {
    DEV_GPIO_PULL_NONE = 0,
    DEV_GPIO_PULL_UP,
    DEV_GPIO_PULL_DOWN
} dev_gpio_pull_t;
```

### 3.5 Interrupt Type

```c
typedef enum {
    DEV_GPIO_INTR_DISABLE      = 0,
    DEV_GPIO_INTR_RISING_EDGE,
    DEV_GPIO_INTR_FALLING_EDGE,
    DEV_GPIO_INTR_BOTH_EDGES,
    DEV_GPIO_INTR_LOW_LEVEL,
    DEV_GPIO_INTR_HIGH_LEVEL
} dev_gpio_intr_type_t;
```

### 3.6 ISR Callback

```c
typedef void (*dev_gpio_isr_callback_t)(dev_gpio_channel_t channel, void *user_arg);
```

**Rules:**
- Callback is executed from ISR context (if the platform fires interrupts).
- Callback must not block.
- Callback must not call non-ISR-safe APIs.
- `user_arg` is stored per-channel and passed to the callback.

### 3.7 Channel Configuration

```c
typedef struct {
    dev_gpio_channel_t      channel;        /* Logical channel ID */
    dev_gpio_direction_t    direction;      /* INPUT, OUTPUT, INPUT_OUTPUT */
    dev_gpio_pull_t         pull;           /* NONE, UP, DOWN */
    dev_gpio_level_t        default_level;  /* Initial output level */
    dev_gpio_intr_type_t    interrupt;      /* Interrupt mode */
    dev_gpio_isr_callback_t callback;       /* Initial ISR callback (may be NULL) */
    void                   *callback_arg;   /* User argument for callback */
} dev_gpio_channel_config_t;
```

### 3.8 Driver Configuration

```c
typedef struct {
    const dev_gpio_channel_config_t *channels;       /* Array of channel configs */
    uint16_t                         channel_count;  /* Number of channels */
} dev_gpio_config_t;
```

---

## 4. Configuration — `dev_gpio_cfg.h`

```c
#define DEV_GPIO_CFG_MAX_CHANNELS          (32U)   /* Maximum channels per driver instance */
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)    /* 1U = interrupt APIs functional; 0U = stubs return NOT_SUPPORTED */
#define DEV_GPIO_CFG_VALIDATE_DUPLICATES   (1U)    /* 1U = reject duplicate channel IDs at init */
#define DEV_GPIO_CFG_ENABLE_RUNTIME_CHECKS (1U)    /* (reserved for future use) */
```

**Feature switch behavior:**

| Switch | Value `1U` | Value `0U` |
|--------|-----------|-----------|
| `INTERRUPT_ENABLED` | Interrupt APIs fully functional | APIs compile but return `DEV_ERR_NOT_SUPPORTED`; `dev_gpio_init()` rejects any channel with non-DISABLE interrupt |
| `VALIDATE_DUPLICATES` | Init scans for duplicate channel IDs | Duplicate check skipped |

---

## 5. Public API — `dev_gpio.h`

### 5.1 Lifecycle

```
UNINITIALIZED ──[dev_gpio_init()]──> INITIALIZED
     ^                                  |
     |                                  |
     +──────[dev_gpio_deinit()]─────────+
```

### 5.2 dev_gpio_init

```c
dev_err_t dev_gpio_init(const dev_gpio_config_t *config);
```

Initializes the GPIO driver. Validates all channels, initializes the port layer, configures each channel. State is set to INITIALIZED only after ALL steps succeed.

**Validation order:**
1. Already initialized → `DEV_ERR_ALREADY_INITIALIZED`
2. `config` is NULL → `DEV_ERR_NULL_PTR`
3. `config->channels` is NULL → `DEV_ERR_NULL_PTR`
4. `channel_count == 0` → `DEV_ERR_INVALID_ARG`
5. `channel_count > DEV_GPIO_CFG_MAX_CHANNELS` → `DEV_ERR_INVALID_ARG`
6. Invalid direction enum → `DEV_ERR_INVALID_ARG`
7. Invalid pull enum → `DEV_ERR_INVALID_ARG`
8. Invalid interrupt enum → `DEV_ERR_INVALID_ARG`
9. Interrupts compiled out but channel requests interrupt → `DEV_ERR_NOT_SUPPORTED`
10. Duplicate channel IDs → `DEV_ERR_CONFIG`
11. Port init fails → `DEV_ERR_HW_FAILURE` (cleanup: port deinit, state stays UNINITIALIZED)
12. Port channel config fails → `DEV_ERR_HW_FAILURE` (cleanup: port deinit, state stays UNINITIALIZED)

**Not reentrant. Not ISR-safe.**

### 5.3 dev_gpio_deinit

```c
dev_err_t dev_gpio_deinit(void);
```

De-initializes the GPIO driver. All interrupts are disabled (best-effort), all callbacks are cleared, and the module is **forced to UNINITIALIZED regardless of port errors**.

**Returns:**
- `DEV_OK` on success
- `DEV_ERR_NOT_INITIALIZED` if not initialized
- `DEV_ERR_HW_FAILURE` if port deinit fails (module is still deinitialized; reinit is allowed)

**Not reentrant. Not ISR-safe.**

### 5.4 dev_gpio_read

```c
dev_err_t dev_gpio_read(dev_gpio_channel_t channel, dev_gpio_level_t *level);
```

Reads the logic level of a GPIO channel. Reads into a local temporary variable; `*level` is written **only on success**. On failure, `*level` is unchanged.

**Returns:**
- `DEV_OK` on success
- `DEV_ERR_NOT_INITIALIZED` if not initialized
- `DEV_ERR_NULL_PTR` if `level` is NULL
- `DEV_ERR_INVALID_ARG` if channel not found
- `DEV_ERR_HW_FAILURE` if port read fails

**Not ISR-safe unless the port documents support.**

### 5.5 dev_gpio_write

```c
dev_err_t dev_gpio_write(dev_gpio_channel_t channel, dev_gpio_level_t level);
```

Writes a logic level to a configured GPIO channel. Rejects writes to input-only channels.

**Returns:**
- `DEV_OK` on success
- `DEV_ERR_NOT_INITIALIZED` if not initialized
- `DEV_ERR_INVALID_ARG` if channel not found or level is not LOW/HIGH
- `DEV_ERR_INVALID_STATE` if channel is input-only
- `DEV_ERR_HW_FAILURE` if port write fails

**Not ISR-safe unless the port documents support.**

### 5.6 dev_gpio_toggle

```c
dev_err_t dev_gpio_toggle(dev_gpio_channel_t channel);
```

Toggles the output level of a GPIO channel (LOW→HIGH, HIGH→LOW). Rejects toggle on input-only channels.

**Not ISR-safe unless the port documents support.**

### 5.7 dev_gpio_set_direction

```c
dev_err_t dev_gpio_set_direction(dev_gpio_channel_t channel,
                                 dev_gpio_direction_t direction);
```

Changes the direction of a GPIO channel at runtime. Updates internal direction tracking on success.

**Not ISR-safe.**

### 5.8 dev_gpio_set_pull

```c
dev_err_t dev_gpio_set_pull(dev_gpio_channel_t channel, dev_gpio_pull_t pull);
```

Changes the pull mode of a GPIO channel at runtime.

**Not ISR-safe.**

### 5.9 dev_gpio_config_interrupt

```c
dev_err_t dev_gpio_config_interrupt(dev_gpio_channel_t channel,
                                    dev_gpio_intr_type_t interrupt);
```

Configures the interrupt mode for a GPIO channel.

**Not ISR-safe.**

### 5.10 dev_gpio_register_callback

```c
dev_err_t dev_gpio_register_callback(dev_gpio_channel_t channel,
                                     dev_gpio_isr_callback_t callback,
                                     void *user_arg);
```

Registers (or clears) an ISR callback for a GPIO channel. `callback` may be NULL to clear. The interrupt must be enabled separately via `dev_gpio_enable_interrupt()`.

### 5.11 dev_gpio_enable_interrupt

```c
dev_err_t dev_gpio_enable_interrupt(dev_gpio_channel_t channel);
```

Enables interrupt for a channel. Common state is marked enabled **before** the port call, so any immediate hardware interrupt is not dropped. On port failure, common state is rolled back to disabled.

### 5.12 dev_gpio_disable_interrupt

```c
dev_err_t dev_gpio_disable_interrupt(dev_gpio_channel_t channel);
```

Disables interrupt for a channel. Common state is marked disabled **before** the port call. Safe to call even if already disabled (idempotent).

### 5.13 dev_gpio_is_initialized

```c
bool dev_gpio_is_initialized(void);
```

Returns `true` if the driver is in INITIALIZED state. **Reentrant. ISR-safe.**

---

## 6. Error Mapping

The common driver applies a consistent error mapping to port return values:

| Port returns | Common driver propagates as |
|-------------|---------------------------|
| `DEV_OK` | `DEV_OK` |
| `DEV_ERR_NOT_SUPPORTED` | `DEV_ERR_NOT_SUPPORTED` (preserved) |
| Any other non-OK | `DEV_ERR_HW_FAILURE` |

This ensures the application sees a consistent error taxonomy regardless of the underlying vendor.

---

## 7. Port Interface — `dev_gpio_port.h`

### 7.1 Port-Implemented Functions

These functions must be implemented by each hardware port:

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

### 7.2 Common-Driver Service (called by port ISR handlers)

```c
/* PORT-ONLY: called from vendor ISR handlers. Do not call from application code. */
void dev_gpio_dispatch_isr(dev_gpio_channel_t channel);
```

### 7.3 Port Rules

- **Callbacks:** Port does NOT own callbacks. It calls `dev_gpio_dispatch_isr()` from ISR handlers.
- **Dispatch gating:** Port does NOT gate callback dispatch. The common driver owns the interrupt-enable state. Ports MAY maintain their own hardware/simulated enable state but MUST NOT use it to gate callbacks.
- **Error mapping:** Map vendor errors to `dev_err_t`. Common driver handles the rest.
- **Unsupported features:** Return `DEV_ERR_NOT_SUPPORTED`.
- **Vendor headers:** Allowed ONLY in port `.c` files.

---

## 8. Mock Port — `dev_gpio_port_mock.h`

The mock port enables host-based testing without hardware.

### 8.1 Error Injection

```c
typedef enum {
    DEV_GPIO_PORT_MOCK_OP_INIT = 0,
    DEV_GPIO_PORT_MOCK_OP_DEINIT,
    DEV_GPIO_PORT_MOCK_OP_CONFIG_CHANNEL,
    DEV_GPIO_PORT_MOCK_OP_READ,
    DEV_GPIO_PORT_MOCK_OP_WRITE,
    DEV_GPIO_PORT_MOCK_OP_TOGGLE,
    DEV_GPIO_PORT_MOCK_OP_SET_DIRECTION,
    DEV_GPIO_PORT_MOCK_OP_SET_PULL,
    DEV_GPIO_PORT_MOCK_OP_CONFIG_INTERRUPT,
    DEV_GPIO_PORT_MOCK_OP_ENABLE_INTERRUPT,
    DEV_GPIO_PORT_MOCK_OP_DISABLE_INTERRUPT,
    DEV_GPIO_PORT_MOCK_OP_COUNT
} dev_gpio_port_mock_op_t;

/* All operations fail with this error */
void dev_gpio_port_mock_set_error(dev_err_t error);

/* Only the specified operation fails */
void dev_gpio_port_mock_set_error_for_op(dev_gpio_port_mock_op_t op, dev_err_t error);

/* The Nth call fails */
void dev_gpio_port_mock_set_fail_after(uint16_t call_count, dev_err_t error);

/* Clear all injected errors */
void dev_gpio_port_mock_clear_error(void);
```

**Precedence:** per-operation > global. Fail-after-N takes precedence over both when its counter matches.

### 8.2 ISR Simulation

```c
void dev_gpio_port_mock_trigger_isr(dev_gpio_channel_t channel);
```

Invokes the common ISR dispatch as if hardware triggered an interrupt.

### 8.3 State Inspection

```c
dev_gpio_level_t     dev_gpio_port_mock_get_level(dev_gpio_channel_t channel);
dev_gpio_direction_t dev_gpio_port_mock_get_direction(dev_gpio_channel_t channel);
dev_gpio_pull_t      dev_gpio_port_mock_get_pull(dev_gpio_channel_t channel);
bool                 dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_channel_t channel);
```

### 8.4 Simulated Unsupported Features

The mock port returns `DEV_ERR_NOT_SUPPORTED` for:
- `DEV_GPIO_DIRECTION_INPUT_OUTPUT` (bidirectional)
- `DEV_GPIO_PULL_DOWN`
- `DEV_GPIO_INTR_BOTH_EDGES`, `DEV_GPIO_INTR_LOW_LEVEL`, `DEV_GPIO_INTR_HIGH_LEVEL`

These simulate a minimal hardware port for negative testing.

---

## 9. Board Configuration

### 9.1 Board Header — `dev_gpio_board_cfg.h`

Define logical channel IDs as named constants:

```c
#include "dev_gpio.h"

#define DEV_GPIO_CHANNEL_LED_STATUS      ((dev_gpio_channel_t)0U)
#define DEV_GPIO_CHANNEL_BUTTON_USER     ((dev_gpio_channel_t)10U)
#define DEV_GPIO_CHANNEL_CAN_STB         ((dev_gpio_channel_t)2U)
#define DEV_GPIO_CHANNEL_SPI_CS          ((dev_gpio_channel_t)3U)

extern const dev_gpio_config_t g_dev_gpio_config;
```

Channel IDs may be sparse — the driver maps them via linear scan, not array index.

### 9.2 Board Implementation — `dev_gpio_board_cfg.c`

```c
#include "dev_gpio_board_cfg.h"
#include "dev_compiler.h"

static dev_gpio_channel_config_t m_channels[] = {
    {
        .channel       = DEV_GPIO_CHANNEL_LED_STATUS,
        .direction     = DEV_GPIO_DIRECTION_OUTPUT,
        .pull          = DEV_GPIO_PULL_NONE,
        .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt     = DEV_GPIO_INTR_DISABLE,
        .callback      = NULL,
        .callback_arg  = NULL,
    },
    {
        .channel       = DEV_GPIO_CHANNEL_BUTTON_USER,
        .direction     = DEV_GPIO_DIRECTION_INPUT,
        .pull          = DEV_GPIO_PULL_UP,
        .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt     = DEV_GPIO_INTR_RISING_EDGE,
        .callback      = NULL,        /* set via dev_gpio_register_callback() */
        .callback_arg  = NULL,
    },
};

const dev_gpio_config_t g_dev_gpio_config = {
    .channels      = m_channels,
    .channel_count = (uint16_t)DEV_ARRAY_SIZE(m_channels),
};
```

---

## 10. Usage Examples

### 10.1 Basic GPIO Output

```c
#include "dev_gpio.h"
#include "dev_gpio_board_cfg.h"
#include "dev_common.h"

int main(void)
{
    dev_err_t err;

    err = dev_gpio_init(&g_dev_gpio_config);
    if (err != DEV_OK) {
        /* Handle init failure */
        return 1;
    }

    /* Turn LED on */
    (void)dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, DEV_GPIO_LEVEL_HIGH);

    /* Blink loop */
    for (;;) {
        (void)dev_gpio_toggle(DEV_GPIO_CHANNEL_LED_STATUS);
        /* delay... */
    }

    return 0;
}
```

### 10.2 GPIO Input with Interrupt

```c
#include "dev_gpio.h"
#include "dev_gpio_board_cfg.h"
#include "dev_common.h"

static volatile bool g_button_pressed = false;

static void button_isr(dev_gpio_channel_t channel, void *user_arg)
{
    (void)channel;
    (void)user_arg;
    g_button_pressed = true;
}

int main(void)
{
    dev_err_t err;

    err = dev_gpio_init(&g_dev_gpio_config);
    if (err != DEV_OK) {
        return 1;
    }

    /* Register ISR callback */
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER,
                                     button_isr, NULL);
    if (err != DEV_OK) {
        return 1;
    }

    /* Enable interrupt */
    err = dev_gpio_enable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    if (err != DEV_OK) {
        return 1;
    }

    for (;;) {
        if (g_button_pressed) {
            g_button_pressed = false;
            (void)dev_gpio_toggle(DEV_GPIO_CHANNEL_LED_STATUS);
        }
    }

    return 0;
}
```

### 10.3 Using Check Macros

```c
dev_err_t safe_gpio_ops(void)
{
    dev_gpio_level_t level;

    /* Propagates errors automatically */
    DEV_CHECK_OK_RET(dev_gpio_read(DEV_GPIO_CHANNEL_BUTTON_USER, &level));

    if (level == DEV_GPIO_LEVEL_HIGH) {
        DEV_CHECK_OK_RET(dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS,
                                        DEV_GPIO_LEVEL_HIGH));
    }

    return DEV_OK;
}
```

### 10.4 Error Injection (Testing)

```c
void test_write_failure_handling(void)
{
    dev_err_t err;

    err = dev_gpio_init(&g_dev_gpio_config);

    /* Inject error: all port writes will fail */
    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);

    err = dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, DEV_GPIO_LEVEL_HIGH);
    /* err == DEV_ERR_HW_FAILURE */

    dev_gpio_port_mock_clear_error();
    dev_gpio_deinit();
}
```

---

## 11. Build Integration

### 11.1 Include Paths

```
drivers/dev_common/include
drivers/dev_gpio/include
boards/board_<target>
```

### 11.2 Source Files

```
drivers/dev_common/src/dev_common.c
drivers/dev_common/src/dev_assert.c
drivers/dev_gpio/src/dev_gpio.c
drivers/dev_gpio/port/<target>/dev_gpio_port_<target>.c
boards/board_<target>/dev_gpio_board_cfg.c
```

### 11.3 CMake (STM32 Target)

```cmake
target_include_directories(${PROJECT_NAME} PRIVATE
    drivers/dev_common/include
    drivers/dev_gpio/include
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

### 11.4 CMake (Host Test with Mock Port)

```cmake
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
```

---

## 12. Porting to New Hardware

### 12.1 Checklist

1. Create `boards/board_<target>/dev_gpio_board_cfg.h` — define logical channel IDs
2. Create `boards/board_<target>/dev_gpio_board_cfg.c` — populate channel config array
3. Create `drivers/dev_gpio/port/<target>/dev_gpio_port_<target>.c` — implement all 11 port functions
4. Map vendor errors to `dev_err_t` inside the port
5. Include vendor HAL headers **only** in the port `.c` file
6. Call `dev_gpio_dispatch_isr()` from vendor ISR handlers
7. Return `DEV_ERR_NOT_SUPPORTED` for unsupported features
8. Build with the port file instead of mock
9. Run the common test suite against the mock port first
10. Run target-specific tests on hardware

### 12.2 Port Implementation Skeleton

```c
/* dev_gpio_port_stm32.c */
#include "dev_gpio_port.h"
#include "stm32h7xx_hal.h"          /* <-- vendor header ONLY here */

/* Internal: channel ID → hardware pin mapping table */
static const struct {
    dev_gpio_channel_t channel;
    GPIO_TypeDef      *port;
    uint16_t           pin;
} m_pin_map[DEV_GPIO_CFG_MAX_CHANNELS];
/* ... */

dev_err_t dev_gpio_port_write(dev_gpio_channel_t channel,
                              dev_gpio_level_t level)
{
    /* Find pin mapping, translate level to HAL, call HAL_GPIO_WritePin() */
    GPIO_PinState hal_level = (level == DEV_GPIO_LEVEL_HIGH) ?
                               GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(port, pin, hal_level);
    return DEV_OK;
}

/* In STM32 interrupt handler: */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    /* Map pin back to logical channel, then dispatch */
    dev_gpio_dispatch_isr(mapped_channel);
}
```

---

## 13. API Quick Reference

| Function | Purpose | ISR-Safe |
|----------|---------|----------|
| `dev_gpio_init(config)` | Initialize driver | No |
| `dev_gpio_deinit()` | De-initialize driver | No |
| `dev_gpio_read(ch, *lvl)` | Read logic level | Port-dependent |
| `dev_gpio_write(ch, lvl)` | Write logic level | Port-dependent |
| `dev_gpio_toggle(ch)` | Toggle output level | Port-dependent |
| `dev_gpio_set_direction(ch, dir)` | Change direction | No |
| `dev_gpio_set_pull(ch, pull)` | Change pull mode | No |
| `dev_gpio_config_interrupt(ch, intr)` | Configure interrupt mode | No |
| `dev_gpio_register_callback(ch, cb, arg)` | Register ISR callback | No |
| `dev_gpio_enable_interrupt(ch)` | Enable interrupt | No |
| `dev_gpio_disable_interrupt(ch)` | Disable interrupt | No |
| `dev_gpio_is_initialized()` | Check init state | **Yes** |
