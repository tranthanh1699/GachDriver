# dev_gpio — GPIO Driver with X-Macro Pin Definition

## 1. Overview

`dev_gpio` is a hardware-independent GPIO driver that uses **X-Macro** to define all pins in one place. Adding a new pin takes **one line** — the enum, hardware mapping table, and pin count are generated automatically.

```
Application                     Driver                         Hardware
──────────                      ──────                         ────────
dev_gpio_write(LED, HIGH)  →   s_gpio_map[LED] lookup    →    HAL_GPIO_WritePin(GPIOB, PIN_0)
                               (O(1) by pin ID)
```

**Key features:**
- **One-line pin definition** via X-Macro `DEV_GPIO_PIN_LIST(X)`
- **Auto-generated enum** — `DEV_GPIO_LED_STATUS`, `DEV_GPIO_BUTTON_USER`, `DEV_GPIO_CFG_PIN_COUNT`
- **Auto-generated map** — `s_gpio_map[]` indexed by pin ID, O(1) access
- **No dynamic memory** — everything is `static const`
- **No board folder** — pin list lives in `dev_gpio_cfg.h`
- **Thin wrapper** over STM32 HAL — easy to port

---

## 2. Quick Start

### 2.1 Define your pins

Open `drivers/dev_gpio/include/dev_gpio_cfg.h` and edit `DEV_GPIO_PIN_LIST`:

```c
#define DEV_GPIO_PIN_LIST(X)                                                     \
    X(LED_STATUS,  GPIOB, GPIO_PIN_0,  DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE) \
    X(BUTTON_USER, GPIOC, GPIO_PIN_13, DEV_GPIO_MODE_INPUT,  DEV_GPIO_PULL_UP)
```

**Format:** `X(NAME, STM32_PORT, STM32_PIN, MODE, PULL)`

### 2.2 Use in application

```c
#include "dev_gpio.h"

int main(void)
{
    dev_gpio_init();                                     // init all pins

    dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH);  // LED on
    dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_LOW);   // LED off
    dev_gpio_toggle(DEV_GPIO_LED_STATUS);                      // toggle

    dev_gpio_level_t btn = dev_gpio_read(DEV_GPIO_BUTTON_USER);
    if (btn == DEV_GPIO_LEVEL_LOW) {
        /* button pressed — pull-up means LOW = active */
    }
}
```

---

## 3. Pin Definition — `dev_gpio_cfg.h`

### 3.1 The X-Macro

All pin configuration lives in ONE macro:

```c
#define DEV_GPIO_PIN_LIST(X)                                                     \
    X(NAME,       PORT,  PIN,          MODE,                    PULL)
    X(LED_STATUS, GPIOB, GPIO_PIN_0,   DEV_GPIO_MODE_OUTPUT,   DEV_GPIO_PULL_NONE)
    X(BUTTON_USER,GPIOC, GPIO_PIN_13,  DEV_GPIO_MODE_INPUT,    DEV_GPIO_PULL_UP)
    X(RELAY_1,    GPIOA, GPIO_PIN_5,   DEV_GPIO_MODE_OUTPUT,   DEV_GPIO_PULL_NONE)
    X(BUZZER,     GPIOB, GPIO_PIN_1,   DEV_GPIO_MODE_OUTPUT,   DEV_GPIO_PULL_NONE)
    X(DIP_SW_1,   GPIOD, GPIO_PIN_2,   DEV_GPIO_MODE_INPUT_PULLUP,  DEV_GPIO_PULL_UP)
```

**Fields:**

| Field | Type | Description |
|-------|------|-------------|
| `NAME` | identifier | Logical pin name (becomes `DEV_GPIO_<NAME>`) |
| `PORT` | `GPIO_TypeDef *` | STM32 GPIO port (`GPIOA`, `GPIOB`, ...) |
| `PIN` | `uint16_t` | STM32 pin mask (`GPIO_PIN_0`, `GPIO_PIN_13`, ...) |
| `MODE` | `dev_gpio_mode_t` | `DEV_GPIO_MODE_INPUT`, `DEV_GPIO_MODE_OUTPUT`, `DEV_GPIO_MODE_INPUT_PULLUP`, `DEV_GPIO_MODE_INPUT_PULLDOWN` |
| `PULL` | `dev_gpio_pull_t` | `DEV_GPIO_PULL_NONE`, `DEV_GPIO_PULL_UP`, `DEV_GPIO_PULL_DOWN` |

