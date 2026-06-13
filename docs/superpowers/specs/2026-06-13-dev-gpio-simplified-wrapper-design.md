# dev_gpio Simplified Wrapper — Design Specification

**Date:** 2026-06-13
**Status:** Ready for implementation
**Approach:** Full rewrite — remove board folders, simplify API, pin map in port

## 1. Purpose

Replace the current configuration-heavy `dev_gpio` driver with a simplified wrapper-style API. The goal is fast porting across STM32, ESP32, nRF52 without board folders or large config structs.

## 2. Architecture

```
Application (main.c)
   │  #include "dev_gpio.h"
   │  dev_gpio_init();
   │  dev_gpio_output(DEV_GPIO_LED_STATUS);
   ▼
dev_gpio.c  (thin wrapper: lifecycle, callback table, error mapping, dispatch)
   │  dev_gpio_port_output(pin, level)
   ▼
dev_gpio_port_<target>.c  (pin mapping table + pin validation + vendor HAL calls)
   │  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, ...)
   ▼
Vendor HAL
```

No `boards/` folder. Pin mapping lives inside the selected port's `.c` file.
Pin validation has two levels:
- **Common driver** checks `pin < DEV_GPIO_CFG_MAX_PINS` (array bounds for callback table).
- **Port** checks whether the in-range pin is actually mapped on this hardware.

### Logical Pin ID Rule

All logical pin IDs MUST be dense values from 0 to `DEV_GPIO_CFG_MAX_PINS - 1`. This allows:
- Mock port to use direct array indexing
- Callback table to use direct array indexing
- No mapping table lookup in the common layer

This is a deliberate simplification: the port's internal `s_gpio_map[]` maps logical IDs to hardware pins, but the common layer treats pin IDs as array indices.

## 3. What Gets Removed

- `boards/board_mock/` — entire directory
- `boards/board_stm32h743/` — entire directory
- `drivers/dev_gpio/include/dev_gpio.h` — replaced with simplified version
- `drivers/dev_gpio/include/dev_gpio_types.h` — replaced (renamed types)
- `drivers/dev_gpio/include/dev_gpio_cfg.h` — replaced (pin IDs added)
- `drivers/dev_gpio/include/dev_gpio_port.h` — replaced (simplified interface)
- `drivers/dev_gpio/src/dev_gpio.c` — replaced (thinner wrapper)
- `drivers/dev_gpio/port/mock/*` — rewritten
- `drivers/dev_gpio/port/stm32/*` — rewritten (pin map inside .c)
- `tests/dev_gpio/*` — rewritten for new API
- `Core/Src/main.c` — updated to new API
- `CMakeLists.txt` — remove board paths, add port selection
- `docs/dev_gpio_library.md` — replaced

## 4. What Stays Unchanged

- `drivers/dev_common/` — all files unchanged

## 5. New Files Created

- `drivers/dev_gpio/port/esp32/dev_gpio_port_esp32.c` — stub
- `drivers/dev_gpio/port/esp32/dev_gpio_port_esp32.h` — stub
- `drivers/dev_gpio/port/nrf52/dev_gpio_port_nrf52.c` — stub
- `drivers/dev_gpio/port/nrf52/dev_gpio_port_nrf52.h` — stub
- `docs/dev_common/README.md` — library reference
- `docs/dev_gpio/README.md` — library reference
- Root `README.md` — updated documentation index

## 6. Types (`dev_gpio_types.h`)

```c
typedef uint16_t dev_gpio_pin_t;

typedef enum { DEV_GPIO_LEVEL_LOW = 0, DEV_GPIO_LEVEL_HIGH = 1 } dev_gpio_level_t;
typedef enum { DEV_GPIO_PULL_NONE = 0, DEV_GPIO_PULL_UP, DEV_GPIO_PULL_DOWN } dev_gpio_pull_t;

typedef enum {
    DEV_GPIO_INTR_DISABLE = 0,
    DEV_GPIO_INTR_RISING_EDGE,
    DEV_GPIO_INTR_FALLING_EDGE,
    DEV_GPIO_INTR_BOTH_EDGES,
    DEV_GPIO_INTR_LOW_LEVEL,
    DEV_GPIO_INTR_HIGH_LEVEL
} dev_gpio_intr_t;

typedef void (*dev_gpio_callback_t)(dev_gpio_pin_t pin, void *user_arg);
```

## 7. Configuration (`dev_gpio_cfg.h`)

