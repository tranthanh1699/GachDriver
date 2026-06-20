# OSAL — Operating System Abstraction Layer

## 1. Purpose

OSAL hides whether the system runs on bare-metal, FreeRTOS, Zephyr, ESP-IDF FreeRTOS,
or any other RTOS. It exposes a minimal set of platform-primitive APIs — tick, delay,
kernel status — so that upper layers (`svc_*`, `app/`) never need to include vendor
or RTOS headers directly.

This is the **first layer** initialized after hardware setup. Every other service
depends on it for time and delay.

---

## 2. Folder Structure

```
platform/osal/
  include/
    osal.h              ← Public API (include this in your code)
    osal_types.h        ← Base types (pulls in dev_types.h, dev_error.h)
    osal_cfg.h          ← Backend selection and compile-time config
    osal_port.h         ← Port interface (implemented by each backend)
  src/
    osal.c              ← Generic implementation — delegates to port
  port/
    baremetal/
      osal_port_baremetal.h   ← Bare-metal port header
      osal_port_baremetal.c   ← Bare-metal port (STM32 HAL_GetTick / HAL_Delay)
```

Future backends will live alongside `baremetal/`:

```
  port/
    freertos/           ← Planned: FreeRTOS xTaskGetTickCount / vTaskDelay
    esp32/              ← Planned: ESP-IDF FreeRTOS variant
    zephyr/             ← Planned: Zephyr k_cycle_get_32 / k_msleep
```

---

## 3. Public API Reference

All functions are declared in `osal.h`. Include only this header in application code.

### 3.1 Initialization

```c
dev_err_t osal_init(void);
```

**What it does:** Initializes the selected backend. Must be called once after hardware
peripherals are set up (`HAL_Init`, `SystemClock_Config`, `MX_*_Init`) but before any
service that uses time or delays.

**Returns:**
| Return value | Meaning |
|--------------|---------|
| `DEV_OK` | Backend initialized successfully |
| `DEV_ERR_ALREADY_INITIALIZED` | `osal_init()` was already called |

**Example:**
```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    dev_err_t ret = osal_init();
    if (ret != DEV_OK && ret != DEV_ERR_ALREADY_INITIALIZED)
    {
        /* Handle failure — should not happen in bare-metal */
        Error_Handler();
    }
    /* ... rest of application ... */
}
```

### 3.2 Initialization Query

```c
bool osal_is_initialized(void);
```

**What it does:** Returns `true` if `osal_init()` completed successfully.

**Example:**
```c
void some_late_init(void)
{
    if (!osal_is_initialized())
    {
        /* OSAL not ready yet — abort or defer */
        return;
    }
    osal_delay_ms(100U);
}
```

### 3.3 System Tick

```c
uint32_t osal_get_tick_ms(void);
```

**What it does:** Returns a free-running millisecond counter.

- Bare-metal: wraps `HAL_GetTick()` (SysTick-driven, starts at 0 after `HAL_Init()`).
- The counter wraps after approximately 49.7 days (2³² ms). This is normal. Use
  unsigned subtraction to compute deltas correctly across wrap:

```c
uint32_t start = osal_get_tick_ms();
do_work();
uint32_t elapsed = osal_get_tick_ms() - start;  /* unsigned wrap is defined */
```

**Important:** The tick value is only meaningful after both `HAL_Init()` and
`osal_init()` have been called. Calling before that produces undefined behavior.

**Example — simple timeout:**
```c
#define UART_READ_TIMEOUT_MS   (500U)

dev_err_t uart_read_with_timeout(uint8_t *byte)
{
    uint32_t deadline = osal_get_tick_ms() + UART_READ_TIMEOUT_MS;

    while (!uart_byte_available())
    {
        if ((osal_get_tick_ms() - deadline) < UINT32_MAX / 2U)
        {
            /* deadline has passed */
            return DEV_ERR_TIMEOUT;
        }
    }

    *byte = uart_read_byte();
    return DEV_OK;
}
```

The pattern `(now - deadline) < UINT32_MAX/2` safely detects timeout across counter
wrap without relying on signed arithmetic.

### 3.4 Blocking Delay

```c
void osal_delay_ms(uint32_t delay_ms);
```

**What it does:** Blocks the CPU for `delay_ms` milliseconds.

- Bare-metal: wraps `HAL_Delay()` (busy-wait on SysTick).
- FreeRTOS (future): will use `vTaskDelay()` to yield to other tasks.

