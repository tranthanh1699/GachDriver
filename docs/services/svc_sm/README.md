# svc_sm — Service State Manager

## 1. Purpose

`svc_sm` is a lightweight service state manager that orchestrates the entire system
lifecycle. It is inspired by AUTOSAR EcuM but simplified for bare-metal superloop
systems.

It manages:

| Responsibility | How |
|----------------|-----|
| **System state machine** | 8 states from UNINIT to SHUTDOWN |
| **Module initialization order** | Forward at startup, reverse at shutdown |
| **Application lifecycle** | 6 callbacks at defined points |
| **Error handling** | Stores error info, transitions to ERROR state |
| **Safe shutdown** | Flushes EEPROM, stops services, deinits hardware |
| **Pending requests** | Deferred shutdown/error requests processed in handle() |

---

## 2. Folder Structure

```
services/svc_sm/
  include/
    svc_sm.h              ← Public API (include this in your code)
    svc_sm_types.h        ← State enum, request enum, error info, module descriptor
    svc_sm_cfg.h          ← Compile-time configuration (feature toggles)
    svc_sm_modules.h      ← Module table declaration (g_svc_sm_modules[])
    svc_sm_app.h          ← App lifecycle declaration (app_init, app_run, ...)
  src/
    svc_sm.c              ← State machine core — all logic lives here
    svc_sm_modules.c      ← Module table definition — list your services here
    svc_sm_app.c          ← Weak default app lifecycle (all return DEV_OK)
```

Companion files in `app/`:

```
app/
  include/
    app_lifecycle.h       ← Your app's lifecycle declarations
  src/
    app_lifecycle.c       ← Your app's lifecycle implementations (overrides weak defaults)
```

---

## 3. State Machine

### 3.1 State Diagram

```
                    ┌─────────────────────────────────────────┐
                    │                                         │
                    ▼                                         │
 UNINIT ──► STARTUP ──► INIT ──► POST_INIT ──► RUN ────────► PREPARE_SHUTDOWN ──► SHUTDOWN
               │          │          │           │                                           ▲
               └──────────┴──────────┴───────────┴──────────► ERROR ────────────────────────┘
```

- **Solid arrows**: normal forward progress.
- **Dashed arrows to ERROR**: any state can transition to ERROR on failure.
- **ERROR → PREPARE_SHUTDOWN**: the only exit from ERROR is toward shutdown (graceful degradation).
- **SHUTDOWN**: terminal state. No further transitions.

### 3.2 State Descriptions

| State | Enum Value | Meaning |
|-------|-----------|---------|
| `UNINIT` | `SVC_SM_STATE_UNINIT` | System not yet initialized. State after `svc_sm_init()`. |
| `STARTUP` | `SVC_SM_STATE_STARTUP` | Startup sequence has begun. Brief transitional state. |
| `INIT` | `SVC_SM_STATE_INIT` | Module `init()` and `start()` callbacks are called here. |
| `POST_INIT` | `SVC_SM_STATE_POST_INIT` | Modules are up. `app_init()` and `app_start()` run here. |
| `RUN` | `SVC_SM_STATE_RUN` | **Normal operation.** `svc_sm_handle()` calls module handles + `app_run()`. |
| `PREPARE_SHUTDOWN` | `SVC_SM_STATE_PREPARE_SHUTDOWN` | Shutdown started. `app_shutdown()` called. |
| `SHUTDOWN` | `SVC_SM_STATE_SHUTDOWN` | System halted. All modules deinitialized. Terminal. |
| `ERROR` | `SVC_SM_STATE_ERROR` | A fault occurred. `app_error()` is called once. |

Optional sleep states (enabled via `SVC_SM_CFG_SLEEP_ENABLED = 1U`):

| State | Meaning |
|-------|---------|
| `PREPARE_SLEEP` | Preparing to enter low-power mode |
| `SLEEP` | In low-power mode. Waiting for wakeup request. |
| `WAKEUP` | Waking from sleep, transitioning back to RUN |

### 3.3 Transition Rules

Exact allowed transitions (enforced at runtime by `svc_sm_is_transition_allowed()`):