```c
#define DEV_GPIO_CFG_MAX_PINS              (32U)
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)
#define DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED (1U)

/* Logical pin IDs — dense, 0..MAX_PINS-1. Pin count = highest ID + 1 */
#define DEV_GPIO_LED_STATUS       ((dev_gpio_pin_t)0U)
#define DEV_GPIO_BUTTON_USER      ((dev_gpio_pin_t)1U)
#define DEV_GPIO_CFG_PIN_COUNT        (2U)
```

Pin IDs live here — one file per project. No board folder.

### Feature Switch: `DEV_GPIO_CFG_INTERRUPT_ENABLED`

| Value | Behavior |
|-------|----------|
| `1U` | `dev_gpio_interrupt()`, `interrupt_enable()`, `interrupt_disable()` fully functional. Callback table allocated and active. `dev_gpio_dispatch_isr()` checks enabled state and invokes callbacks. |
| `0U` | Above APIs compile and link but return `DEV_ERR_NOT_SUPPORTED`. `dev_gpio_dispatch_isr()` compiles as a no-op. Callback table may be compiled out or left unused. |

## 8. Public API (`dev_gpio.h`)

```c
/* Lifecycle */
dev_err_t dev_gpio_init(void);
dev_err_t dev_gpio_deinit(void);
bool     dev_gpio_is_initialized(void);

/* Input configuration */
dev_err_t dev_gpio_input(dev_gpio_pin_t pin);
dev_err_t dev_gpio_input_pullup(dev_gpio_pin_t pin);
dev_err_t dev_gpio_input_pulldown(dev_gpio_pin_t pin);

/* Output configuration */
dev_err_t dev_gpio_output(dev_gpio_pin_t pin);
dev_err_t dev_gpio_output_level(dev_gpio_pin_t pin, dev_gpio_level_t level);

/* Runtime operations */
dev_err_t dev_gpio_read(dev_gpio_pin_t pin, dev_gpio_level_t *level);
dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level);
dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin);
dev_err_t dev_gpio_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull);

/* Convenience */
dev_err_t dev_gpio_high(dev_gpio_pin_t pin);
dev_err_t dev_gpio_low(dev_gpio_pin_t pin);

/* Interrupts */
dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t callback, void *user_arg);
dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin);
dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin);
```

### Per-API Behavior

| API | Not Init | Invalid Arg | Null Ptr | Unsupported | Port Fail | Notes |
|-----|----------|-------------|----------|-------------|-----------|-------|
| `init()` | ALREADY_INIT | — | — | — | HW_FAILURE | Calls port_init, sets state on success |
| `deinit()` | NOT_INIT | — | — | — | HW_FAILURE | Clears callbacks, forces UNINIT |
| `is_initialized()` | — | — | — | — | — | ISR-safe |
| `input(pin)` | NOT_INIT | INVALID_ARG | — | NOT_SUPPORTED | HW_FAILURE | Dispatches to port_input(pin, NONE) |
| `input_pullup(pin)` | NOT_INIT | INVALID_ARG | — | NOT_SUPPORTED | HW_FAILURE | Dispatches to port_input(pin, UP) |
| `input_pulldown(pin)` | NOT_INIT | INVALID_ARG | — | NOT_SUPPORTED | HW_FAILURE | Dispatches to port_input(pin, DOWN) |
| `output(pin)` | NOT_INIT | INVALID_ARG | — | NOT_SUPPORTED | HW_FAILURE | Default LOW |
| `output_level(pin, lvl)` | NOT_INIT | INVALID_ARG | — | NOT_SUPPORTED | HW_FAILURE | lvl not LOW/HIGH → INVALID_ARG |
| `read(pin, *lvl)` | NOT_INIT | INVALID_ARG | NULL_PTR | — | HW_FAILURE | Reads into local temp; *lvl unchanged on fail |
| `write(pin, lvl)` | NOT_INIT | INVALID_ARG | — | — | HW_FAILURE | Invalid level enum → INVALID_ARG |
| `toggle(pin)` | NOT_INIT | INVALID_ARG | — | — | HW_FAILURE | — |
| `set_pull(pin, pull)` | NOT_INIT | INVALID_ARG | — | NOT_SUPPORTED | HW_FAILURE | Invalid pull enum → INVALID_ARG |
| `high(pin)` | NOT_INIT | INVALID_ARG | — | — | HW_FAILURE | Calls write(pin, HIGH) |
| `low(pin)` | NOT_INIT | INVALID_ARG | — | — | HW_FAILURE | Calls write(pin, LOW) |
| `interrupt(pin, intr, cb, arg)` | NOT_INIT | INVALID_ARG | NULL_PTR* | NOT_SUPPORTED | HW_FAILURE | *NULL cb allowed only if intr==DISABLE. Calls port first, stores cb+arg only on success. When INTERRUPT_ENABLED==0U: returns NOT_SUPPORTED, no state change. |
| `interrupt_enable(pin)` | NOT_INIT | INVALID_ARG/INVALID_STATE | — | NOT_SUPPORTED | HW_FAILURE | INVALID_STATE if callback is NULL or intr is DISABLE. Marks enabled BEFORE port call; rolls back on fail. When INTERRUPT_ENABLED==0U: returns NOT_SUPPORTED. |
| `interrupt_disable(pin)` | NOT_INIT | INVALID_ARG | — | — | HW_FAILURE | Marks disabled BEFORE port call; idempotent. When INTERRUPT_ENABLED==0U: returns NOT_SUPPORTED. |