### 3.2 What gets auto-generated

From the X-Macro above, the compiler automatically produces:

```c
// Enum — never write this manually
typedef enum {
    DEV_GPIO_LED_STATUS  = 0,
    DEV_GPIO_BUTTON_USER = 1,
    DEV_GPIO_RELAY_1     = 2,
    DEV_GPIO_BUZZER      = 3,
    DEV_GPIO_DIP_SW_1    = 4,
    DEV_GPIO_CFG_PIN_COUNT = 5   // auto count
} dev_gpio_logical_pin_id_t;

// Hardware map — never write this manually
static const dev_gpio_hw_pin_t s_gpio_map[5] = {
    [0] = { DEV_GPIO_LED_STATUS,  GPIOB, GPIO_PIN_0,  DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE },
    [1] = { DEV_GPIO_BUTTON_USER, GPIOC, GPIO_PIN_13, DEV_GPIO_MODE_INPUT,  DEV_GPIO_PULL_UP },
    [2] = { DEV_GPIO_RELAY_1,     GPIOA, GPIO_PIN_5,  DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE },
    [3] = { DEV_GPIO_BUZZER,      GPIOB, GPIO_PIN_1,  DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE },
    [4] = { DEV_GPIO_DIP_SW_1,    GPIOD, GPIO_PIN_2,  DEV_GPIO_MODE_INPUT_PULLUP, DEV_GPIO_PULL_UP },
};
```

### 3.3 Adding a new pin

**Just add one line to `DEV_GPIO_PIN_LIST`.** That's it. No other file changes.

```c
X(UART_TX, GPIOD, GPIO_PIN_5, DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE)
```

You immediately get:
- `DEV_GPIO_UART_TX` — usable in application code
- `DEV_GPIO_CFG_PIN_COUNT` — auto-incremented
- Entry in `s_gpio_map[<index>]` — auto-generated

---

## 4. Configuration — `dev_gpio_cfg.h`

```c
#define DEV_GPIO_CFG_MAX_PINS              (32U)   // max pins supported
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)    // 1U = interrupt API compiled in
#define DEV_GPIO_CFG_RUNTIME_CHECK_ENABLED (1U)    // 1U = validate init + pin range
```

| Macro | `1U` | `0U` |
|-------|------|------|
| `RUNTIME_CHECK_ENABLED` | Check init state + pin range on every call | Skip runtime checks (smaller/faster) |
| `INTERRUPT_ENABLED` | Interrupt functions compile normally | Reserved for future use |

---

## 5. API Reference — `dev_gpio.h`

### 5.1 dev_gpio_init

```c
dev_err_t dev_gpio_init(void);
```

Initializes **all pins** defined in `DEV_GPIO_PIN_LIST`. Enables GPIO peripheral clocks and configures each pin's mode and pull.

| Return | Meaning |
|--------|---------|
| `DEV_OK` | All pins configured |
| `DEV_ERR_ALREADY_INITIALIZED` | Already called |
| `DEV_ERR_HW_FAILURE` | HAL configuration failed |

**Not ISR-safe. Not reentrant.**

### 5.2 dev_gpio_write

```c
dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level);
```

Writes `DEV_GPIO_LEVEL_HIGH` or `DEV_GPIO_LEVEL_LOW` to a pin.

| Return | Meaning |
|--------|---------|
| `DEV_OK` | Write succeeded |
| `DEV_ERR_NOT_INITIALIZED` | `dev_gpio_init()` not called (runtime checks on) |
| `DEV_ERR_INVALID_ARG` | `pin >= DEV_GPIO_CFG_PIN_COUNT` |

**Example:**
```c
dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH);  // turn on
dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_LOW);   // turn off
```

### 5.3 dev_gpio_read

```c
dev_gpio_level_t dev_gpio_read(dev_gpio_pin_t pin);
```

Returns the current logic level of a pin.

| Return | Meaning |
|--------|---------|
| `DEV_GPIO_LEVEL_HIGH` | Pin is HIGH |
| `DEV_GPIO_LEVEL_LOW` | Pin is LOW, or not initialized, or invalid pin |

**Example:**
```c
dev_gpio_level_t btn = dev_gpio_read(DEV_GPIO_BUTTON_USER);
if (btn == DEV_GPIO_LEVEL_LOW) {
    dev_gpio_toggle(DEV_GPIO_LED_STATUS);
}
```