```
From                  → To
────────────────────────────────────────────
UNINIT                → STARTUP
STARTUP               → INIT, ERROR
INIT                  → POST_INIT, ERROR
POST_INIT             → RUN, ERROR
RUN                   → PREPARE_SHUTDOWN, ERROR [, PREPARE_SLEEP]
PREPARE_SHUTDOWN      → SHUTDOWN, ERROR
SHUTDOWN              → (none — terminal)
ERROR                 → PREPARE_SHUTDOWN
```

Any attempt to make an invalid transition returns `DEV_ERR_INVALID_STATE`.
The state is **never** changed directly by application code — all changes go
through the validated `svc_sm_set_state()` private function.

---

## 4. Public API Reference

### 4.1 Initialization

```c
dev_err_t svc_sm_init(void);
```

Resets all internal state to defaults. State becomes `UNINIT`. Call once before
`svc_sm_startup()`.

```c
dev_err_t ret = svc_sm_init();
if (ret != DEV_OK)
{
    /* ret can only be DEV_ERR_ALREADY_INITIALIZED here */
}
```

### 4.2 Startup

```c
dev_err_t svc_sm_startup(void);
```

Runs the full startup sequence. **Must be called exactly once** after `svc_sm_init()`.

**What happens step by step:**

```
Step 1  ──► SVC_SM_STATE_STARTUP
Step 2  ──► SVC_SM_STATE_INIT
Step 3  ──► Module init()    [index 0 → N-1]    ← forward order
Step 4  ──► Module start()   [index 0 → N-1]    ← forward order
Step 5  ──► SVC_SM_STATE_POST_INIT
Step 6  ──► app_init()
Step 7  ──► app_start()
Step 8  ──► SVC_SM_STATE_RUN
```

**Error handling during startup:**

- **Critical module failure**: system enters `ERROR` immediately, `svc_sm_startup()`
  returns the error code. Startup is aborted.
- **Non-critical module failure**: error is stored (accessible via
  `svc_sm_get_last_error()`), startup continues.
- **App lifecycle failure** (`app_init()` or `app_start()` returns error): system
  enters `ERROR`, startup is aborted.

**Returns:**
| Return | Meaning |
|--------|---------|
| `DEV_OK` | Startup complete, system is in RUN |
| `DEV_ERR_NOT_INITIALIZED` | `svc_sm_init()` was not called first |
| `DEV_ERR_INVALID_STATE` | State transition was rejected (should not happen) |
| (module error) | A critical module failed — system is in ERROR |

### 4.3 Runtime Handle

```c
dev_err_t svc_sm_handle(void);
```

Called **repeatedly from the superloop**. This is the heartbeat of the system.
Must return quickly — no infinite loops or long blocking delays inside.

**What happens on each call (RUN state):**

```
1. Check for pending ERROR request → transition to ERROR if present
2. Check for pending SHUTDOWN request → execute shutdown if present
3. Call module handle()  [index 0 → N-1]   ← if SVC_SM_CFG_CALL_MODULE_HANDLE_IN_RUN = 1U
4. Call app_run()                            ← if SVC_SM_CFG_CALL_APP_RUN_IN_HANDLE = 1U
```

**What happens on each call (ERROR state):**

```
1. Call app_error() — once per error entry
2. Check for pending SHUTDOWN request → execute shutdown if present
```

**What happens on each call (SHUTDOWN state):**

```
Nothing — terminal state. The superloop should check for this and break.
```

### 4.4 Shutdown

```c
dev_err_t svc_sm_shutdown(void);
```

Executes the full shutdown sequence immediately. Also called internally when a
pending shutdown request is processed by `svc_sm_handle()`.

**Shutdown sequence step by step:**

```
Step 1  ──► SVC_SM_STATE_PREPARE_SHUTDOWN
Step 2  ──► app_shutdown()
Step 3  ──► Module stop()      [index N-1 → 0]   ← reverse order
Step 4  ──► Module shutdown()  [index N-1 → 0]   ← reverse order
Step 5  ──► Module deinit()    [index N-1 → 0]   ← reverse order
Step 6  ──► SVC_SM_STATE_SHUTDOWN
```

The reverse order ensures that higher-level services shut down before the lower-level
drivers they depend on. For the default module table: `svc_shell` stops first,
then `svc_eep` (which flushes dirty pages before I2C is deinitialized).