**Pin validation:** `INVALID_ARG` means pin >= `DEV_GPIO_CFG_MAX_PINS` or not found in port's mapping table. Common driver checks `pin < MAX_PINS` before dispatching (common owns the dense-ID rule). Port additionally rejects pins not in its internal map.

## 9. Port Interface (`dev_gpio_port.h`)

```c
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
```

Plus the common-driver ISR dispatch:

```c
void dev_gpio_dispatch_isr(dev_gpio_pin_t pin);   /* PORT-ONLY */
```

## 10. Error Mapping (Common Driver)

```c
static dev_err_t dev_gpio_map_port_error(dev_err_t port_err)
{
    if (port_err == DEV_OK)                 { return DEV_OK; }
    if (port_err == DEV_ERR_INVALID_ARG)    { return DEV_ERR_INVALID_ARG; }
    if (port_err == DEV_ERR_NULL_PTR)       { return DEV_ERR_NULL_PTR; }
    if (port_err == DEV_ERR_NOT_SUPPORTED)  { return DEV_ERR_NOT_SUPPORTED; }
    return DEV_ERR_HW_FAILURE;
}
```

Preserved: `DEV_OK`, `DEV_ERR_INVALID_ARG`, `DEV_ERR_NULL_PTR`, `DEV_ERR_NOT_SUPPORTED`. All other port errors → `DEV_ERR_HW_FAILURE`.

## 11. Callback Table (Common Driver)

Common `dev_gpio.c` owns a static callback table indexed directly by pin ID:

```c
typedef struct {
    dev_gpio_callback_t callback;
    void               *user_arg;
    dev_gpio_intr_t     intr;       /* configured interrupt mode */
    bool                enabled;
} dev_gpio_callback_entry_t;

static dev_gpio_callback_entry_t g_callbacks[DEV_GPIO_CFG_MAX_PINS];
```

### Callback Table Semantics

**Initialization:** All entries zeroed at startup (static storage). `intr` defaults to `DISABLE` (0), `enabled` to `false`. `dev_gpio_init()` does NOT touch the callback table.

**`dev_gpio_interrupt(pin, intr, cb, arg)`:**
1. If `intr == DISABLE` and `cb == NULL`: set `enabled = false` FIRST. Call `port_interrupt(pin, DISABLE)`. On success, clear entry (cb=NULL, arg=NULL, intr=DISABLE). On port failure, return the error — entry stays `enabled=false` with `intr=DISABLE` regardless (safe: dispatch + enable both blocked).
2. If `intr != DISABLE` and `cb == NULL`: return `DEV_ERR_NULL_PTR`
3. If `intr != DISABLE` and `cb != NULL`: call `port_interrupt(pin, intr)` FIRST. On success, store `cb`, `arg`, and `intr` in `g_callbacks[pin]` (enabled stays false until `interrupt_enable`). On port failure, return the error WITHOUT modifying the callback table.

**`dev_gpio_interrupt_enable(pin)`:**
1. If `g_callbacks[pin].callback == NULL`: return `DEV_ERR_INVALID_STATE`
2. If `g_callbacks[pin].intr == DEV_GPIO_INTR_DISABLE`: return `DEV_ERR_INVALID_STATE`
3. Set `g_callbacks[pin].enabled = true`
4. Call `port_interrupt_enable(pin)`
5. If port fails: set `g_callbacks[pin].enabled = false`, return error

**`dev_gpio_interrupt_disable(pin)`:**
1. Set `g_callbacks[pin].enabled = false`
2. Call `port_interrupt_disable(pin)`
3. If port fails: enabled stays false (no rollback needed), return error

