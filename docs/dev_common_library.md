# dev_common — Foundation Library

**Version:** 0.1.0  
**Language:** C99/C11  
**Dependencies:** None (standalone)  
**MISRA-C oriented:** Yes

---

## 1. Overview

`dev_common` is the foundation component shared by all `dev_*` driver modules. It provides:

- Fixed-width integer types and standard headers
- Unified error code enumeration (`dev_err_t`)
- Configurable assert / check macro system
- Compiler abstraction macros
- Version information

`dev_common` must NOT depend on any hardware driver component. It is pure software infrastructure.

---

## 2. Include

```c
#include "dev_common.h"   /* Umbrella — brings in all dev_common headers */
```

Or individually:

```c
#include "dev_types.h"     /* stdint, stdbool, stddef */
#include "dev_error.h"     /* dev_err_t */
#include "dev_assert.h"    /* assert macros, backends */
#include "dev_compiler.h"  /* compiler abstraction */
#include "dev_version.h"   /* version macros */
```

---

## 3. Error Codes — `dev_error.h`

### 3.1 Type

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

### 3.2 Error Code Reference

| Value | Meaning | Typical Context |
|-------|---------|-----------------|
| `DEV_OK` (0) | Success | All operations that succeed |
| `DEV_ERR_FAIL` | Generic failure | Catch-all for unspecified errors |
| `DEV_ERR_INVALID_ARG` | Invalid function argument | Out-of-range enum, bad channel count |
| `DEV_ERR_NULL_PTR` | Unexpected NULL pointer | NULL config, NULL output pointer |
| `DEV_ERR_INVALID_STATE` | Operation invalid in current state | Write to input-only channel |
| `DEV_ERR_NOT_INITIALIZED` | Module not initialized | API called before `_init()` |
| `DEV_ERR_ALREADY_INITIALIZED` | Duplicate initialization | `_init()` called twice |
| `DEV_ERR_NOT_SUPPORTED` | Feature unsupported by port | Both-edge interrupt on simple GPIO |
| `DEV_ERR_TIMEOUT` | Operation timed out | (reserved for future use) |
| `DEV_ERR_BUSY` | Resource busy | (reserved for future use) |
| `DEV_ERR_OUT_OF_RANGE` | Value out of valid range | Channel index overflow |
| `DEV_ERR_HW_FAILURE` | Hardware / port failure | Vendor HAL returned error |
| `DEV_ERR_CONFIG` | Static configuration invalid | Duplicate channels, bad compile-time settings |

---

## 4. Assert / Check System — `dev_assert.h`

### 4.1 Overview

The assert system provides configurable error reporting with six backends. It distinguishes between **fatal asserts** (program must stop) and **non-fatal checks** (report + return error).

### 4.2 Assert Backends

```c
typedef enum {
    DEV_ASSERT_BACKEND_NONE = 0,      /* Silent — no output */
    DEV_ASSERT_BACKEND_UART,           /* Send message via output_hook() */
    DEV_ASSERT_BACKEND_TEXT_BUFFER,    /* Write message into text_buffer */
    DEV_ASSERT_BACKEND_BREAKPOINT,     /* Trigger __builtin_trap() (ASSERT only) */
    DEV_ASSERT_BACKEND_RESET,          /* Call reset_hook() then loop (ASSERT only) */
    DEV_ASSERT_BACKEND_USER_HOOK       /* Call user_hook() (ASSERT: then loop) */
} dev_assert_backend_t;
```

### 4.3 Assert Types

```c
typedef enum {
    DEV_ASSERT_TYPE_ASSERT = 0,   /* Fatal — never returns */
    DEV_ASSERT_TYPE_CHECK,         /* Non-fatal — returns after reporting */
    DEV_ASSERT_TYPE_ERROR          /* Non-fatal — returns after reporting */
} dev_assert_type_t;
```

### 4.4 Configuration