### 5.4 dev_gpio_toggle

```c
dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin);
```

Toggles the output level (LOW→HIGH, HIGH→LOW).

| Return | Meaning |
|--------|---------|
| `DEV_OK` | Toggle succeeded |
| `DEV_ERR_NOT_INITIALIZED` | `dev_gpio_init()` not called |
| `DEV_ERR_INVALID_ARG` | `pin >= DEV_GPIO_CFG_PIN_COUNT` |

**Example:**
```c
dev_gpio_toggle(DEV_GPIO_LED_STATUS);  // blink each call
```

---

## 6. Usage Examples

### 6.1 Blink LED

```c
#include "dev_gpio.h"

int main(void)
{
    dev_gpio_init();

    for (;;) {
        dev_gpio_toggle(DEV_GPIO_LED_STATUS);
        for (volatile uint32_t d = 0U; d < 5000000U; d++) {}
    }
}
```

### 6.2 Button controls LED

```c
#include "dev_gpio.h"

int main(void)
{
    dev_gpio_init();

    for (;;) {
        if (dev_gpio_read(DEV_GPIO_BUTTON_USER) == DEV_GPIO_LEVEL_LOW) {
            dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH);
        } else {
            dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_LOW);
        }
    }
}
```

### 6.3 Multiple outputs

```c
dev_gpio_init();

dev_gpio_write(DEV_GPIO_RELAY_1,  DEV_GPIO_LEVEL_HIGH);  // relay on
dev_gpio_write(DEV_GPIO_BUZZER,   DEV_GPIO_LEVEL_HIGH);  // buzzer on

for (volatile uint32_t d = 0U; d < 1000000U; d++) {}      // delay

dev_gpio_write(DEV_GPIO_RELAY_1,  DEV_GPIO_LEVEL_LOW);   // relay off
dev_gpio_write(DEV_GPIO_BUZZER,   DEV_GPIO_LEVEL_LOW);   // buzzer off
```

### 6.4 Read input with pull-up

```c
// Configured as DEV_GPIO_MODE_INPUT_PULLUP, DEV_GPIO_PULL_UP
// Button pressed → pin pulled LOW through external switch

if (dev_gpio_read(DEV_GPIO_DIP_SW_1) == DEV_GPIO_LEVEL_LOW) {
    /* switch closed */
}
```

### 6.5 Button interrupt — LED toggle on press

```c
#include "dev_gpio.h"

static void button_isr(dev_gpio_pin_t pin, void *user_arg)
{
    (void)user_arg;
    dev_gpio_toggle(DEV_GPIO_LED_STATUS);
}

int main(void)
{
    dev_gpio_init();

    /* Enable interrupt on BUTTON_USER */
    dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER,
                              DEV_GPIO_INTR_FALLING_EDGE,
                              button_isr,
                              NULL);

    for (;;) {
        /* main loop — ISR handles the LED */
    }
}
```

### 6.6 Wiring the EXTI callback

In your STM32 project (`Core/Src/stm32h7xx_it.c`), add one line to route EXTI interrupts to `dev_gpio`:

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    dev_gpio_dispatch_isr(GPIO_Pin);
}
```

This is the **only** place vendor ISR code touches `dev_gpio`. The driver handles callback lookup and invocation.

### 6.7 Multiple interrupt pins

```c
static void btn_isr(dev_gpio_pin_t pin, void *arg) { /* ... */ }
static void sensor_isr(dev_gpio_pin_t pin, void *arg) { /* ... */ }

dev_gpio_init();

dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER,
                          DEV_GPIO_INTR_FALLING_EDGE, btn_isr, NULL);

dev_gpio_interrupt_enable(DEV_GPIO_SENSOR_INT,
                          DEV_GPIO_INTR_RISING_EDGE, sensor_isr, NULL);
```

---

## 7. Interrupt API Reference

### 7.1 dev_gpio_interrupt_enable

```c
dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t      pin,
                                    dev_gpio_intr_t     intr,
                                    dev_gpio_callback_t callback,
                                    void               *user_arg);