**`dev_gpio_deinit()`:**
1. For all pins: clear callback table entries (callback=NULL, arg=NULL, intr=DISABLE, enabled=false)
2. Call `port_deinit()`
3. Force state to UNINITIALIZED

Callbacks are fully cleared on deinit. After reinit, the application must call `dev_gpio_interrupt()` again to re-register callbacks before enabling interrupts. This avoids stale callbacks surviving across reinit with no corresponding hardware configuration.

**`dev_gpio_dispatch_isr(pin)`:**
1. If `pin >= MAX_PINS`: return
2. If `!g_callbacks[pin].enabled`: return
3. If `g_callbacks[pin].callback == NULL`: return
4. Call `g_callbacks[pin].callback(pin, g_callbacks[pin].user_arg)`

## 12. Common Driver (`dev_gpio.c`)

Thin wrapper. Each public API:
1. If not `g_initialized` → `DEV_ERR_NOT_INITIALIZED`
2. If `pin >= DEV_GPIO_CFG_MAX_PINS` → `DEV_ERR_INVALID_ARG`
3. Validate other arguments (level enum, null pointers) where applicable
4. Dispatch to matching `dev_gpio_port_*()` call
5. Map port result through `dev_gpio_map_port_error()`

The common driver checks `pin < MAX_PINS`. The port may ADDITIONALLY reject pins not in its internal table. This is not a contradiction — common rejects IDs outside the array range (preventing OOB access), port rejects IDs that are in range but not wired on this hardware.

## 13. Port Pin Mapping

### Logical Pin ID Contract

All pin IDs MUST be dense: 0, 1, 2, ... `DEV_GPIO_CFG_PIN_COUNT - 1`, all < `DEV_GPIO_CFG_MAX_PINS`. This contract is enforced by `dev_gpio_cfg.h` defining the IDs, not by any runtime code.

### STM32

```c
typedef struct {
    dev_gpio_pin_t pin_id;
    GPIO_TypeDef  *port;
    uint16_t       hal_pin;
} dev_gpio_hw_pin_t;

static const dev_gpio_hw_pin_t s_gpio_map[] = {
    [DEV_GPIO_LED_STATUS]  = { DEV_GPIO_LED_STATUS,  GPIOB, GPIO_PIN_0 },
    [DEV_GPIO_BUTTON_USER] = { DEV_GPIO_BUTTON_USER, GPIOC, GPIO_PIN_13 },
};
```

Designated initializers index by pin ID. `dev_gpio_port_init()` enables clocks for all ports in the map.

### ESP32 (stub)

```c
typedef struct {
    dev_gpio_pin_t pin_id;
    int            gpio_num;
} dev_gpio_hw_pin_t;

static const dev_gpio_hw_pin_t s_gpio_map[] = {
    [DEV_GPIO_LED_STATUS]  = { DEV_GPIO_LED_STATUS,  2 },
    [DEV_GPIO_BUTTON_USER] = { DEV_GPIO_BUTTON_USER, 0 },
};
```

### nRF52 (stub)

```c
typedef struct {
    dev_gpio_pin_t pin_id;
    uint32_t       pin_number;
} dev_gpio_hw_pin_t;

static const dev_gpio_hw_pin_t s_gpio_map[] = {
    [DEV_GPIO_LED_STATUS]  = { DEV_GPIO_LED_STATUS,  17U },
    [DEV_GPIO_BUTTON_USER] = { DEV_GPIO_BUTTON_USER, 13U },
};
```

## 14. Mock Port

Internal arrays indexed directly by pin ID (valid because pins are dense < MAX_PINS):

```c
static dev_gpio_level_t m_levels[DEV_GPIO_CFG_MAX_PINS];
static bool             m_is_output[DEV_GPIO_CFG_MAX_PINS];
```

Error injection:
```c
void dev_gpio_port_mock_set_error(dev_err_t error);
void dev_gpio_port_mock_clear_error(void);
void dev_gpio_port_mock_trigger_isr(dev_gpio_pin_t pin);
```

State inspection:
```c
dev_gpio_level_t dev_gpio_port_mock_get_level(dev_gpio_pin_t pin);
bool             dev_gpio_port_mock_is_output(dev_gpio_pin_t pin);
```

Mock validates `pin < MAX_PINS` internally and returns `DEV_ERR_INVALID_ARG` if out of range.

## 15. Build Integration

