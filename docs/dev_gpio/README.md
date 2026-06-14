# dev_gpio — GPIO Driver (X-Macro Pin Definition)

## 1. Overview

All pins are defined in **one place** — `dev_gpio_cfg.h`. From a single X-Macro, the compiler auto-generates:

- **Pin ID enum** (`DEV_GPIO_LED_GREEN`, `DEV_GPIO_LED_RED`, `DEV_GPIO_CFG_PIN_COUNT`)
- **Hardware map** (`s_gpio_map[]` — in the STM32 port `.c` file)

```
dev_gpio_cfg.h            ← ONE file: DEV_GPIO_PIN_LIST(X)
    │
    ├── enum auto-generated      DEV_GPIO_LED_GREEN=0, DEV_GPIO_LED_RED=1, PIN_COUNT=2
    ├── s_gpio_map[] auto-gen    [0]={GPIOB,PIN_0,OUTPUT}, [1]={GPIOB,PIN_1,OUTPUT}
    │
    ▼
Application                 Driver (thin wrapper)         STM32 Port (HAL)
──────────                  ────────────────────         ─────────────────
dev_gpio_write(LED, HIGH) → validates + dispatches    → HAL_GPIO_WritePin(GPIOB, PIN_0)
```

**No `#define` per pin needed.** No board folder. No manual map. One line per pin.

---

## 2. Quick Start

### 2.1 Define pins

Open `drivers/dev_gpio/include/dev_gpio_cfg.h`, edit `DEV_GPIO_PIN_LIST`:

```c
#define DEV_GPIO_PIN_LIST(X)                                                     \
    X(LED_GREEN, GPIOB, GPIO_PIN_0, DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE)    \
    X(LED_RED,   GPIOB, GPIO_PIN_1, DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE)
```

That's it. `DEV_GPIO_LED_GREEN`, `DEV_GPIO_LED_RED`, `DEV_GPIO_CFG_PIN_COUNT` are auto-generated as an enum.

### 2.2 Use in application

```c
#include "dev_gpio.h"

int main(void)
{
    dev_gpio_init();                                      // init all pins

    dev_gpio_high(DEV_GPIO_LED_GREEN);                    // LED on
    dev_gpio_low(DEV_GPIO_LED_GREEN);                     // LED off
    dev_gpio_toggle(DEV_GPIO_LED_RED);                    // toggle
}
```

---

## 3. Pin Definition — `dev_gpio_cfg.h`

### 3.1 The X-Macro (one place to edit)

```c
#define DEV_GPIO_PIN_LIST(X)                                                     \
    X(NAME,        PORT,  PIN,          MODE,                       PULL)
    X(LED_GREEN,   GPIOB, GPIO_PIN_0,   DEV_GPIO_MODE_OUTPUT,      DEV_GPIO_PULL_NONE)
    X(LED_RED,     GPIOB, GPIO_PIN_1,   DEV_GPIO_MODE_OUTPUT,      DEV_GPIO_PULL_NONE)
    X(BUTTON_USER, GPIOC, GPIO_PIN_13,  DEV_GPIO_MODE_INPUT_PULLUP, DEV_GPIO_PULL_UP)
```

| Field | Type | Description |
|-------|------|-------------|
| `NAME` | identifier | Becomes `DEV_GPIO_<NAME>` enum member |
| `PORT` | `GPIO_TypeDef *` | STM32 port: `GPIOA`, `GPIOB`, ... |
| `PIN` | `uint16_t` | STM32 pin: `GPIO_PIN_0` ... `GPIO_PIN_15` |
| `MODE` | `dev_gpio_mode_t` | `DEV_GPIO_MODE_INPUT`, `OUTPUT`, `INPUT_PULLUP`, `INPUT_PULLDOWN` |
| `PULL` | `dev_gpio_pull_t` | `DEV_GPIO_PULL_NONE`, `PULL_UP`, `PULL_DOWN` |

### 3.2 Auto-generated enum (you never write this)

