# dev_common + dev_gpio Design Specification

**Date:** 2026-06-13
**Status:** Ready for implementation
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

State transitions:
- Only `dev_gpio_init()` moves from UNINITIALIZED → INITIALIZED. State is set to INITIALIZED
  ONLY after ALL port init and per-channel configuration succeed. On any failure, the driver
  remains UNINITIALIZED and any partially configured channels are cleaned up via
  `dev_gpio_port_deinit()`.
- Only `dev_gpio_deinit()` moves from INITIALIZED → UNINITIALIZED. Before clearing state,
  all interrupts are disabled, all callbacks are cleared, and `dev_gpio_port_deinit()` is called.

### 2.3 Key Design Decisions

| Decision | Choice | Reason |
|----------|--------|--------|
| Config storage | Store pointer, not copy | No dynamic allocation, MISRA-C compliant |
| Channel lookup | Map channel ID → config index; arrays indexed by config index | Supports sparse logical channel IDs; board controls channel numbering |
| Callback ownership | Common driver owns static callback tables; port calls `dev_gpio_dispatch_isr()` | Clear ownership, port stays thin, ISR dispatch is consistent |
| Error reporting | Static backend config, compile-time | Deterministic |
| Mock state | Static arrays indexed by config index | No allocation, fast, testable |
| Build order | Mock-first on host, then STM32 port | Fast iteration, no hardware needed |

### 2.4 Callback and ISR Dispatch Design

The `dev_gpio_channel_config_t` struct is passed through a `const` config pointer, so
`dev_gpio_register_callback()` cannot mutate the original config. Instead:

- **Common driver (`dev_gpio.c`)** owns two static arrays sized to `DEV_GPIO_CFG_MAX_CHANNELS`:
  - `g_dev_gpio_callbacks[config_index]` — `dev_gpio_isr_callback_t` per channel
  - `g_dev_gpio_callback_args[config_index]` — `void*` user arg per channel
- `dev_gpio_register_callback()` writes into these common-driver-owned tables.
- The common driver exposes **`dev_gpio_dispatch_isr(dev_gpio_channel_t channel)`** for the
  port layer to call when a hardware interrupt fires.
- `dev_gpio_dispatch_isr()` maps channel ID → config index, checks interrupt enable state,
  and invokes the registered callback with the stored user arg.
- Port implementations call `dev_gpio_dispatch_isr()` from their vendor ISR handlers.
- `dev_gpio_deinit()` clears both tables to NULL.

### 2.5 Channel ID → Config Index Mapping

Logical channel IDs are `uint16_t` values defined by the board configuration. They may be
sparse (e.g., 0, 3, 7, 100). The common driver maps channel ID to config index:

- During `dev_gpio_init()`, the driver scans `config->channels[]` to build a lookup.
- All internal arrays (callback tables, and the mock port's state arrays) are indexed by
  **config index** (0..channel_count-1), NOT by raw channel ID.
- A helper `dev_gpio_find_channel(channel)` returns the config index or `channel_count` if
  not found.
- Channel IDs must be `< DEV_GPIO_CFG_MAX_CHANNELS` to keep the lookup bounded (validated
  at init).

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
    DEV_ERR_NOT_SUPPORTED,
    DEV_ERR_TIMEOUT,
    DEV_ERR_BUSY,
    DEV_ERR_OUT_OF_RANGE,
    DEV_ERR_HW_FAILURE,
    DEV_ERR_CONFIG
} dev_err_t;
```

Enum order matches the requirements document exactly. `DEV_OK` is 0. All other values are
auto-incremented from 1. If numeric values ever matter in tests, logs, or diagnostics,
this order is the authoritative reference.

### 3.4 Assert System — Full API

#### 3.4.1 Backend Type

```c
typedef enum {
    DEV_ASSERT_BACKEND_NONE = 0,
    DEV_ASSERT_BACKEND_UART,
    DEV_ASSERT_BACKEND_TEXT_BUFFER,
    DEV_ASSERT_BACKEND_BREAKPOINT,
    DEV_ASSERT_BACKEND_RESET,
    DEV_ASSERT_BACKEND_USER_HOOK
} dev_assert_backend_t;
```

#### 3.4.2 Assert Type

```c
typedef enum {
    DEV_ASSERT_TYPE_ASSERT = 0,
    DEV_ASSERT_TYPE_CHECK,
    DEV_ASSERT_TYPE_ERROR
} dev_assert_type_t;
```

#### 3.4.3 Info Struct

```c
typedef struct {
    const char      *file;
    uint32_t         line;
    dev_assert_type_t type;
    dev_err_t        error;
} dev_assert_info_t;
```

#### 3.4.4 Hook Typedefs

```c
typedef void (*dev_assert_user_hook_t)(const dev_assert_info_t *info);
typedef void (*dev_assert_output_hook_t)(const char *text);
```

#### 3.4.5 Config Struct

```c
typedef struct {
    dev_assert_backend_t     backend;
    dev_assert_output_hook_t output_hook;   /* used by UART backend */
    dev_assert_user_hook_t   user_hook;     /* used by USER_HOOK backend */
    char                    *text_buffer;   /* used by TEXT_BUFFER backend */
    uint16_t                 text_buffer_size;
} dev_assert_config_t;
```

#### 3.4.6 Public Functions

```c
void dev_assert_init(const dev_assert_config_t *config);
void dev_assert_report(const char *file, uint32_t line,
                       dev_assert_type_t type, dev_err_t error);
```

#### 3.4.7 Backend Behavior

| Backend | Behavior |
|---------|----------|
| `NONE` | No output, returns immediately |
| `UART` | Formats message, calls `output_hook()` with the formatted string |
| `TEXT_BUFFER` | Formats message into `text_buffer` (bounded by `text_buffer_size`) |
| `BREAKPOINT` | Formats message, then triggers compiler breakpoint (`__builtin_trap()` or equivalent) |
| `RESET` | Formats message, calls `user_hook()`, then triggers reset via `NVIC_SystemReset()` or equivalent |
| `USER_HOOK` | Formats message, calls `user_hook()` with the assert info struct |

#### 3.4.8 Macros

```c
#define DEV_CHECK_RET(condition, error_code)     \
    do {                                         \
        if (!(condition)) {                      \
            dev_assert_report(__FILE__,          \
                              (uint32_t)__LINE__, \
                              DEV_ASSERT_TYPE_CHECK, \
                              (error_code));      \
            return (error_code);                  \
        }                                         \
    } while (false)

#define DEV_CHECK_PTR_RET(pointer)               \
    DEV_CHECK_RET(((pointer) != NULL), DEV_ERR_NULL_PTR)

#define DEV_CHECK_OK_RET(expression)             \
    do {                                         \
        dev_err_t _err = (expression);           \
        if (_err != DEV_OK) {                    \
            dev_assert_report(__FILE__,          \
                              (uint32_t)__LINE__, \
                              DEV_ASSERT_TYPE_CHECK, \
                              _err);              \
            return _err;                          \
        }                                         \
    } while (false)

#define DEV_ASSERT(condition)                    \
    do {                                         \
        if (!(condition)) {                      \
            dev_assert_report(__FILE__,          \
                              (uint32_t)__LINE__, \
                              DEV_ASSERT_TYPE_ASSERT, \
                              DEV_ERR_FAIL);      \
            /* backend BREAKPOINT/RESET handles termination */ \
        }                                         \
    } while (false)
```

Rules:
- All macros use `do { } while (false)`.
- `DEV_CHECK_RET` and `DEV_CHECK_PTR_RET` evaluate `condition`/`pointer` exactly once.
- `DEV_CHECK_OK_RET` evaluates `expression` exactly once via a local variable.
- `DEV_ASSERT` does NOT return; the backend (BREAKPOINT/RESET) determines termination.
- No magic numbers — `__LINE__` is cast to `uint32_t` explicitly.
- Macros do not allocate memory, call vendor APIs, or contain complex logic.

### 3.5 Compiler Abstraction (dev_compiler.h)

```c
#define DEV_UNUSED(x)              ((void)(x))
#define DEV_ARRAY_SIZE(a)          (sizeof(a) / sizeof((a)[0]))
#define DEV_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)