**Important:** This is a **blocking** call. The CPU spins and no other work progresses.
In bare-metal superloop designs, use it sparingly — only for short, necessary delays
during initialization. During RUN state, prefer the superloop's non-blocking polling
pattern instead.

**Example — initialization delay:**
```c
dev_err_t sensor_power_up(void)
{
    gpio_set(DEV_GPIO_SENSOR_EN, DEV_GPIO_LEVEL_HIGH);
    osal_delay_ms(10U);   /* wait for sensor LDO to stabilize */
    return DEV_OK;
}
```

**Anti-pattern — do NOT do this in the superloop:**
```c
/* BAD: blocks the superloop for 1 second */
while (1)
{
    svc_sm_handle();
    osal_delay_ms(1000U);   /* ← everything freezes for 1s */
    toggle_led();
}
```

**Correct — use tick-based polling:**
```c
/* GOOD: non-blocking, other work continues each iteration */
static uint32_t last_toggle_ms = 0U;
while (1)
{
    svc_sm_handle();
    if ((osal_get_tick_ms() - last_toggle_ms) >= 1000U)
    {
        last_toggle_ms = osal_get_tick_ms();
        toggle_led();
    }
}
```

### 3.5 Kernel Status

```c
bool osal_is_kernel_running(void);
```

**What it does:** Returns `true` if an RTOS kernel scheduler is actively running.

- Bare-metal: always returns `false`.
- This function is useful for code that needs different behavior depending on
  whether task preemption is active.

**Example — safe print depending on context:**
```c
void debug_print(const char *msg)
{
    if (osal_is_kernel_running())
    {
        /* RTOS mode: use thread-safe logging */
        rtos_log_write(msg);
    }
    else
    {
        /* Bare-metal: direct UART is fine */
        uart_write_string(msg);
    }
}
```

### 3.6 Kernel Start

```c
dev_err_t osal_kernel_start(void);
```

**What it does:** Starts the RTOS scheduler.

- Bare-metal: returns `DEV_ERR_NOT_SUPPORTED`. The superloop is the scheduler.
- FreeRTOS (future): calls `vTaskStartScheduler()` and never returns.

**Returns:**
| Return value | Meaning |
|--------------|---------|
| `DEV_OK` | Scheduler started (RTOS mode) |
| `DEV_ERR_NOT_SUPPORTED` | Bare-metal — no kernel to start |
| `DEV_ERR_NOT_INITIALIZED` | `osal_init()` not called |

**Example — portable main():**
```c
int main(void)
{
    hardware_init();
    osal_init();
    app_init();

    dev_err_t ret = osal_kernel_start();

    /* Bare-metal: returns DEV_ERR_NOT_SUPPORTED — fall through to superloop */
    if (ret == DEV_ERR_NOT_SUPPORTED)
    {
        for (;;)
        {
            app_run();
        }
    }

    /* RTOS mode: osal_kernel_start() never returns on success */
    return 0;
}
```

---

## 4. Configuration

All configuration lives in `osal_cfg.h`. This is the **only file you need to edit**
to switch between bare-metal and RTOS backends.

### 4.1 Config Knobs

```c
/* Master switch for parameter validation.
 * Set to 0U to remove runtime checks (saves code size). */
#define OSAL_CFG_RUNTIME_CHECK_ENABLED      (1U)

/* Backend selection — EXACTLY ONE must be 1U.
 * The compile-time check below will #error if this rule is violated. */
#define OSAL_CFG_BAREMETAL_ENABLED          (1U)
#define OSAL_CFG_FREERTOS_ENABLED           (0U)

/* Expected tick rate. Informational only — does not change behavior.
 * Used by future backends to validate timer configuration. */
#define OSAL_CFG_DEFAULT_TICK_RATE_HZ       (1000U)
```

### 4.2 Switching to FreeRTOS (Future)

When FreeRTOS support is added, change **exactly two lines**:

```c
#define OSAL_CFG_BAREMETAL_ENABLED          (0U)   /* was 1U */
#define OSAL_CFG_FREERTOS_ENABLED           (1U)   /* was 0U */
```

The compile-time check ensures you can't accidentally enable both (or neither):

```c
#if ((OSAL_CFG_BAREMETAL_ENABLED + OSAL_CFG_FREERTOS_ENABLED) != 1U)
#error "osal_cfg: exactly one OSAL backend must be enabled"
#endif
```