### 4.5 State Queries

```c
svc_sm_state_t svc_sm_get_state(void);
svc_sm_state_t svc_sm_get_previous_state(void);
bool           svc_sm_is_initialized(void);
bool           svc_sm_is_running(void);
```

`svc_sm_is_running()` is a convenience for checking `== SVC_SM_STATE_RUN`.

### 4.6 Requests

```c
dev_err_t svc_sm_request_shutdown(void);
dev_err_t svc_sm_request_error(dev_err_t reason);
```

**Requests are deferred.** They set a pending flag and are processed on the
**next call** to `svc_sm_handle()`. The application never sets state directly —
it requests a state change and the state machine processes it.

**Shutdown request:**
| Current State | Result |
|---------------|--------|
| `RUN` | Sets pending shutdown. Processed next `handle()`. |
| `ERROR` | Sets pending shutdown. Processed next `handle()`. |
| Any other | Returns `DEV_ERR_INVALID_STATE` |

**Error request:**
- Stores `reason` in `svc_sm_error_info_t`.
- Sets pending error request.
- On next `handle()`, transitions to `ERROR` and calls `app_error()` once.

### 4.7 Error Info

```c
dev_err_t svc_sm_get_last_error(svc_sm_error_info_t *info);
```

Returns the most recent error info. The `svc_sm_error_info_t` struct contains:

| Field | Meaning |
|-------|---------|
| `error` | The `dev_err_t` error code |
| `state` | The `svc_sm_state_t` the system was in when the error occurred |
| `module_name` | Name string of the module that failed, or `"app"` for app lifecycle errors, or `NULL` for direct `svc_sm_request_error()` calls |

---

## 5. Module Table

### 5.1 Module Descriptor

Each module in the system is described by an `svc_sm_module_t` struct:

```c
typedef struct
{
    const char         *name;       /* Human-readable name for error reporting */
    svc_sm_module_fn_t  init;       /* Called during INIT (forward)      — can be NULL */
    svc_sm_module_fn_t  start;      /* Called during INIT (forward)      — can be NULL */
    svc_sm_module_fn_t  handle;     /* Called during RUN (forward)       — can be NULL */
    svc_sm_module_fn_t  stop;       /* Called during shutdown (reverse)  — can be NULL */
    svc_sm_module_fn_t  shutdown;   /* Called during shutdown (reverse)  — can be NULL */
    svc_sm_module_fn_t  deinit;     /* Called during shutdown (reverse)  — can be NULL */
    bool                critical;   /* If true, failure → ERROR immediately */
} svc_sm_module_t;
```

Each callback has the signature `dev_err_t (*)(void)`. If a module's API takes
arguments (like `svc_shell_init(dev_uart_id_t)`), create a wrapper function.

**NULL callbacks are skipped silently** — no error, no log. You only need to
populate the callbacks your module actually needs.

### 5.2 Default Module Table

Defined in `services/svc_sm/src/svc_sm_modules.c`:

```c
const svc_sm_module_t g_svc_sm_modules[] =
{
    {
        "svc_eep",         /* name */
        svc_eep_init,      /* init     — initialize I2C EEPROM */
        NULL,              /* start    — not needed */
        NULL,              /* handle   — not needed */
        NULL,              /* stop     — not needed */
        svc_eep_shutdown,  /* shutdown — flush dirty pages to hardware */
        svc_eep_deinit,    /* deinit   — release I2C resources */
        true               /* critical — EEPROM failure is fatal */
    },
    {
        "svc_shell",
        svc_sm_shell_init_wrapper,  /* init     — svc_shell_init(DEV_UART_CONSOLE) */
        NULL,                       /* start    — not needed */
        svc_shell_handle,           /* handle   — process UART input */
        NULL,                       /* stop     — not needed */
        NULL,                       /* shutdown — not needed */
        svc_shell_deinit,           /* deinit   — release UART resources */
        false                       /* non-critical — shell failure is non-fatal */
    },
};

const uint16_t g_svc_sm_module_count =
    (uint16_t)(sizeof(g_svc_sm_modules) / sizeof(g_svc_sm_modules[0]));
```

### 5.3 Call Order Summary

