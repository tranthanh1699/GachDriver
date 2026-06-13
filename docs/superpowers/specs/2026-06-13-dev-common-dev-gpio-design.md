# dev_common + dev_gpio Design Specification

**Date:** 2026-06-13
**Status:** Approved
**Approach:** A — Mock-first, host-testable

## 1. Purpose

Implement the first two driver modules (`dev_common`, `dev_gpio`) for the GachDriver embedded
driver abstraction project. These modules enable application code to control digital I/O through
logical channels without depending on vendor HALs (STM32, ESP-IDF, nrfx).

## 2. Architecture

### 2.1 Dependency Graph

```
Application (main.c)
   |
   v
dev_gpio.h (public API: dev_gpio_init, dev_gpio_write, ...)
   |
   v
dev_gpio.c (common logic: validate → dispatch to port)
   |
   v
dev_gpio_port.h (port interface)
   |
   +-- port/mock/dev_gpio_port_mock.c   (host testing)
   +-- port/stm32/dev_gpio_port_stm32.c (future)
   +-- port/esp32/dev_gpio_port_esp32.c (future)
   +-- port/nrf52/dev_gpio_port_nrf52.c (future)
   |
   v
dev_common (dev_err_t, dev_assert, dev_types, dev_compiler, dev_version)
```

- `dev_gpio` depends on `dev_common`
- `dev_common` does NOT depend on any driver module
- Vendor headers are allowed ONLY in port layer files
- Application code never includes vendor HAL headers

### 2.2 Module State Machine

```
UNINITIALIZED ──[dev_gpio_init()]──> INITIALIZED
     ^                                  |
     |                                  |
     +──────[dev_gpio_deinit()]─────────+
```

### 2.3 Key Design Decisions

| Decision | Choice | Reason |
|----------|--------|--------|
| Config storage | Store pointer, not copy | No dynamic allocation, MISRA-C compliant |
| Channel lookup | Linear scan over config array | Max 32 channels, deterministic |
| Callback context | ISR-safe `void*` user arg per channel | Flexible, no global state leakage |
| Error reporting | Static backend config, compile-time | Deterministic |
| Mock state | Static arrays indexed by channel | No allocation, fast, testable |
| Build order | Mock-first on host, then STM32 port | Fast iteration, no hardware needed |

## 3. Module: dev_common

### 3.1 Purpose

Foundation component providing shared types, error codes, assert/check macros,
compiler abstraction, and version information for all `dev_*` modules.

### 3.2 Files

```
drivers/dev_common/
  include/
    dev_common.h       # Umbrella include
    dev_types.h        # <stdint.h>, <stdbool.h>, <stddef.h>
    dev_error.h        # dev_err_t enum
    dev_assert.h       # Assert macros, backend config, report API
    dev_compiler.h     # Compiler abstraction macros
    dev_version.h      # DEV_VERSION_MAJOR/MINOR/PATCH
  src/
    dev_common.c       # Shared helpers
    dev_assert.c       # dev_assert_report implementation
```

### 3.3 Error Type (dev_error.h)

```c
typedef enum {
    DEV_OK = 0,
    DEV_ERR_FAIL,
    DEV_ERR_INVALID_ARG,
    DEV_ERR_NULL_PTR,
    DEV_ERR_INVALID_STATE,
    DEV_ERR_NOT_INITIALIZED,
    DEV_ERR_ALREADY_INITIALIZED,
    DEV_ERR_BUSY,
    DEV_ERR_TIMEOUT,
    DEV_ERR_NOT_SUPPORTED,
    DEV_ERR_OUT_OF_RANGE,
    DEV_ERR_HW_FAILURE,
    DEV_ERR_CONFIG
} dev_err_t;
```

### 3.4 Assert System

Backend modes: `NONE | UART | TEXT_BUFFER | BREAKPOINT | RESET | USER_HOOK`

Macros:
- `DEV_CHECK_RET(condition, error_code)` — validate, report, return on failure
- `DEV_CHECK_PTR_RET(pointer)` — null pointer check, return DEV_ERR_NULL_PTR
- `DEV_CHECK_OK_RET(expression)` — call expression, propagate non-DEV_OK results
- `DEV_ASSERT(condition)` — fatal assert with backend dispatch

All macros use `do { } while (false)`, evaluate args once, and include `_RET` if returning.

## 4. Module: dev_gpio

### 4.1 Purpose

Hardware-independent digital I/O driver using logical channels.

### 4.2 Files

```
drivers/dev_gpio/
  include/
    dev_gpio.h         # Public API declarations
    dev_gpio_types.h   # channel_t, level_t, direction_t, pull_t, intr_type_t, callback_t
    dev_gpio_cfg.h     # DEV_GPIO_CFG_MAX_CHANNELS, feature switches
    dev_gpio_port.h    # Port interface
  src/
    dev_gpio.c         # Common logic
  port/
    mock/
      dev_gpio_port_mock.c
      dev_gpio_port_mock.h
```

### 4.3 Key Types