```c
typedef void (*dev_assert_user_hook_t)(const dev_assert_info_t *info);
typedef void (*dev_assert_output_hook_t)(const char *text);
typedef void (*dev_assert_reset_hook_t)(void);

typedef struct {
    const char       *file;
    uint32_t          line;
    dev_assert_type_t type;
    dev_err_t         error;
} dev_assert_info_t;

typedef struct {
    dev_assert_backend_t     backend;          /* Selected backend */
    dev_assert_output_hook_t output_hook;      /* UART message output */
    dev_assert_user_hook_t   user_hook;        /* USER_HOOK / RESET callback */
    dev_assert_reset_hook_t  reset_hook;       /* Platform reset function */
    char                    *text_buffer;      /* TEXT_BUFFER destination */
    uint16_t                 text_buffer_size;  /* TEXT_BUFFER max bytes */
} dev_assert_config_t;
```

### 4.5 API Functions

```c
void dev_assert_init(const dev_assert_config_t *config);
void dev_assert_report(const char *file, uint32_t line,
                       dev_assert_type_t type, dev_err_t error);
```

`dev_assert_init()` copies the config struct. If `config` is NULL, defaults to `DEV_ASSERT_BACKEND_NONE`.

`dev_assert_report()` behavior depends on `type`:

**Fatal (`DEV_ASSERT_TYPE_ASSERT`):** Never returns.

| Backend | Sequence |
|---------|----------|
| `NONE` | Infinite loop (safe halt) |
| `UART` | Call `output_hook(msg)`, then infinite loop |
| `TEXT_BUFFER` | Write `msg` into `text_buffer`, then infinite loop |
| `BREAKPOINT` | Call `output_hook(msg)` if set, then `__builtin_trap()` |
| `RESET` | Call `user_hook(info)`, call `reset_hook()`, then infinite loop |
| `USER_HOOK` | Call `user_hook(info)`, then infinite loop |

**Non-fatal (`CHECK` / `ERROR`):** Returns after reporting.

| Backend | Sequence |
|---------|----------|
| `NONE` | Return immediately |
| `UART` | Call `output_hook(msg)`, return |
| `TEXT_BUFFER` | Write `msg` into `text_buffer`, return |
| `BREAKPOINT` | Return (no trap for non-fatal) |
| `RESET` | Call `user_hook(info)`, return (no reset for non-fatal) |
| `USER_HOOK` | Call `user_hook(info)`, return |

### 4.6 Check / Assert Macros

```c
/* Validate condition; report and return error_code on failure */
#define DEV_CHECK_RET(condition, error_code)

/* Validate pointer is not NULL; return DEV_ERR_NULL_PTR on failure */
#define DEV_CHECK_PTR_RET(pointer)

/* Call expression; if it returns non-DEV_OK, report and propagate the error */
#define DEV_CHECK_OK_RET(expression)

/* Fatal assert — never returns if condition is false */
#define DEV_ASSERT(condition)
```

**Rules:**
- All macros use `do { } while (false)` pattern.
- Arguments are evaluated exactly once.
- `_RET` suffix indicates the macro may return from the calling function.
- `DEV_ASSERT` is fatal; `dev_assert_report()` with `DEV_ASSERT_TYPE_ASSERT` must not return.
- No dynamic allocation, no vendor API calls, no complex logic in macros.

**Usage examples:**

```c
dev_err_t my_function(int *ptr, int value)
{
    DEV_CHECK_PTR_RET(ptr);                                  /* null guard */
    DEV_CHECK_RET((value >= 0) && (value <= 100), DEV_ERR_INVALID_ARG);

    DEV_CHECK_OK_RET(dev_gpio_write(LED, DEV_GPIO_LEVEL_HIGH)); /* propagate */

    *ptr = value * 2;
    return DEV_OK;
}

void critical_section(void)
{
    DEV_ASSERT(SystemCoreClock > 0U);   /* fatal if clock not running */
}
```

### 4.7 Fatal Assert Loops — MISRA Deviation