```
Operation     Order       Direction
────────────────────────────────────
init()        forward     modules[0] → modules[N-1]
start()       forward     modules[0] → modules[N-1]
handle()      forward     modules[0] → modules[N-1]
stop()        reverse     modules[N-1] → modules[0]
shutdown()    reverse     modules[N-1] → modules[0]
deinit()      reverse     modules[N-1] → modules[0]
```

**Why forward for startup:** Low-level drivers (`svc_eep`) must initialize before
higher-level services that depend on them.

**Why reverse for shutdown:** Higher-level services (`svc_shell`) must stop before
the drivers they depend on are deinitialized.

### 5.4 Critical vs Non-Critical Modules

| Property | `critical = true` | `critical = false` |
|----------|-------------------|---------------------|
| Init/start failure | → ERROR immediately, abort startup | Error stored, startup continues |
| Handle failure | → ERROR immediately | Error stored, next handle() continues |
| Shutdown failure | → ERROR | Error stored, shutdown continues |

### 5.5 How to Add a New Module

**Step 1 — Ensure the module has the right function signatures.**

If your module takes arguments, write a wrapper:

```c
/* Wrapper because svc_sm expects dev_err_t (*)(void) */
static dev_err_t my_service_init_wrapper(void)
{
    return my_service_init(MY_SERVICE_CONFIG_DEFAULT);
}
```

**Step 2 — Add the entry to `svc_sm_modules.c`:**

```c
#include "my_service.h"

const svc_sm_module_t g_svc_sm_modules[] =
{
    /* ... existing entries ... */
    {
        "my_service",           /* name */
        my_service_init_wrapper,/* init     */
        my_service_start,       /* start    */
        my_service_handle,      /* handle   — called every superloop iteration */
        my_service_stop,        /* stop     */
        my_service_shutdown,    /* shutdown */
        my_service_deinit,      /* deinit   */
        false                   /* critical */
    },
};
```

**Step 3 — Update `svc_sm/CMakeLists.txt` to link your library:**

```cmake
target_link_libraries(svc_sm PUBLIC dev_common svc_eep svc_shell dev_uart my_service)
```

**Step 4 — If you need more than `SVC_SM_CFG_MAX_MODULES` (16) entries, increase the define.**

That's it. No dynamic registration. The table is `const`.

---

## 6. Application Lifecycle

### 6.1 Callback Reference

Declared in `app_lifecycle.h`, defined in `app/src/app_lifecycle.c`:

```c
dev_err_t app_init(void);       /* POST_INIT — modules are up, restore state here */
dev_err_t app_start(void);      /* POST_INIT — after app_init(), before RUN */
dev_err_t app_run(void);        /* RUN — every superloop iteration */
dev_err_t app_stop(void);       /* Shutdown — before modules stop (reserved) */
dev_err_t app_shutdown(void);   /* Shutdown — first step, save state here */
dev_err_t app_error(void);      /* ERROR — called once when error occurs */
```

### 6.2 Callback Timing Diagram

```
svc_sm_startup()
  │
  ├─ SVC_SM_STATE_INIT
  │    ├─ module[0].init()
  │    ├─ module[1].init()
  │    ├─ module[0].start()
  │    ├─ module[1].start()
  │
  ├─ SVC_SM_STATE_POST_INIT
  │    ├─ app_init()          ← ① your init code here
  │    └─ app_start()         ← ② your start code here
  │
  └─ SVC_SM_STATE_RUN
       └─ svc_sm_handle() called repeatedly ───┐
            ├─ module[0].handle()              │
            ├─ module[1].handle()              │
            └─ app_run()          ← ③ called every iteration
                                               │
  ┌────────────────────────────────────────────┘
  │
  ├─ Shutdown requested
  │    ├─ SVC_SM_STATE_PREPARE_SHUTDOWN
  │    │    └─ app_shutdown()   ← ④ save state, notify
  │    ├─ module[1].stop()      (reverse)
  │    ├─ module[0].stop()
  │    ├─ module[1].shutdown()
  │    ├─ module[0].shutdown()
  │    ├─ module[1].deinit()
  │    ├─ module[0].deinit()
  │    └─ SVC_SM_STATE_SHUTDOWN

  ── ERROR (from any state) ──
       └─ app_error()           ← ⑤ safe state, log, notify
```