#if defined(__GNUC__) || defined(__clang__)
  #define DEV_WEAK                 __attribute__((weak))
  #define DEV_PACKED               __attribute__((packed))
  #define DEV_ALIGNED(n)           __attribute__((aligned(n)))
  #define DEV_NORETURN             __attribute__((noreturn))
  #define DEV_SECTION(s)           __attribute__((section(s)))
  #define DEV_BREAKPOINT()         __builtin_trap()
#else
  #error "Unsupported compiler"
#endif
```

### 3.6 Version (dev_version.h)

```c
#define DEV_VERSION_MAJOR  0U
#define DEV_VERSION_MINOR  1U
#define DEV_VERSION_PATCH  0U
```

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
    dev_gpio.c         # Common logic + static callback tables + ISR dispatch
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
    dev_gpio_channel_t      channel;
    dev_gpio_direction_t    direction;
    dev_gpio_pull_t         pull;
    dev_gpio_level_t        default_level;
    dev_gpio_intr_type_t    interrupt;
    dev_gpio_isr_callback_t callback;     /* initial callback, may be NULL */
    void                   *callback_arg; /* initial user arg, may be NULL */
} dev_gpio_channel_config_t;

typedef struct {
    const dev_gpio_channel_config_t *channels;
    uint16_t                         channel_count;
} dev_gpio_config_t;
```

### 4.4 Public API with Per-API Behavior

Each API documents the exact validation order, port calls, state changes, and error returns.

#### 4.4.1 dev_gpio_init

```
dev_err_t dev_gpio_init(const dev_gpio_config_t *config);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If already initialized | DEV_ERR_ALREADY_INITIALIZED |
| 2 | If `config` is NULL | DEV_ERR_NULL_PTR |
| 3 | If `config->channels` is NULL | DEV_ERR_NULL_PTR |
| 4 | If `config->channel_count == 0` | DEV_ERR_INVALID_ARG |
| 5 | If `config->channel_count > DEV_GPIO_CFG_MAX_CHANNELS` | DEV_ERR_INVALID_ARG |
| 6 | For each channel: validate channel ID `< DEV_GPIO_CFG_MAX_CHANNELS` | DEV_ERR_INVALID_ARG |
| 7 | For each channel: validate `direction` is a known enum value | DEV_ERR_INVALID_ARG |
| 8 | For each channel: validate `pull` is a known enum value | DEV_ERR_INVALID_ARG |
| 9 | For each channel: validate `interrupt` is a known enum value | DEV_ERR_INVALID_ARG |
| 10 | If `DEV_GPIO_CFG_VALIDATE_DUPLICATES`: detect duplicate channel IDs | DEV_ERR_CONFIG |
| 11 | Call `dev_gpio_port_init(config)` | DEV_ERR_HW_FAILURE |
| 12 | For each channel: call `dev_gpio_port_config_channel(&channels[i])` | DEV_ERR_HW_FAILURE |
| 13 | On port config failure: call `dev_gpio_port_deinit()` for cleanup, remain UNINITIALIZED | — |
| 14 | Initialize internal callback tables from config | — |
| 15 | Set state to INITIALIZED | — |
| 16 | Return DEV_OK | — |

#### 4.4.2 dev_gpio_deinit

```
dev_err_t dev_gpio_deinit(void);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | For each channel: disable interrupt via port | (best-effort, error logged but not blocking) |
| 3 | Clear all callback table entries to NULL | — |
| 4 | Call `dev_gpio_port_deinit()` | DEV_ERR_HW_FAILURE |
| 5 | Clear state to UNINITIALIZED | — |
| 6 | Return DEV_OK | — |

