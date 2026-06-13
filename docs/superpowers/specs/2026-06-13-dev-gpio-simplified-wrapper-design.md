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
dev_gpio.c  (common wrapper: validate pin, dispatch to port)
   │  dev_gpio_port_output(pin, level)
   ▼
dev_gpio_port_<target>.c  (pin mapping table + vendor HAL calls)
   │  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, ...)
   ▼
Vendor HAL
```

No `boards/` folder. Pin mapping lives inside the selected port's `.c` file.

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
- `docs/dev_gpio/README.md` — new documentation
- Root `README.md` — updated with documentation index

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

/* Logical pin IDs — project-specific */
#define DEV_GPIO_LED_STATUS       ((dev_gpio_pin_t)0U)
#define DEV_GPIO_BUTTON_USER      ((dev_gpio_pin_t)1U)
```

Pin IDs live here — one file per project. No board folder.

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

### API Behavior

**dev_gpio_init:**
- Calls `dev_gpio_port_init()` (port enables clocks)
- Sets state to INITIALIZED
- If already initialized → `DEV_ERR_ALREADY_INITIALIZED`
- If port init fails → `DEV_ERR_HW_FAILURE`

**dev_gpio_output:** Configures pin as output, default LOW.

**dev_gpio_output_level:** Configures pin as output with explicit initial level (avoids glitches).

**dev_gpio_input / input_pullup / input_pulldown:** Configure pin as input with specified pull.

**dev_gpio_read:** Reads into local temp; `*level` written only on success.

**dev_gpio_write:** Validates pin exists, validates level enum, dispatches to port.

**dev_gpio_interrupt:** Combines config + callback registration in one call. Callback may be NULL only if intr is DISABLE.

### Validation and Errors

| Condition | Error |
|-----------|-------|
| Pin ID not in port's mapping table | `DEV_ERR_INVALID_ARG` |
| Null pointer (read level, callback) | `DEV_ERR_NULL_PTR` |
| Not initialized | `DEV_ERR_NOT_INITIALIZED` |
| Already initialized | `DEV_ERR_ALREADY_INITIALIZED` |
| Unsupported pull/intr | `DEV_ERR_NOT_SUPPORTED` |
| Port failure | `DEV_ERR_HW_FAILURE` |

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

## 10. Callback Ownership

Common `dev_gpio.c` owns a static callback table:

```c
typedef struct {
    dev_gpio_pin_t      pin;
    dev_gpio_callback_t callback;
    void               *user_arg;
    bool                enabled;
} dev_gpio_callback_entry_t;

static dev_gpio_callback_entry_t g_callbacks[DEV_GPIO_CFG_MAX_PINS];
```

`dev_gpio_interrupt()` stores callback + arg. `dev_gpio_dispatch_isr()` looks up pin, checks enabled, calls callback. Vendor ISR handlers call `dev_gpio_dispatch_isr(pin)`.

## 11. Port Pin Mapping

### STM32

```c
typedef struct {
    dev_gpio_pin_t pin_id;
    GPIO_TypeDef  *port;
    uint16_t       hal_pin;
} dev_gpio_hw_pin_t;

static const dev_gpio_hw_pin_t s_gpio_map[] = {
    { DEV_GPIO_LED_STATUS,  GPIOB, GPIO_PIN_0 },
    { DEV_GPIO_BUTTON_USER, GPIOC, GPIO_PIN_13 },
};
```

`dev_gpio_port_init()` enables clocks for all ports in the map.

### ESP32 (stub)

```c
typedef struct {
    dev_gpio_pin_t pin_id;
    int            gpio_num;
} dev_gpio_hw_pin_t;
```

### nRF52 (stub)

```c
typedef struct {
    dev_gpio_pin_t pin_id;
    uint32_t       pin_number;
} dev_gpio_hw_pin_t;
```

## 12. Common Driver (`dev_gpio.c`)

Thin wrapper. Each public API:
1. Checks `g_initialized` (return `DEV_ERR_NOT_INITIALIZED`)
2. Validates pin (iterate port's `s_gpio_map`, return `DEV_ERR_INVALID_ARG` if not found)
3. For write: validates level enum
4. Dispatches to matching `dev_gpio_port_*()` call
5. Returns port result mapped through `dev_gpio_map_port_error()`

The common driver does NOT own the pin map — it calls through to the port for pin validation. Actually, since the common driver doesn't know the pin map (it's in the port), validation must either:
- (A) be in the port (port returns `DEV_ERR_INVALID_ARG` for unknown pins), or
- (B) the port exposes a `dev_gpio_port_pin_is_valid(pin)` function

**Decision: Option A.** Port validates pins internally. Common driver just dispatches and maps errors. This keeps the common driver truly thin — it doesn't need to know about hardware pin mapping at all.

Error mapping:
- `DEV_OK` → `DEV_OK`
- `DEV_ERR_NOT_SUPPORTED` → `DEV_ERR_NOT_SUPPORTED` (preserved)
- Any other non-OK → `DEV_ERR_HW_FAILURE`

## 13. Mock Port

Simplified mock. Internal arrays indexed by pin ID (must be < MAX_PINS).

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

## 14. Build Integration

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

## 15. Documentation

- `docs/dev_gpio/README.md` — full reference
- Root `README.md` — updated index linking to docs

## 16. Definition of Done

- [ ] All old board folders removed
- [ ] `dev_gpio` rewritten with simplified API (17 functions)
- [ ] `dev_common` unchanged
- [ ] STM32 port rewritten (pin map in .c)
- [ ] Mock port rewritten
- [ ] ESP32 stub created
- [ ] nRF52 stub created
- [ ] `main.c` updated to new API
- [ ] CMakeLists.txt updated with port selection
- [ ] All 36 tests pass against mock port
- [ ] No vendor headers in public headers
- [ ] `docs/dev_gpio/README.md` written
- [ ] Root `README.md` updated