### 6.3 Default Implementation (Weak)

The file `services/svc_sm/src/svc_sm_app.c` provides **weak defaults** for all six
callbacks — each simply returns `DEV_OK`. This means:

- If you **don't** create `app/src/app_lifecycle.c`, the system still compiles
  and runs. All callbacks are no-ops.
- If you **do** create `app/src/app_lifecycle.c` (strong symbols), the linker
  prefers your implementation over the weak defaults.

### 6.4 Example Application Lifecycle

```c
#include "app_lifecycle.h"
#include "dev_gpio.h"
#include "svc_eep.h"
#include "dev_log.h"

#define MY_APP_VERSION_FIELD    SVC_EEP_FIELD_APP_VERSION
#define MY_APP_VERSION          (0x0100U)   /* v1.0 */

dev_err_t app_init(void)
{
    uint16_t stored_version = 0U;

    /* Restore saved version from EEPROM */
    dev_err_t ret = svc_eep_read_u16(MY_APP_VERSION_FIELD, &stored_version);
    if (ret == DEV_OK)
    {
        DEV_LOG_INFO("EEPROM version: 0x%04X", stored_version);
    }
    else
    {
        DEV_LOG_INFO("First boot — no saved version");
    }

    return DEV_OK;
}

dev_err_t app_start(void)
{
    /* Turn on status LED */
    dev_gpio_output(DEV_GPIO_LED_STATUS);
    dev_gpio_high(DEV_GPIO_LED_STATUS);

    DEV_LOG_INFO("Application started");
    return DEV_OK;
}

dev_err_t app_run(void)
{
    /* Periodic application logic — called every superloop iteration.
     * Return quickly. Use tick-based polling for timed operations. */

    static uint32_t last_blink_ms = 0U;
    uint32_t now = osal_get_tick_ms();

    if ((now - last_blink_ms) >= 500U)
    {
        last_blink_ms = now;
        dev_gpio_toggle(DEV_GPIO_LED_HEARTBEAT);
    }

    return DEV_OK;
}

dev_err_t app_shutdown(void)
{
    /* Save application state before shutdown */
    dev_err_t ret = svc_eep_write_u16(MY_APP_VERSION_FIELD, MY_APP_VERSION);
    if (ret == DEV_OK)
    {
        ret = svc_eep_flush(SVC_EEP_ID_PRIMARY);
    }

    DEV_LOG_INFO("Application shutting down (ret=%d)", (int)ret);
    return ret;
}

dev_err_t app_error(void)
{
    /* Enter safe state */
    dev_gpio_low(DEV_GPIO_LED_STATUS);

    /* Blink error code on LED */
    /* ... */

    DEV_LOG_ERROR("Application entered ERROR state");
    return DEV_OK;
}
```

---

## 7. Configuration

All compile-time configuration lives in `services/svc_sm/include/svc_sm_cfg.h`.
Edit this single file to tune the state machine.

### 7.1 Config Knobs Reference

```c
/* ── Module limits ── */

/* Maximum number of entries in g_svc_sm_modules[].
 * Increase if you add more modules. */
#define SVC_SM_CFG_MAX_MODULES                  (16U)


/* ── Runtime checks ── */

/* Enable parameter validation and null-pointer checks in public API.
 * Set to 0U to save code size in production (removes DEV_CHECK_PTR_RET etc). */
#define SVC_SM_CFG_RUNTIME_CHECK_ENABLED        (1U)


/* ── Feature toggles ── */

/* Enable ERROR state handling.
 * When 0U: svc_sm_request_error() returns DEV_ERR_NOT_SUPPORTED.
 *           Critical module failures are ignored.
 *           No error info is stored. */
#define SVC_SM_CFG_ERROR_STATE_ENABLED          (1U)

/* Enable sleep/wakeup states (PREPARE_SLEEP, SLEEP, WAKEUP).
 * When 0U: sleep states are compiled out entirely. */
#define SVC_SM_CFG_SLEEP_ENABLED                (0U)

/* Enable shutdown request processing.
 * When 0U: svc_sm_request_shutdown() returns DEV_ERR_NOT_SUPPORTED. */
#define SVC_SM_CFG_SHUTDOWN_ENABLED             (1U)

/* When 1U: shutdown is "safe" — each step runs even if a previous step failed.
 * When 0U: shutdown aborts on first error. */
#define SVC_SM_CFG_SAFE_SHUTDOWN_ENABLED        (1U)


/* ── Runtime behavior ── */

/* When 1U: app_run() is called from svc_sm_handle() during RUN state.
 * When 0U: app_run() is never called. Useful if app logic runs elsewhere. */
#define SVC_SM_CFG_CALL_APP_RUN_IN_HANDLE       (1U)

/* When 1U: module handle() callbacks are called from svc_sm_handle() during RUN.
 * When 0U: module handles are never called. Useful for pure event-driven designs. */
#define SVC_SM_CFG_CALL_MODULE_HANDLE_IN_RUN    (1U)


/* ── Logging ── */

/* When 1U: error transitions are logged via DEV_LOG.
 * Currently reserved for future use — not yet wired in. */
#define SVC_SM_CFG_USE_DEV_LOG                  (1U)
```