```c
typedef enum {
    DEV_GPIO_LED_GREEN   = 0,    // from X(LED_GREEN, ...)
    DEV_GPIO_LED_RED     = 1,    // from X(LED_RED, ...)
    DEV_GPIO_BUTTON_USER = 2,    // from X(BUTTON_USER, ...)
    DEV_GPIO_CFG_PIN_COUNT = 3   // auto count
} dev_gpio_logical_pin_id_t;
```

### 3.3 Adding a pin — ONE LINE

```c
X(RELAY_1, GPIOA, GPIO_PIN_5, DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE)
```

Immediately available: `DEV_GPIO_RELAY_1`, `DEV_GPIO_CFG_PIN_COUNT` incremented, `s_gpio_map[]` updated.

---

## 4. Configuration

```c
#define DEV_GPIO_CFG_MAX_PINS              (32U)   // max pins supported
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)    // 1U = interrupt APIs compiled in
#define DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED (1U)    // 1U = validate init state + pin range
```

---

## 5. Public API — `dev_gpio.h`

### 5.1 Lifecycle

```c
dev_err_t dev_gpio_init(void);       // init all pins from X-Macro
dev_err_t dev_gpio_deinit(void);     // clear callbacks, force UNINIT
bool     dev_gpio_is_initialized(void); // ISR-safe
```

### 5.2 Input configuration

```c
dev_err_t dev_gpio_input(dev_gpio_pin_t pin);           // input, no pull
dev_err_t dev_gpio_input_pullup(dev_gpio_pin_t pin);    // input with pull-up
dev_err_t dev_gpio_input_pulldown(dev_gpio_pin_t pin);   // input with pull-down
```

### 5.3 Output configuration

```c
dev_err_t dev_gpio_output(dev_gpio_pin_t pin);                  // output, default LOW
dev_err_t dev_gpio_output_level(dev_gpio_pin_t pin, dev_gpio_level_t level); // output with initial level
```

### 5.4 Runtime operations

```c
dev_err_t dev_gpio_read(dev_gpio_pin_t pin, dev_gpio_level_t *level);  // read to *level
dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level);  // write HIGH/LOW
dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin);                         // flip output
dev_err_t dev_gpio_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull); // change pull
dev_err_t dev_gpio_high(dev_gpio_pin_t pin);  // convenience: write HIGH
dev_err_t dev_gpio_low(dev_gpio_pin_t pin);   // convenience: write LOW
```

### 5.5 Interrupts

```c
// Configure interrupt mode + register callback (does NOT enable yet)
dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t cb, void *arg);

dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin);   // enable NVIC
dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin);  // disable NVIC
```

### 5.6 Return values

| Condition | Error |
|-----------|-------|
| Success | `DEV_OK` |
| Not initialized | `DEV_ERR_NOT_INITIALIZED` |
| Invalid pin / enum | `DEV_ERR_INVALID_ARG` |
| NULL pointer | `DEV_ERR_NULL_PTR` |
| No callback registered | `DEV_ERR_INVALID_STATE` |
| Feature not supported | `DEV_ERR_NOT_SUPPORTED` |
| Port/HAL failure | `DEV_ERR_HW_FAILURE` |

---

## 6. Usage Examples

### 6.1 Blink two LEDs

```c
#include "dev_gpio.h"

int main(void)
{
    dev_gpio_init();
    for (;;) {
        dev_gpio_toggle(DEV_GPIO_LED_GREEN);
        dev_gpio_toggle(DEV_GPIO_LED_RED);
        HAL_Delay(500);
    }
}
```

### 6.2 Button interrupt → toggle LED

```c
static void btn_isr(dev_gpio_pin_t pin, void *arg)
{
    (void)pin; (void)arg;
    dev_gpio_toggle(DEV_GPIO_LED_GREEN);
}

int main(void)
{
    dev_gpio_init();
    dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_FALLING_EDGE, btn_isr, NULL);
    dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER);
    for (;;) {}
}
```

### 6.3 Wiring EXTI callback

In `Core/Src/stm32h7xx_it.c`:

```c
// Already handled by port/stm32/dev_gpio_port_stm32.c — nothing to add.
// The port defines HAL_GPIO_EXTI_Callback which calls dev_gpio_dispatch_isr().
```

### 6.4 Read an input