```

Configures EXTI interrupt mode, registers the callback, and enables NVIC.

| Parameter | Description |
|-----------|-------------|
| `pin` | Logical pin ID |
| `intr` | `DEV_GPIO_INTR_RISING_EDGE`, `DEV_GPIO_INTR_FALLING_EDGE`, or `DEV_GPIO_INTR_BOTH_EDGES` |
| `callback` | ISR handler (must not be NULL) |
| `user_arg` | Passed to callback (may be NULL) |

| Return | Meaning |
|--------|---------|
| `DEV_OK` | Interrupt enabled |
| `DEV_ERR_NULL_PTR` | callback is NULL |
| `DEV_ERR_NOT_INITIALIZED` | dev_gpio_init() not called |
| `DEV_ERR_INVALID_ARG` | pin out of range |
| `DEV_ERR_NOT_SUPPORTED` | pin number has no mapped EXTI IRQ |

### 7.2 dev_gpio_interrupt_disable

```c
dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin);
```

Disables NVIC, restores pin to regular input mode, clears callback.

### 7.3 dev_gpio_dispatch_isr

```c
void dev_gpio_dispatch_isr(uint16_t hal_pin);
```

Called from `HAL_GPIO_EXTI_Callback`. Maps STM32 pin mask → logical pin ID, invokes registered callback. **PORT-ONLY** — do not call from application code.

### 7.4 Interrupt Types

| Constant | EXTI Mode |
|----------|-----------|
| `DEV_GPIO_INTR_RISING_EDGE` | `GPIO_MODE_IT_RISING` |
| `DEV_GPIO_INTR_FALLING_EDGE` | `GPIO_MODE_IT_FALLING` |
| `DEV_GPIO_INTR_BOTH_EDGES` | `GPIO_MODE_IT_RISING_FALLING` |

### 7.5 Callback Rules

- Callback runs in **ISR context** — must not block, must not call non-ISR-safe APIs
- Callback receives `(dev_gpio_pin_t pin, void *user_arg)`
- Callback table is static, sized to `DEV_GPIO_CFG_MAX_PINS`
- Only pins configured as input (INPUT, INPUT_PULLUP, INPUT_PULLDOWN) should enable interrupts

---

## 8. Porting to New Hardware

### 8.1 Overview

Porting `dev_gpio` to a new MCU requires changing **2 files**:

| File | What to change |
|------|---------------|
| `dev_gpio_cfg.h` | Replace `#include "stm32h7xx_hal.h"` with your vendor header. Update `dev_gpio_hw_pin_t` struct. Update `DEV_GPIO_PIN_LIST` fields. |
| `dev_gpio.c` | Replace STM32 HAL calls with your vendor's GPIO API. |

The X-Macro mechanism (`DEV_GPIO_PIN_LIST`, `dev_gpio_get_hw_map`, `dev_gpio_get_pin_count`, `dev_gpio_logical_pin_id_t`) stays identical — only the hardware access layer changes.

### 8.2 STM32 → ESP32 Example

**Step 1: `dev_gpio_cfg.h`**

```c
#include "driver/gpio.h"    // ESP-IDF GPIO

// ESP32 hardware pin descriptor
typedef struct {
    dev_gpio_pin_t  logical_id;
    gpio_num_t      gpio_num;
    dev_gpio_mode_t mode;
    dev_gpio_pull_t pull;
} dev_gpio_hw_pin_t;

// ESP32 pin definition — different fields, same X-Macro mechanism
#define DEV_GPIO_PIN_LIST(X)                                          \
    X(LED_STATUS,  GPIO_NUM_2,  DEV_GPIO_MODE_OUTPUT, DEV_GPIO_PULL_NONE) \
    X(BUTTON_USER, GPIO_NUM_0,  DEV_GPIO_MODE_INPUT,  DEV_GPIO_PULL_UP)
```

Note: `dev_gpio_logical_pin_id_t` and `DEV_GPIO_CFG_PIN_COUNT` still auto-generate — no manual enum.

**Step 2: `dev_gpio.c`**