### 7.2 Configuration Scenarios

**Minimal production build (smallest code size):**

```c
#define SVC_SM_CFG_RUNTIME_CHECK_ENABLED        (0U)   /* drop checks */
#define SVC_SM_CFG_ERROR_STATE_ENABLED          (0U)   /* drop error handling */
#define SVC_SM_CFG_SLEEP_ENABLED                (0U)   /* no sleep states */
#define SVC_SM_CFG_SHUTDOWN_ENABLED             (0U)   /* no graceful shutdown */
#define SVC_SM_CFG_SAFE_SHUTDOWN_ENABLED        (0U)
#define SVC_SM_CFG_CALL_APP_RUN_IN_HANDLE       (0U)   /* app_run() not called */
#define SVC_SM_CFG_CALL_MODULE_HANDLE_IN_RUN    (0U)   /* module handles not called */
#define SVC_SM_CFG_USE_DEV_LOG                  (0U)
```

**Full-featured development build:**

```c
#define SVC_SM_CFG_RUNTIME_CHECK_ENABLED        (1U)
#define SVC_SM_CFG_ERROR_STATE_ENABLED          (1U)
#define SVC_SM_CFG_SLEEP_ENABLED                (0U)   /* not yet implemented */
#define SVC_SM_CFG_SHUTDOWN_ENABLED             (1U)
#define SVC_SM_CFG_SAFE_SHUTDOWN_ENABLED        (1U)
#define SVC_SM_CFG_CALL_APP_RUN_IN_HANDLE       (1U)
#define SVC_SM_CFG_CALL_MODULE_HANDLE_IN_RUN    (1U)
#define SVC_SM_CFG_USE_DEV_LOG                  (1U)
```

### 7.3 Config Interaction Matrix

What happens when you combine settings:

| ERROR enabled | SHUTDOWN enabled | Behavior |
|---------------|-----------------|----------|
| `1U` | `1U` | Full state machine: RUN → ERROR → PREPARE_SHUTDOWN → SHUTDOWN |
| `1U` | `0U` | Error handling works but shutdown is disabled — system stays in ERROR |
| `0U` | `1U` | No error handling — errors are silently ignored, shutdown still works |
| `0U` | `0U` | Bare minimum: UNINIT → STARTUP → INIT → POST_INIT → RUN → (nothing else) |

---

## 8. End-to-End Examples

### 8.1 Minimal Superloop main()