### 4.3 Adding a New Backend (Zephyr Example)

1. Add a new config flag:
   ```c
   #define OSAL_CFG_ZEPHYR_ENABLED          (0U)
   ```

2. Update the validation check to include the new flag.

3. Create `platform/osal/port/zephyr/osal_port_zephyr.c` implementing
   all functions declared in `osal_port.h`.

4. Update `platform/osal/CMakeLists.txt` to compile the new port file when
   its flag is enabled.

No changes needed in `osal.c` or the public headers.

---

## 5. Dependency Rules

### Allowed

```
osal.c  ──►  dev_common
osal_port_baremetal.c  ──►  dev_common, STM32 HAL (stm32h7xx_hal.h)
```

### Forbidden

- `osal.c` including RTOS or HAL headers
- Public headers (`osal.h`, `osal_types.h`, `osal_port.h`) including vendor headers
- Application code calling `osal_port_*` functions directly (always use the `osal_*` wrappers)
- `dev_common` depending on `osal` (circular dependency)

### Include Graph (correct)

```
Application
  └─ #include "osal.h"
       └─ #include "osal_types.h"    → dev_types.h, dev_error.h
       └─ #include "osal_cfg.h"

  Application does NOT include:
    ✗ stm32h7xx_hal.h
    ✗ FreeRTOS.h
    ✗ osal_port.h
    ✗ osal_port_baremetal.h
```

---

## 6. Bare-Metal Backend Details

| Function | Implementation | Notes |
|----------|---------------|-------|
| `osal_port_init()` | Returns `DEV_OK` | Nothing to init — SysTick runs from `HAL_Init()` |
| `osal_port_get_tick_ms()` | `HAL_GetTick()` | SysTick-driven, 1 ms resolution |
| `osal_port_delay_ms(ms)` | `HAL_Delay(ms)` | Busy-wait on SysTick |
| `osal_port_is_kernel_running()` | `return false` | No kernel exists |
| `osal_port_kernel_start()` | `return DEV_ERR_NOT_SUPPORTED` | No scheduler to start |

The STM32 HAL include (`stm32h7xx_hal.h`) is confined to `osal_port_baremetal.c`.
If porting to a different bare-metal target (e.g., a PIC or MSP430), only this one
file changes.

---

## 7. Port Interface Contract

Every backend must implement the five functions declared in `osal_port.h`:

```c
dev_err_t osal_port_init(void);
uint32_t  osal_port_get_tick_ms(void);
void      osal_port_delay_ms(uint32_t delay_ms);
bool      osal_port_is_kernel_running(void);
dev_err_t osal_port_kernel_start(void);
```

**Contract rules:**

1. `osal_port_init()` — called once. Perform backend setup here. Return `DEV_OK` or an
   error code. Must be idempotent (safe to call again, though `osal_init()` prevents this).

2. `osal_port_get_tick_ms()` — must return a monotonically increasing millisecond value.
   Wraps at 32-bit boundary (~49.7 days) — callers must handle this using unsigned
   subtraction.

3. `osal_port_delay_ms(ms)` — block for at least `ms` milliseconds. May block longer
   (e.g., if interrupted) but never less. `delay_ms = 0` should return immediately.

4. `osal_port_is_kernel_running()` — `true` if scheduler started, `false` otherwise.
   Must be callable at any time.

5. `osal_port_kernel_start()` — start the scheduler. May never return (RTOS). Return
   `DEV_ERR_NOT_SUPPORTED` if no kernel exists.

---

## 8. CMake Integration

### For the OSAL library itself (`platform/osal/CMakeLists.txt`)

```cmake
add_library(osal STATIC
    src/osal.c
    port/baremetal/osal_port_baremetal.c
)

target_include_directories(osal PUBLIC
    include
    port/baremetal
)

target_link_libraries(osal PUBLIC
    dev_common
    stm32cubemx          # ← provides HAL include paths for the port file
)
```

### For consumers (root `CMakeLists.txt`)

```cmake
add_subdirectory(platform/osal)

target_link_libraries(${PROJECT_NAME}
    osal
    # ... other libs ...
)
```

Any target that links against `osal` automatically gets:
- `platform/osal/include` in its include path
- `dev_common` in its include path
- Access to `osal.h` and all its functions

---

## 9. Complete Usage Example