#### 4.4.3 dev_gpio_read

```
dev_err_t dev_gpio_read(dev_gpio_channel_t channel, dev_gpio_level_t *level);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | If `level` is NULL | DEV_ERR_NULL_PTR |
| 3 | Find channel → config index; if not found | DEV_ERR_INVALID_ARG |
| 4 | Call `dev_gpio_port_read(channel, level)` | DEV_ERR_HW_FAILURE |
| 5 | Write `*level` only if port read succeeds | — |
| 6 | Return DEV_OK | — |

#### 4.4.4 dev_gpio_write

```
dev_err_t dev_gpio_write(dev_gpio_channel_t channel, dev_gpio_level_t level);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | If `level` is not LOW or HIGH | DEV_ERR_INVALID_ARG |
| 3 | Find channel → config index; if not found | DEV_ERR_INVALID_ARG |
| 4 | If channel direction is INPUT | DEV_ERR_INVALID_STATE |
| 5 | Call `dev_gpio_port_write(channel, level)` | DEV_ERR_HW_FAILURE |
| 6 | Return DEV_OK | — |

#### 4.4.5 dev_gpio_toggle

```
dev_err_t dev_gpio_toggle(dev_gpio_channel_t channel);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | Find channel → config index; if not found | DEV_ERR_INVALID_ARG |
| 3 | If channel direction is INPUT | DEV_ERR_INVALID_STATE |
| 4 | Call `dev_gpio_port_toggle(channel)` | DEV_ERR_HW_FAILURE |
| 5 | Return DEV_OK | — |

#### 4.4.6 dev_gpio_set_direction

```
dev_err_t dev_gpio_set_direction(dev_gpio_channel_t channel, dev_gpio_direction_t direction);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | If `direction` is not a known enum value | DEV_ERR_INVALID_ARG |
| 3 | Find channel → config index; if not found | DEV_ERR_INVALID_ARG |
| 4 | Call `dev_gpio_port_set_direction(channel, direction)` | DEV_ERR_HW_FAILURE |
| 5 | If port returns DEV_ERR_NOT_SUPPORTED, propagate | DEV_ERR_NOT_SUPPORTED |
| 6 | Update internal direction tracking | — |
| 7 | Return DEV_OK | — |

#### 4.4.7 dev_gpio_set_pull

```
dev_err_t dev_gpio_set_pull(dev_gpio_channel_t channel, dev_gpio_pull_t pull);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | If `pull` is not a known enum value | DEV_ERR_INVALID_ARG |
| 3 | Find channel → config index; if not found | DEV_ERR_INVALID_ARG |
| 4 | Call `dev_gpio_port_set_pull(channel, pull)` | DEV_ERR_HW_FAILURE |
| 5 | If port returns DEV_ERR_NOT_SUPPORTED, propagate | DEV_ERR_NOT_SUPPORTED |
| 6 | Return DEV_OK | — |

#### 4.4.8 dev_gpio_config_interrupt

```
dev_err_t dev_gpio_config_interrupt(dev_gpio_channel_t channel, dev_gpio_intr_type_t interrupt);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | If `interrupt` is not a known enum value | DEV_ERR_INVALID_ARG |
| 3 | Find channel → config index; if not found | DEV_ERR_INVALID_ARG |
| 4 | Call `dev_gpio_port_config_interrupt(channel, interrupt)` | DEV_ERR_HW_FAILURE |
| 5 | If port returns DEV_ERR_NOT_SUPPORTED, propagate | DEV_ERR_NOT_SUPPORTED |
| 6 | Update internal interrupt config tracking | — |
| 7 | Return DEV_OK | — |

#### 4.4.9 dev_gpio_register_callback