The infinite loops used in fatal assert termination (`for (;;) {}`) are an intentional, documented deviation from the "no unbounded loops" project rule (DEV-MISRA-001). When an assertion fires in a safety-critical system, there is no safe recovery — the system must halt at a deterministic, debuggable point.

---

## 5. Compiler Abstraction — `dev_compiler.h`

### 5.1 Utility Macros

| Macro | Purpose |
|-------|---------|
| `DEV_UNUSED(x)` | Suppress "unused parameter" warnings |
| `DEV_ARRAY_SIZE(a)` | Compile-time array element count |
| `DEV_STATIC_ASSERT(cond, msg)` | Compile-time assertion |

### 5.2 Compiler-Specific Attributes (GCC / Clang)

| Macro | Purpose |
|-------|---------|
| `DEV_WEAK` | Weak symbol (`__attribute__((weak))`) |
| `DEV_PACKED` | Packed struct (`__attribute__((packed))`) |
| `DEV_ALIGNED(n)` | Alignment (`__attribute__((aligned(n)))`) |
| `DEV_NORETURN` | Function does not return |
| `DEV_SECTION(s)` | Place in named section |
| `DEV_BREAKPOINT()` | Debugger breakpoint (`__builtin_trap()`) |

Unsupported compilers generate `#error "Unsupported compiler"`.

### 5.3 Usage

```c
static int DEV_SECTION(".noinit") retained_counter;

DEV_NORETURN void panic(void)
{
    DEV_BREAKPOINT();
    for (;;) {}
}

DEV_STATIC_ASSERT(sizeof(my_struct) == 16U, "my_struct size wrong");
```

---

## 6. Version — `dev_version.h`

```c
#define DEV_VERSION_MAJOR  0U
#define DEV_VERSION_MINOR  1U
#define DEV_VERSION_PATCH  0U
```

---

## 7. Types — `dev_types.h`

```c
#include <stdint.h>    /* uint8_t .. uint64_t, int8_t .. int64_t, uintptr_t */
#include <stdbool.h>   /* bool, true, false */
#include <stddef.h>    /* size_t, NULL */
```

All `dev_*` modules use fixed-width integer types and `bool`. No custom aliases for standard types are created unless required by the project.

---

## 8. Umbrella Header — `dev_common.h`

```c
#include "dev_types.h"
#include "dev_error.h"
#include "dev_assert.h"
#include "dev_compiler.h"
#include "dev_version.h"
```

Include this single header to get all `dev_common` functionality.

---

## 9. Build Integration

### 9.1 Include Paths

```
drivers/dev_common/include
```

### 9.2 Source Files

```
drivers/dev_common/src/dev_common.c    (placeholder — required for build)
drivers/dev_common/src/dev_assert.c    (assert report implementation)
```

### 9.3 CMake

```cmake
target_include_directories(my_app PRIVATE drivers/dev_common/include)
target_sources(my_app PRIVATE
    drivers/dev_common/src/dev_common.c
    drivers/dev_common/src/dev_assert.c
)
```

---

## 10. Usage Example

```c
#include "dev_common.h"
#include <stdio.h>

/* UART output hook for assert messages */
static void my_uart_output(const char *text)
{
    /* platform-specific: send text to debug UART */
    (void)text;
}

/* Platform reset hook */
static void my_reset(void)
{
    /* platform-specific: trigger system reset */
}

int main(void)
{
    dev_assert_config_t assert_cfg = {
        .backend          = DEV_ASSERT_BACKEND_UART,
        .output_hook      = my_uart_output,
        .user_hook        = NULL,
        .reset_hook       = my_reset,
        .text_buffer      = NULL,
        .text_buffer_size = 0U
    };

    dev_assert_init(&assert_cfg);

    /* Use check macros for parameter validation */
    int val = 42;
    DEV_CHECK_RET((val > 0), DEV_ERR_INVALID_ARG);

    /* DEV_OK is 0 — success */
    return (int)DEV_OK;
}
```