CMake with port selection:

```cmake
set(DEV_GPIO_PORT "stm32")

target_sources(${PROJECT_NAME} PRIVATE
    drivers/dev_common/src/dev_common.c
    drivers/dev_common/src/dev_assert.c
    drivers/dev_gpio/src/dev_gpio.c
    drivers/dev_gpio/port/${DEV_GPIO_PORT}/dev_gpio_port_${DEV_GPIO_PORT}.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    drivers/dev_common/include
    drivers/dev_gpio/include
    drivers/dev_gpio/port/${DEV_GPIO_PORT}
)
```

One port per build. Host tests use `DEV_GPIO_PORT=mock`.

## 16. Documentation

- `docs/dev_common/README.md` — dev_common library reference
- `docs/dev_gpio/README.md` — dev_gpio wrapper reference with porting guide
- Root `README.md` — updated index linking to both docs

## 17. Test Plan

34 test cases for the simplified wrapper:

| # | Test | Expected |
|---|------|----------|
| 1 | init succeeds | DEV_OK, is_initialized() == true |
| 2 | double init | DEV_ERR_ALREADY_INITIALIZED |
| 3 | output then high | DEV_OK, level == HIGH |
| 4 | output then low | DEV_OK, level == LOW |
| 5 | output_level with HIGH | DEV_OK, level == HIGH (no glitch) |
| 6 | output_level with LOW | DEV_OK, level == LOW |
| 7 | input with pull-up, read LOW | DEV_OK (pull-up overridden by mock) |
| 8 | input_pullup | DEV_OK |
| 9 | input_pulldown | DEV_OK |
| 10 | write valid pin | DEV_OK, level matches |
| 11 | write invalid level | DEV_ERR_INVALID_ARG |
| 12 | read with NULL pointer | DEV_ERR_NULL_PTR |
| 13 | read from valid pin | DEV_OK, level returned |
| 14 | toggle output | DEV_OK, level flips |
| 15 | set_pull valid | DEV_OK |
| 16 | high convenience | DEV_OK, level == HIGH |
| 17 | low convenience | DEV_OK, level == LOW |
| 18 | interrupt register with callback | DEV_OK |
| 19 | interrupt with NULL callback + DISABLE | DEV_OK, callback cleared |
| 20 | interrupt with NULL callback + RISING | DEV_ERR_NULL_PTR |
| 21 | interrupt enable + trigger | callback invoked with correct pin+arg |
| 22 | interrupt disable + trigger | callback NOT invoked |
| 23 | deinit | DEV_OK, is_initialized() == false |
| 24 | reinit after deinit | DEV_OK |
| 25 | unknown pin ID (>= MAX_PINS) | DEV_ERR_INVALID_ARG |
| 26 | operation before init | DEV_ERR_NOT_INITIALIZED |
| 27 | mock error injection | DEV_ERR_HW_FAILURE propagated |
| 28 | enable interrupt + port fail + rollback | DEV_ERR_HW_FAILURE, enabled stays false |
| 29 | interrupt config with INTERRUPT_ENABLED=0U | DEV_ERR_NOT_SUPPORTED |
| 30 | enable/disable with INTERRUPT_ENABLED=0U | DEV_ERR_NOT_SUPPORTED |
| 31 | pin < MAX_PINS but absent from port map | DEV_ERR_INVALID_ARG |
| 32 | deinit clears callbacks (reinit, trigger ISR) | callback NOT invoked after reinit |
| 33 | interrupt_enable without prior callback registration | DEV_ERR_INVALID_STATE |
| 34 | interrupt(DISABLE) port fails; enable afterwards | DEV_ERR_INVALID_STATE (intr is DISABLE) |

## 18. Definition of Done

- [ ] All old board folders removed
- [ ] `dev_gpio` rewritten with simplified API (17 functions)
- [ ] `dev_common` unchanged
- [ ] STM32 port rewritten (pin map in .c)
- [ ] Mock port rewritten
- [ ] ESP32 stub created
- [ ] nRF52 stub created
- [ ] `main.c` updated to new API
- [ ] CMakeLists.txt updated with port selection
- [ ] All 34 tests pass against mock port
- [ ] All pin IDs are dense (0..DEV_GPIO_CFG_PIN_COUNT-1, all < DEV_GPIO_CFG_MAX_PINS)
- [ ] No vendor headers in public headers
- [ ] `docs/dev_common/README.md` written
- [ ] `docs/dev_gpio/README.md` written
- [ ] Root `README.md` updated