```
dev_err_t dev_gpio_register_callback(dev_gpio_channel_t channel,
                                     dev_gpio_isr_callback_t callback, void *user_arg);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | Find channel → config index; if not found | DEV_ERR_INVALID_ARG |
| 3 | Write `callback` and `user_arg` into common driver's static tables | — |
| 4 | Return DEV_OK | — |

Note: `callback` is allowed to be NULL (clears the callback). Interrupt must be enabled
separately via `dev_gpio_enable_interrupt()`.

#### 4.4.10 dev_gpio_enable_interrupt

```
dev_err_t dev_gpio_enable_interrupt(dev_gpio_channel_t channel);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | Find channel → config index; if not found | DEV_ERR_INVALID_ARG |
| 3 | Call `dev_gpio_port_enable_interrupt(channel)` | DEV_ERR_HW_FAILURE |
| 4 | Mark interrupt as enabled in internal state | — |
| 5 | Return DEV_OK | — |

#### 4.4.11 dev_gpio_disable_interrupt

```
dev_err_t dev_gpio_disable_interrupt(dev_gpio_channel_t channel);
```

| Step | Action | Error on failure |
|------|--------|------------------|
| 1 | If not initialized | DEV_ERR_NOT_INITIALIZED |
| 2 | Find channel → config index; if not found | DEV_ERR_INVALID_ARG |
| 3 | Call `dev_gpio_port_disable_interrupt(channel)` | DEV_ERR_HW_FAILURE |
| 4 | Mark interrupt as disabled in internal state | — |
| 5 | Return DEV_OK | — |

Safe to call even if already disabled (idempotent).

#### 4.4.12 dev_gpio_is_initialized

```
bool dev_gpio_is_initialized(void);
```

Returns `true` if the driver is in INITIALIZED state.

#### 4.4.13 dev_gpio_dispatch_isr (internal, called by port layer)

```
void dev_gpio_dispatch_isr(dev_gpio_channel_t channel);
```

| Step | Action |
|------|--------|
| 1 | Find channel → config index; if not found, return |
| 2 | If interrupt not enabled for this channel, return |
| 3 | If callback is NULL, return |
| 4 | Invoke `callback(channel, user_arg)` |

This function is ISR-safe: no blocking, no allocation, no port calls.

### 4.5 Documentation Requirement for Public API

Each public API function must have a Doxygen-style comment covering:
- **@brief** — purpose
- **@param** — each parameter
- **@return** — all possible return values
- **@note** — initialization requirement
- **@note** — ISR safety
- **@note** — reentrancy
- **@note** — error behavior

### 4.6 Port Interface (dev_gpio_port.h)

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

Port rules:
- Port does NOT own callbacks. It calls `dev_gpio_dispatch_isr()` from its ISR handlers.
- Port does NOT track interrupt enable state. The common driver does.
- Port maps vendor errors to `dev_err_t`.
- Unsupported features return `DEV_ERR_NOT_SUPPORTED`.
- Port is allowed to include vendor HAL headers.

### 4.7 Mock Port

#### 4.7.1 Internal State

All arrays are indexed by **config index** (the position in `config->channels[]`),
NOT by raw channel ID. The mock port stores the channel count internally and maps
channel → index on each call.

```c
static dev_gpio_level_t      m_levels[DEV_GPIO_CFG_MAX_CHANNELS];
static dev_gpio_direction_t  m_directions[DEV_GPIO_CFG_MAX_CHANNELS];
static dev_gpio_pull_t       m_pulls[DEV_GPIO_CFG_MAX_CHANNELS];
static dev_gpio_intr_type_t  m_interrupts[DEV_GPIO_CFG_MAX_CHANNELS];
static bool                  m_interrupt_enabled[DEV_GPIO_CFG_MAX_CHANNELS];
static uint16_t              m_channel_count;
static bool                  m_initialized;
```

#### 4.7.2 Error Injection API

The mock port exposes test helpers for negative testing:

```c
/* Error injection: make a specific port operation return an error */
void dev_gpio_port_mock_set_error(dev_err_t error);  /* all subsequent calls fail */
void dev_gpio_port_mock_clear_error(void);           /* clear injected error */

/* ISR simulation: invoke the common ISR dispatch as if hardware triggered */
void dev_gpio_port_mock_trigger_isr(dev_gpio_channel_t channel);

/* State inspection for test assertions */
dev_gpio_level_t     dev_gpio_port_mock_get_level(dev_gpio_channel_t channel);
dev_gpio_direction_t dev_gpio_port_mock_get_direction(dev_gpio_channel_t channel);
dev_gpio_pull_t      dev_gpio_port_mock_get_pull(dev_gpio_channel_t channel);
bool                 dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_channel_t channel);
```

Error injection behavior:
- When an error is injected, all subsequent port operations return that error.
- `dev_gpio_port_mock_clear_error()` restores normal operation.
- Port operations check the injected error BEFORE any validation, so even valid
  calls fail with the injected error — this tests the common driver's error
  propagation path.

### 4.8 Configuration (dev_gpio_cfg.h)

```c
#define DEV_GPIO_CFG_MAX_CHANNELS          (32U)
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)
#define DEV_GPIO_CFG_VALIDATE_DUPLICATES   (1U)
#define DEV_GPIO_CFG_ENABLE_RUNTIME_CHECKS (1U)
```

### 4.9 Validation Summary

The following validations are required across the module:

| What | Check | Error |
|------|-------|-------|
| Config pointer | != NULL | DEV_ERR_NULL_PTR |
| Channels pointer | != NULL | DEV_ERR_NULL_PTR |
| channel_count | > 0 | DEV_ERR_INVALID_ARG |
| channel_count | ≤ DEV_GPIO_CFG_MAX_CHANNELS | DEV_ERR_INVALID_ARG |
| Each channel ID | < DEV_GPIO_CFG_MAX_CHANNELS | DEV_ERR_INVALID_ARG |
| Duplicate channel IDs | (if VALIDATE_DUPLICATES enabled) | DEV_ERR_CONFIG |
| level parameter | LOW or HIGH | DEV_ERR_INVALID_ARG |
| direction parameter | known enum value | DEV_ERR_INVALID_ARG |
| pull parameter | known enum value | DEV_ERR_INVALID_ARG |
| interrupt parameter | known enum value | DEV_ERR_INVALID_ARG |
| Output pointer (read) | != NULL | DEV_ERR_NULL_PTR |
| Write to input channel | direction != INPUT | DEV_ERR_INVALID_STATE |
| Toggle input channel | direction != INPUT | DEV_ERR_INVALID_STATE |
| Init state | not already initialized | DEV_ERR_ALREADY_INITIALIZED |
| Runtime state | initialized | DEV_ERR_NOT_INITIALIZED |
| Port init failure | cleanup + remain UNINITIALIZED | DEV_ERR_HW_FAILURE |

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

Example config struct:
```c
static const dev_gpio_channel_config_t m_channels[] = {
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
        .callback      = NULL,
        .callback_arg  = NULL,
    },
};

const dev_gpio_config_t g_dev_gpio_config = {
    .channels      = m_channels,
    .channel_count = DEV_ARRAY_SIZE(m_channels),
};
```

## 6. Build Integration

### 6.1 Host Mock Build

Standalone CMakeLists.txt compiling `dev_common`, `dev_gpio`, `mock port`,
`board_mock`, and test files into a host executable. Uses standard gcc/clang on Linux.

```cmake
add_executable(gpio_test_host
    drivers/dev_common/src/dev_assert.c
    drivers/dev_gpio/src/dev_gpio.c
    drivers/dev_gpio/port/mock/dev_gpio_port_mock.c
    boards/board_mock/dev_gpio_board_cfg.c
    tests/dev_gpio/test_gpio.c
)
target_include_directories(gpio_test_host PRIVATE
    drivers/dev_common/include
    drivers/dev_gpio/include
    drivers/dev_gpio/port/mock
    boards/board_mock
)
```

### 6.2 Target STM32 Build (future)