```c
#include "main.h"
#include "osal.h"
#include "svc_sm.h"

int main(void)
{
    /* ── Hardware setup ── */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_I2C1_Init();

    /* ── Platform ── */
    (void)osal_init();

    /* ── State manager ── */
    (void)svc_sm_init();
    (void)svc_sm_startup();

    /* ── Superloop ── */
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

### 8.2 Requesting Shutdown from a Shell Command

```c
/* In svc_shell command handler: */
static dev_err_t cmd_shutdown(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    dev_err_t ret = svc_sm_request_shutdown();
    if (ret != DEV_OK)
    {
        svc_shell_write_line("Shutdown not available in current state");
        return ret;
    }

    svc_shell_write_line("Shutdown requested — shutting down...");
    return DEV_OK;
}
```

### 8.3 Requesting Error from a Module Handle

```c
/* In a module handle callback: */
static dev_err_t sensor_module_handle(void)
{
    if (sensor_overcurrent_detected())
    {
        /* Request system to enter ERROR state */
        (void)svc_sm_request_error(DEV_ERR_HW_FAILURE);
        return DEV_ERR_HW_FAILURE;
    }

    return DEV_OK;
}
```

### 8.4 Reading and Displaying Last Error

```c
static dev_err_t cmd_last_error(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    svc_sm_error_info_t info;
    dev_err_t ret = svc_sm_get_last_error(&info);

    if (ret == DEV_ERR_NOT_FOUND)
    {
        svc_shell_write_line("No error recorded");
        return DEV_OK;
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
             "Error: %d | State: %d | Module: %s",
             (int)info.error,
             (int)info.state,
             info.module_name ? info.module_name : "(none)");
    svc_shell_write_line(buf);

    return DEV_OK;
}
```

### 8.5 Watching for Errors in the Superloop

```c
int main(void)
{
    /* ... init ... */

    for (;;)
    {
        (void)svc_sm_handle();

        /* Check if system entered ERROR state */
        if (svc_sm_get_state() == SVC_SM_STATE_ERROR)
        {
            svc_sm_error_info_t info;
            if (svc_sm_get_last_error(&info) == DEV_OK)
            {
                /* Take emergency action */
                emergency_led_blink((uint8_t)info.error);
            }

            /* Request shutdown after brief delay for diagnostics */
            osal_delay_ms(3000U);
            (void)svc_sm_request_shutdown();
        }

        if (svc_sm_get_state() == SVC_SM_STATE_SHUTDOWN)
        {
            break;
        }
    }

    return 0;
}
```

---

## 9. CMake Integration

### Root `CMakeLists.txt`

```cmake
# Add the svc_sm library (depends on svc_eep, svc_shell, dev_uart)
add_subdirectory(services/svc_sm)

# Add application sources to the main executable
target_sources(${PROJECT_NAME} PRIVATE
    app/src/app_lifecycle.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    app/include
)

# Link everything
target_link_libraries(${PROJECT_NAME}
    osal
    dev_common
    dev_gpio
    dev_uart
    svc_shell
    svc_eep
    svc_sm
)
```

### `services/svc_sm/CMakeLists.txt`

```cmake
add_library(svc_sm STATIC
    src/svc_sm.c
    src/svc_sm_modules.c
    src/svc_sm_app.c
)

target_include_directories(svc_sm PUBLIC include)
target_link_libraries(svc_sm PUBLIC dev_common svc_eep svc_shell dev_uart)
```

Any target linking against `svc_sm` automatically gets `services/svc_sm/include`
in its include path and can use `#include "svc_sm.h"`.

---

## 10. Dependency Rules

### Allowed

```
svc_sm  ──►  dev_common       (types, errors, assert macros)
svc_sm  ──►  osal              (time/delay — if needed in the future)
svc_sm  ──►  app_lifecycle     (application callbacks)
svc_sm  ──►  svc_eep           (through module table)
svc_sm  ──►  svc_shell         (through module table)
```

### Forbidden

```
svc_sm  ──✗  STM32 HAL headers
svc_sm  ──✗  FreeRTOS headers
svc_sm  ──✗  Direct hardware registers
```

If `svc_sm` needs time or delay, it uses `osal_get_tick_ms()` / `osal_delay_ms()`
— never `HAL_GetTick()` or `vTaskDelay()` directly.

---

## 11. Design Decisions

### Why static module table instead of dynamic registration?

- **Predictable**: all modules known at compile time.
- **Safe**: no function pointers from runtime data, no use-after-register bugs.
- **Small**: no linked-list or dynamic array overhead.
- **Reviewable**: a single table shows the entire init order at a glance.

### Why deferred requests instead of immediate state change?

- **Reentrant-safe**: `svc_sm_request_shutdown()` can be called from interrupt
  context (in future RTOS builds). It only sets a flag.
- **Predictable**: state changes happen at known points — inside `svc_sm_handle()`.
- **Testable**: the request + process split makes it easy to unit-test each side.

### Why weak app lifecycle defaults?