```c
#include "dev_gpio.h"
#include "dev_common.h"

static bool g_initialized = false;

static void dev_gpio_setup_pin(const dev_gpio_hw_pin_t *hw)
{
    gpio_config_t cfg = {0};
    cfg.pin_bit_mask = (1ULL << hw->gpio_num);

    switch (hw->mode) {
    case DEV_GPIO_MODE_INPUT:
        cfg.mode = GPIO_MODE_INPUT;
        cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        break;
    case DEV_GPIO_MODE_OUTPUT:
        cfg.mode = GPIO_MODE_OUTPUT;
        break;
    case DEV_GPIO_MODE_INPUT_PULLUP:
        cfg.mode = GPIO_MODE_INPUT;
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        break;
    case DEV_GPIO_MODE_INPUT_PULLDOWN:
        cfg.mode = GPIO_MODE_INPUT;
        cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
        break;
    }
    gpio_config(&cfg);
}

dev_err_t dev_gpio_init(void)
{
    if (g_initialized) { return DEV_ERR_ALREADY_INITIALIZED; }

    uint16_t count = dev_gpio_get_pin_count();
    for (uint16_t i = 0U; i < count; i++) {
        dev_gpio_setup_pin(&dev_gpio_get_hw_map()[i]);
    }

    g_initialized = true;
    return DEV_OK;
}

dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level)
{
    uint32_t val = (level == DEV_GPIO_LEVEL_HIGH) ? 1U : 0U;
    gpio_set_level(dev_gpio_get_hw_map()[pin].gpio_num, val);
    return DEV_OK;
}

dev_gpio_level_t dev_gpio_read(dev_gpio_pin_t pin)
{
    return (gpio_get_level(dev_gpio_get_hw_map()[pin].gpio_num) == 1)
           ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW;
}

dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin)
{
    gpio_num_t num = dev_gpio_get_hw_map()[pin].gpio_num;
    gpio_set_level(num, (gpio_get_level(num) == 1) ? 0 : 1);
    return DEV_OK;
}
```

### 8.3 Porting Checklist

| # | Step |
|---|------|
| 1 | Replace `#include` vendor header in `dev_gpio_cfg.h` |
| 2 | Update `dev_gpio_hw_pin_t` to match your vendor's types |
| 3 | Update `DEV_GPIO_PIN_LIST` field count and names |
| 4 | Update `DEV_GPIO_DECLARE_PIN_ID` and `DEV_GPIO_BUILD_HW_MAP` macros to match new fields |
| 5 | Replace HAL calls in `dev_gpio.c` with your vendor API |
| 6 | Add clock enable logic for your vendor (if needed) |
| 7 | Update `CMakeLists.txt` to link your vendor SDK |

---

## 9. Types Reference — `dev_gpio_types.h`

```c
typedef uint16_t dev_gpio_pin_t;        // Logical pin ID

typedef enum {
    DEV_GPIO_LEVEL_LOW  = 0,
    DEV_GPIO_LEVEL_HIGH = 1
} dev_gpio_level_t;

typedef enum {
    DEV_GPIO_MODE_INPUT = 0,            // Input, no pull
    DEV_GPIO_MODE_OUTPUT,               // Push-pull output, default LOW
    DEV_GPIO_MODE_INPUT_PULLUP,         // Input with pull-up
    DEV_GPIO_MODE_INPUT_PULLDOWN        // Input with pull-down
} dev_gpio_mode_t;

typedef enum {
    DEV_GPIO_PULL_NONE = 0,
    DEV_GPIO_PULL_UP,
    DEV_GPIO_PULL_DOWN
} dev_gpio_pull_t;
```

---

## 10. File Reference

| File | Role |
|------|------|
| `dev_gpio_types.h` | Portable types: `dev_gpio_pin_t`, `dev_gpio_level_t`, `dev_gpio_mode_t`, `dev_gpio_pull_t` |
| `dev_gpio_cfg.h` | **The file you edit.** X-Macro `DEV_GPIO_PIN_LIST`, auto-generated enum, `dev_gpio_hw_pin_t`, config macros, accessor prototypes |
| `dev_gpio_cfg.c` | Auto-generated `s_gpio_map[]` + `dev_gpio_get_hw_map()` + `dev_gpio_get_pin_count()` |
| `dev_gpio.h` | Public API: `dev_gpio_init`, `dev_gpio_write`, `dev_gpio_read`, `dev_gpio_toggle` |
| `dev_gpio.c` | Implementation — STM32 HAL calls, clock enable, pin setup |

---

## 11. Build Integration

```cmake
# CMakeLists.txt
add_subdirectory(drivers/dev_common)
add_subdirectory(drivers/dev_gpio)

target_link_libraries(${PROJECT_NAME}
    dev_common
    dev_gpio
)
```

`dev_gpio` links `stm32cubemx` privately — application code never sees vendor headers.