Integrates into existing `CMakeLists.txt`:
- Add `drivers/dev_common/include`, `drivers/dev_gpio/include`, `boards/board_stm32h743`
  to `target_include_directories`
- Add `.c` files to `target_sources`
- STM32 port `.c` files include `stm32h7xx_hal.h` ONLY in the port layer

## 7. Test Plan

All tests run against the mock port on host. 26 test cases:

| # | Test | Expected |
|---|------|----------|
| 1 | init with valid config | DEV_OK, is_initialized() == true |
| 2 | init with NULL config | DEV_ERR_NULL_PTR |
| 3 | init with NULL channels | DEV_ERR_NULL_PTR |
| 4 | init with channel_count=0 | DEV_ERR_INVALID_ARG |
| 5 | init with channel_count > MAX_CHANNELS | DEV_ERR_INVALID_ARG |
| 6 | init with invalid channel ID (≥ MAX_CHANNELS) | DEV_ERR_INVALID_ARG |
| 7 | init with invalid direction enum | DEV_ERR_INVALID_ARG |
| 8 | init with invalid pull enum | DEV_ERR_INVALID_ARG |
| 9 | init with invalid interrupt enum | DEV_ERR_INVALID_ARG |
| 10 | double init | DEV_ERR_ALREADY_INITIALIZED |
| 11 | duplicate channel detection | DEV_ERR_CONFIG |
| 12 | port init failure propagates | DEV_ERR_HW_FAILURE |
| 13 | port channel config failure + cleanup | DEV_ERR_HW_FAILURE, state stays UNINITIALIZED |
| 14 | read before init | DEV_ERR_NOT_INITIALIZED |
| 15 | write before init | DEV_ERR_NOT_INITIALIZED |
| 16 | read with NULL level pointer | DEV_ERR_NULL_PTR |
| 17 | read from unknown channel | DEV_ERR_INVALID_ARG |
| 18 | write with invalid level | DEV_ERR_INVALID_ARG |
| 19 | write to input-only channel | DEV_ERR_INVALID_STATE |
| 20 | toggle input-only channel | DEV_ERR_INVALID_STATE |
| 21 | read from output channel | DEV_OK, returns default_level |
| 22 | write then read output | DEV_OK, level matches |
| 23 | toggle output | DEV_OK, level flips |
| 24 | set pull mode | DEV_OK |
| 25 | register callback | DEV_OK |
| 26 | enable interrupt, mock trigger ISR | callback invoked with correct channel+arg |
| 27 | disable interrupt | DEV_OK, callback not invoked on trigger |
| 28 | deinit | DEV_OK, is_initialized() == false |
| 29 | reinit after deinit | DEV_OK |
| 30 | unsupported intr type | DEV_ERR_NOT_SUPPORTED |
| 31 | unsupported pull mode | DEV_ERR_NOT_SUPPORTED |
| 32 | mock error injection: port write fails | DEV_ERR_HW_FAILURE propagated |
| 33 | mock error injection: port read fails | DEV_ERR_HW_FAILURE, *level unchanged |

## 8. MISRA-C Compliance

- No dynamic memory allocation
- No recursion
- No unbounded loops (all loops bounded by channel_count ≤ DEV_GPIO_CFG_MAX_CHANNELS)
- No magic numbers (all constants named in cfg.h)
- Fixed-width integer types throughout
- `bool` for boolean values
- Private functions declared `static`
- Macros use `do { } while (false)`, evaluate args once
- No `goto`, no `continue`
- Vendor code isolated in port layer

## 9. Definition of Done

- [ ] 18 source files created
- [ ] All public APIs documented with Doxygen comments (purpose, params, returns, init req, ISR safety, reentrancy, error behavior)
- [ ] Mock port compiles and runs on host
- [ ] All 33 test cases pass
- [ ] No vendor headers in public APIs or common logic
- [ ] No magic numbers
- [ ] `DEV_OK` equals 0
- [ ] All error paths return specific `dev_err_t` values