```c
typedef uint16_t dev_gpio_channel_t;

typedef enum { DEV_GPIO_LEVEL_LOW = 0, DEV_GPIO_LEVEL_HIGH = 1 } dev_gpio_level_t;
typedef enum { DEV_GPIO_DIRECTION_INPUT = 0, DEV_GPIO_DIRECTION_OUTPUT,
               DEV_GPIO_DIRECTION_INPUT_OUTPUT } dev_gpio_direction_t;
typedef enum { DEV_GPIO_PULL_NONE = 0, DEV_GPIO_PULL_UP, DEV_GPIO_PULL_DOWN } dev_gpio_pull_t;
typedef enum { DEV_GPIO_INTR_DISABLE = 0, DEV_GPIO_INTR_RISING_EDGE, DEV_GPIO_INTR_FALLING_EDGE,
               DEV_GPIO_INTR_BOTH_EDGES, DEV_GPIO_INTR_LOW_LEVEL,
               DEV_GPIO_INTR_HIGH_LEVEL } dev_gpio_intr_type_t;

typedef void (*dev_gpio_isr_callback_t)(dev_gpio_channel_t channel, void *user_arg);

typedef struct {
    dev_gpio_channel_t    channel;
    dev_gpio_direction_t  direction;
    dev_gpio_pull_t       pull;
    dev_gpio_level_t      default_level;
    dev_gpio_intr_type_t  interrupt;
    dev_gpio_isr_callback_t callback;
    void                 *callback_arg;
} dev_gpio_channel_config_t;

typedef struct {
    const dev_gpio_channel_config_t *channels;
    uint16_t                         channel_count;
} dev_gpio_config_t;
```

### 4.4 Public API

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
bool     dev_gpio_is_initialized(void);
```

### 4.5 Port Interface (dev_gpio_port.h)

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

### 4.6 Mock Port

Maintains internal static arrays:
- `levels[MAX_CHANNELS]` — simulated pin levels
- `directions[MAX_CHANNELS]` — simulated directions
- `pulls[MAX_CHANNELS]` — simulated pull modes
- `interrupts[MAX_CHANNELS]` — simulated interrupt configs
- `callbacks[MAX_CHANNELS]` — registered callbacks
- `callback_args[MAX_CHANNELS]` — user args
- `interrupt_enabled[MAX_CHANNELS]` — enable state
- `error_mode` — configurable error injection for negative testing

Provides `dev_gpio_port_mock_trigger_isr(channel)` to simulate interrupt firing.

### 4.7 Configuration (dev_gpio_cfg.h)

```c
#define DEV_GPIO_CFG_MAX_CHANNELS          (32U)
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)
#define DEV_GPIO_CFG_VALIDATE_DUPLICATES   (1U)
#define DEV_GPIO_CFG_ENABLE_RUNTIME_CHECKS (1U)
```

## 5. Board Configuration (board_mock)

```
boards/board_mock/
  dev_board_cfg.h          # Board-level feature switches
  dev_gpio_board_cfg.h     # Logical channel #defines
  dev_gpio_board_cfg.c     # Static config struct instance
```

Example channel definitions:
```c
#define DEV_GPIO_CHANNEL_LED_STATUS      ((dev_gpio_channel_t)0U)
#define DEV_GPIO_CHANNEL_BUTTON_USER     ((dev_gpio_channel_t)1U)
```

## 6. Build Integration

### 6.1 Host Mock Build

Standalone CMakeLists.txt or Makefile compiling `dev_common`, `dev_gpio`, `mock port`,
`board_mock`, and test files into a host executable. Uses standard gcc/clang on Linux.

### 6.2 Target STM32 Build

Integrates into existing `CMakeLists.txt`:
- Add `drivers/dev_common/include`, `drivers/dev_gpio/include`, `boards/board_stm32h743`
  to `target_include_directories`
- Add `.c` files to `target_sources`
- STM32 port `.c` files (future) include `stm32h7xx_hal.h` ONLY in the port layer

## 7. Test Plan

All tests run against the mock port on host. 19 test cases:

| # | Test | Expected |
|---|------|----------|
| 1 | init with valid config | DEV_OK |
| 2 | init with NULL config | DEV_ERR_NULL_PTR |
| 3 | init with NULL channels | DEV_ERR_NULL_PTR |
| 4 | init with channel_count=0 | DEV_ERR_INVALID_ARG |
| 5 | double init | DEV_ERR_ALREADY_INITIALIZED |
| 6 | read before init | DEV_ERR_NOT_INITIALIZED |
| 7 | write before init | DEV_ERR_NOT_INITIALIZED |
| 8 | write to input-only channel | DEV_ERR_INVALID_STATE |
| 9 | read from output channel | DEV_OK, returns default_level |
| 10 | write then read output | DEV_OK, level matches |
| 11 | toggle output | DEV_OK, level flips |
| 12 | set pull mode | DEV_OK |
| 13 | register callback | DEV_OK |
| 14 | enable interrupt, trigger ISR | callback invoked with correct channel+arg |
| 15 | disable interrupt | DEV_OK, callback not invoked |
| 16 | deinit | DEV_OK |
| 17 | reinit after deinit | DEV_OK |
| 18 | unsupported intr type | DEV_ERR_NOT_SUPPORTED |
| 19 | duplicate channel detection | DEV_ERR_CONFIG |

## 8. MISRA-C Compliance

- No dynamic memory allocation
- No recursion
- No unbounded loops (all loops bounded by channel_count ≤ 32)
- No magic numbers (all constants named in cfg.h)
- Fixed-width integer types throughout
- `bool` for boolean values
- Private functions declared `static`
- Macros use `do { } while (false)`, evaluate args once
- No `goto`, no `continue`
- Vendor code isolated in port layer

## 9. Definition of Done

- [x] 18 source files created
- [ ] All public APIs documented with Doxygen comments
- [ ] Mock port compiles and runs on host
- [ ] All 19 test cases pass
- [ ] No vendor headers in public APIs or common logic
- [ ] No magic numbers
- [ ] `DEV_OK` equals 0
- [ ] All error paths return specific `dev_err_t` values