### Minimal bare-metal main()

```c
#include "main.h"
#include "gpio.h"
#include "usart.h"

#include "dev_common.h"
#include "osal.h"
#include "svc_sm.h"

int main(void)
{
    /* ── Step 1: Hardware setup (vendor HAL) ── */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    /* ── Step 2: Platform abstraction ── */
    (void)osal_init();           /* tick and delay become available */

    /* ── Step 3: Service layer ── */
    (void)svc_sm_init();         /* state manager initialization */
    (void)svc_sm_startup();      /* module init → app init → RUN */

    /* ── Step 4: Superloop ── */
    for (;;)
    {
        (void)svc_sm_handle();

        if (svc_sm_get_state() == SVC_SM_STATE_SHUTDOWN)
        {
            break;
        }
    }

    return 0;
}
```

### Using osal in a service or driver

```c
#include "osal.h"
#include "dev_error.h"

#define SENSOR_POWER_ON_DELAY_MS   (50U)
#define SENSOR_POLL_INTERVAL_MS    (10U)
#define SENSOR_READY_TIMEOUT_MS    (500U)

dev_err_t sensor_wait_ready(void)
{
    uint32_t deadline = osal_get_tick_ms() + SENSOR_READY_TIMEOUT_MS;
    uint32_t last_poll = 0U;

    while (1)
    {
        if ((osal_get_tick_ms() - deadline) < UINT32_MAX / 2U)
        {
            return DEV_ERR_TIMEOUT;
        }

        if ((osal_get_tick_ms() - last_poll) >= SENSOR_POLL_INTERVAL_MS)
        {
            last_poll = osal_get_tick_ms();
            if (sensor_is_ready())
            {
                return DEV_OK;
            }
        }
    }
}
```

---

## 10. Future RTOS Extension Plan

When FreeRTOS (or another RTOS) support is added, the following will also be
introduced as a **separate** extension — `osal_rtos`:

```c
/* NOT IMPLEMENTED in this phase — planned for future */
dev_err_t osal_task_create(...);
dev_err_t osal_mutex_create(...);
dev_err_t osal_mutex_lock(...);
dev_err_t osal_queue_send(...);
dev_err_t osal_event_wait(...);
```

This keeps the minimal OSAL small and dedicated to tick/delay/kernel primitives.
The `svc_sm` state machine is designed to work both in superloop mode (calling
`svc_sm_handle()` repeatedly) and in task mode (calling `svc_sm_handle()` from a
dedicated task) without modification.

---

## 11. MISRA-C Compliance Notes

| Rule | How OSAL complies |
|------|-------------------|
| No dynamic memory | All state is static (`g_osal_initialized`) |
| No recursion | No functions call themselves |
| Fixed-width types | `uint32_t` for ticks, `dev_err_t` for errors |
| Vendor isolation | `stm32h7xx_hal.h` only in `osal_port_baremetal.c` |
| No magic numbers | All constants defined in `osal_cfg.h` |
| Return value handling | All `dev_err_t` returns are documented |
| No `goto` / `continue` | Not used |

---

## 12. Quick Reference Card

```
┌─────────────────────────────────────────────────────────┐
│                     OSAL Quick Reference                 │
├──────────────┬──────────────────────────────────────────┤
│ osal_init()  │ Call once after HAL_Init(), before       │
│              │ services. Returns DEV_OK.                │
├──────────────┼──────────────────────────────────────────┤
│ get_tick_ms()│ Free-running ms counter. Use unsigned    │
│              │ subtraction for deltas. Wraps ~49 days.  │
├──────────────┼──────────────────────────────────────────┤
│ delay_ms(ms) │ Blocking delay. Use ONLY during init.    │
│              │ Never in superloop — poll tick instead.  │
├──────────────┼──────────────────────────────────────────┤
│ is_kernel_   │ false in bare-metal, true if RTOS        │
│ running()    │ scheduler is active.                     │
├──────────────┼──────────────────────────────────────────┤
│ kernel_start │ DEV_ERR_NOT_SUPPORTED in bare-metal.     │
│              │ Starts scheduler in RTOS builds.         │
├──────────────┼──────────────────────────────────────────┤
│ Backend      │ Edit osal_cfg.h: set one flag to 1U.     │
│ selection    │ Compile-time error if invalid combo.     │
└──────────────┴──────────────────────────────────────────┘
```