- **Compiles without app code**: clean build when iterating on services.
- **No linker errors**: app is optional until you need it.
- **Override only what you need**: define `app_run()` and leave the rest as weak
  `DEV_OK` stubs.

---

## 12. Testing Guide

### Host-based tests (mock ports)

The state machine logic in `svc_sm.c` is hardware-independent. You can test it on
a Linux host by:

1. Using mock ports for `dev_uart`, `dev_i2c`.
2. Creating a test module table with mock entries.
3. Writing unit tests for each state transition.

### Recommended test coverage

**Startup tests:**
- `svc_sm_init()` returns `DEV_OK`
- Double init returns `DEV_ERR_ALREADY_INITIALIZED`
- Startup without init returns `DEV_ERR_NOT_INITIALIZED`
- Normal startup reaches RUN
- Module init called in forward order
- Module start called in forward order
- `app_init()` / `app_start()` called
- Critical module failure enters ERROR and aborts
- Non-critical module failure stores error and continues
- App lifecycle failure enters ERROR

**Runtime tests:**
- Module handle called in RUN
- `app_run()` called in RUN
- Handle without init returns `DEV_ERR_NOT_INITIALIZED`

**Shutdown tests:**
- `svc_sm_request_shutdown()` from RUN returns `DEV_OK`
- Shutdown reaches `SHUTDOWN` state
- Module stop/shutdown/deinit called in reverse order
- `app_shutdown()` called before modules
- Shutdown from invalid state returns `DEV_ERR_INVALID_STATE`

**Error tests:**
- `svc_sm_request_error()` stores error info
- Error info is retrievable via `svc_sm_get_last_error()`
- `app_error()` called once per error entry
- Shutdown can be requested from ERROR
- Invalid transition rejected

**Config tests:**
- Disabling `SVC_SM_CFG_SHUTDOWN_ENABLED` makes `svc_sm_request_shutdown()` return `DEV_ERR_NOT_SUPPORTED`
- Disabling `SVC_SM_CFG_ERROR_STATE_ENABLED` makes `svc_sm_request_error()` return `DEV_ERR_NOT_SUPPORTED`
- Sleep states are absent when `SVC_SM_CFG_SLEEP_ENABLED = 0U`

---

## 13. Quick Reference Card

```
┌────────────────────────────────────────────────────────────────────┐
│                      SVC_SM Quick Reference                        │
├──────────────┬─────────────────────────────────────────────────────┤
│ svc_sm_init()│ Call once. Resets state to UNINIT.                  │
├──────────────┼─────────────────────────────────────────────────────┤
│ startup()    │ UNINIT→STARTUP→INIT→POST_INIT→RUN.                  │
│              │ Modules init/start forward, then app_init/start.    │
├──────────────┼─────────────────────────────────────────────────────┤
│ handle()     │ Call from superloop. Processes requests, calls      │
│              │ module handles (forward) and app_run().             │
├──────────────┼─────────────────────────────────────────────────────┤
│ shutdown()   │ PREPARE_SHUTDOWN → app_shutdown → module stop       │
│              │ (reverse) → shutdown (reverse) → deinit (reverse).  │
├──────────────┼─────────────────────────────────────────────────────┤
│ requests     │ DEFERRED — processed next handle().                 │
│              │ shutdown: from RUN or ERROR. error: from any state. │
├──────────────┼─────────────────────────────────────────────────────┤
│ get_state()  │ Current state. Check for SHUTDOWN to exit loop.     │
├──────────────┼─────────────────────────────────────────────────────┤
│ error info   │ svc_sm_get_last_error() → {error, state, module}.   │
│              │ Cleared on svc_sm_init().                           │
├──────────────┼─────────────────────────────────────────────────────┤
│ modules[]    │ STATIC const table in svc_sm_modules.c.             │
│              │ Order matters: forward for init, reverse for stop.  │
├──────────────┼─────────────────────────────────────────────────────┤
│ app_*()      │ 6 weak callbacks. Override in app_lifecycle.c.      │
│              │ All return DEV_OK by default.                       │
├──────────────┼─────────────────────────────────────────────────────┤
│ config       │ svc_sm_cfg.h — toggle features, max modules,        │
│              │ runtime checks, logging.                            │
└──────────────┴─────────────────────────────────────────────────────┘
```