```c
dev_gpio_level_t lvl;
if (dev_gpio_read(DEV_GPIO_BUTTON_USER, &lvl) == DEV_OK) {
    if (lvl == DEV_GPIO_LEVEL_LOW) { /* button pressed */ }
}
```

---

## 7. Architecture

```
drivers/dev_gpio/
  include/
    dev_gpio_types.h        Portable types (pin_t, level_t, pull_t, intr_t, callback_t)
    dev_gpio_cfg.h           ★ X-Macro DEV_GPIO_PIN_LIST + auto enum + mode enum
    dev_gpio_port.h          Port interface (11 functions + dispatch_isr)
    dev_gpio.h               Full public API (17 functions)
  src/
    dev_gpio.c               Thin wrapper: validation, callback table, port dispatch
  port/
    mock/
      dev_gpio_port_mock.h   Error injection + state inspection
      dev_gpio_port_mock.c   In-memory simulation (host testing)
    stm32/
      dev_gpio_port_stm32.h  dev_gpio_hw_pin_t, includes stm32h7xx_hal.h
      dev_gpio_port_stm32.c  Auto-generated s_gpio_map[], HAL calls, EXTI ISR
```

- **`dev_gpio_cfg.h`** — NO vendor headers, host-build safe. Contains the ONE X-Macro list.
- **Port layer** — all vendor-specific code isolated in `port/<target>/`.
- **Host tests** — build with `DEV_GPIO_PORT=mock`, 34 tests pass on Linux.

---

## 8. Porting to New Hardware

### 8.1 What to change

| File | Purpose |
|------|---------|
| `port/<vendor>/dev_gpio_port_<vendor>.h` | Define vendor-specific `dev_gpio_hw_pin_t`, include vendor header |
| `port/<vendor>/dev_gpio_port_<vendor>.c` | Auto-generate `s_gpio_map[]` from X-Macro, implement 11 port functions |

The X-Macro (`DEV_GPIO_PIN_LIST` in `dev_gpio_cfg.h`) stays identical — just change the port files.

### 8.2 ESP32 example

`port/esp32/dev_gpio_port_esp32.c`: same structure, replace HAL calls with ESP-IDF:

```c
// s_gpio_map[] uses gpio_num_t instead of GPIO_TypeDef*
static const dev_gpio_hw_pin_t s_gpio_map[DEV_GPIO_CFG_PIN_COUNT] = {
    DEV_GPIO_PIN_LIST(DEV_GPIO_BUILD_MAP)   // same X-Macro!
};

dev_err_t dev_gpio_port_output(dev_gpio_pin_t pin, dev_gpio_level_t lvl)
{
    gpio_set_direction(s_gpio_map[pin].gpio_num, GPIO_MODE_OUTPUT);
    gpio_set_level(s_gpio_map[pin].gpio_num, lvl);
    return DEV_OK;
}
// ... etc
```

### 8.3 Checklist

1. Create `port/<vendor>/dev_gpio_port_<vendor>.h` — vendor hw_pin_t
2. Create `port/<vendor>/dev_gpio_port_<vendor>.c` — implement 11 functions
3. Add `target_link_libraries(dev_gpio PRIVATE <vendor_sdk>)` in CMake
4. Set `DEV_GPIO_PORT=<vendor>` in CMake
5. Build and test

---

## 9. File Reference

| File | Edit? | Purpose |
|------|-------|---------|
| `dev_gpio_types.h` | No | Portable types |
| `dev_gpio_cfg.h` | **YES** | Add pins to `DEV_GPIO_PIN_LIST` |
| `dev_gpio_port.h` | No | Port interface |
| `dev_gpio.h` | No | Public API |
| `dev_gpio.c` | No | Thin wrapper |
| `port/stm32/*` | No | STM32 implementation |
| `port/mock/*` | No | Host testing |

---

## 10. Build

```cmake
# Root CMakeLists.txt
set(DEV_GPIO_PORT "stm32" CACHE STRING "GPIO port: mock, stm32")
add_subdirectory(drivers/dev_gpio)
target_link_libraries(${PROJECT_NAME} dev_gpio)

# Host tests
# DEV_GPIO_PORT=mock — builds against mock port, 34 tests pass on Linux
```
