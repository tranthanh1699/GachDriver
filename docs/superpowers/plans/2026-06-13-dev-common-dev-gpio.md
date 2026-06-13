# dev_common + dev_gpio Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the `dev_common` foundation and `dev_gpio` driver with mock port, fully testable on host Linux.

**Architecture:** Header-only `dev_common` types + `dev_assert` with configurable backends. `dev_gpio` common logic validates and dispatches to a port interface. Mock port enables host testing of all 36 test cases without hardware. Follows lower_case naming (`dev_err_t`, `dev_gpio_init`), MISRA-C oriented, no dynamic allocation.

**Tech Stack:** C11, GCC/Clang on Linux host, CMake for build

---

### Task 1: Directory structure + dev_types.h + dev_error.h

**Files:**
- Create: `drivers/dev_common/include/dev_types.h`
- Create: `drivers/dev_common/include/dev_error.h`

- [ ] **Step 1: Create directory structure**

```bash
mkdir -p drivers/dev_common/include
mkdir -p drivers/dev_common/src
mkdir -p drivers/dev_gpio/include
mkdir -p drivers/dev_gpio/src
mkdir -p drivers/dev_gpio/port/mock
mkdir -p boards/board_mock
mkdir -p tests/dev_gpio
```

- [ ] **Step 2: Write drivers/dev_common/include/dev_types.h**

```c
#ifndef DEV_TYPES_H
#define DEV_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
}
#endif

#endif /* DEV_TYPES_H */
```

- [ ] **Step 3: Write drivers/dev_common/include/dev_error.h**

```c
#ifndef DEV_ERROR_H
#define DEV_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

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

#ifdef __cplusplus
}
#endif

#endif /* DEV_ERROR_H */
```

- [ ] **Step 4: Commit**

```bash
git add drivers/
git commit -m "feat: add dev_types.h and dev_error.h with dev_err_t enum

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 2: dev_compiler.h + dev_version.h

**Files:**
- Create: `drivers/dev_common/include/dev_compiler.h`
- Create: `drivers/dev_common/include/dev_version.h`

- [ ] **Step 1: Write drivers/dev_common/include/dev_compiler.h**

```c
#ifndef DEV_COMPILER_H
#define DEV_COMPILER_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_UNUSED(x)                ((void)(x))
#define DEV_ARRAY_SIZE(a)            (sizeof(a) / sizeof((a)[0]))
#define DEV_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)

#if defined(__GNUC__) || defined(__clang__)
  #define DEV_WEAK                   __attribute__((weak))
  #define DEV_PACKED                 __attribute__((packed))
  #define DEV_ALIGNED(n)             __attribute__((aligned(n)))
  #define DEV_NORETURN               __attribute__((noreturn))
  #define DEV_SECTION(s)             __attribute__((section(s)))
  #define DEV_BREAKPOINT()           __builtin_trap()
#else
  #error "Unsupported compiler"
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEV_COMPILER_H */
```

- [ ] **Step 2: Write drivers/dev_common/include/dev_version.h**

```c
#ifndef DEV_VERSION_H
#define DEV_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_VERSION_MAJOR  0U
#define DEV_VERSION_MINOR  1U
#define DEV_VERSION_PATCH  0U

#ifdef __cplusplus
}
#endif

#endif /* DEV_VERSION_H */
```

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_common/include/dev_compiler.h drivers/dev_common/include/dev_version.h
git commit -m "feat: add dev_compiler.h and dev_version.h

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 3: dev_assert.h + dev_assert.c + dev_common.h + dev_common.c

**Files:**
- Create: `drivers/dev_common/include/dev_assert.h`
- Create: `drivers/dev_common/src/dev_assert.c`
- Create: `drivers/dev_common/include/dev_common.h`
- Create: `drivers/dev_common/src/dev_common.c`

- [ ] **Step 1: Write drivers/dev_common/include/dev_assert.h**

```c
#ifndef DEV_ASSERT_H
#define DEV_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_error.h"
#include "dev_compiler.h"

typedef enum {
    DEV_ASSERT_BACKEND_NONE = 0,
    DEV_ASSERT_BACKEND_UART,
    DEV_ASSERT_BACKEND_TEXT_BUFFER,
    DEV_ASSERT_BACKEND_BREAKPOINT,
    DEV_ASSERT_BACKEND_RESET,
    DEV_ASSERT_BACKEND_USER_HOOK
} dev_assert_backend_t;

typedef enum {
    DEV_ASSERT_TYPE_ASSERT = 0,
    DEV_ASSERT_TYPE_CHECK,
    DEV_ASSERT_TYPE_ERROR
} dev_assert_type_t;

typedef struct {
    const char       *file;
    uint32_t          line;
    dev_assert_type_t type;
    dev_err_t         error;
} dev_assert_info_t;

typedef void (*dev_assert_user_hook_t)(const dev_assert_info_t *info);
typedef void (*dev_assert_output_hook_t)(const char *text);
typedef void (*dev_assert_reset_hook_t)(void);

typedef struct {
    dev_assert_backend_t     backend;
    dev_assert_output_hook_t output_hook;
    dev_assert_user_hook_t   user_hook;
    dev_assert_reset_hook_t  reset_hook;
    char                    *text_buffer;
    uint16_t                 text_buffer_size;
} dev_assert_config_t;

void dev_assert_init(const dev_assert_config_t *config);

void dev_assert_report(const char *file,
                       uint32_t line,
                       dev_assert_type_t type,
                       dev_err_t error);

/* Assert/check macros — do { } while (false), args evaluated once */

#define DEV_CHECK_RET(condition, error_code)                         \
    do {                                                             \
        if (!(condition)) {                                          \
            dev_err_t _dev_check_err = (error_code);                 \
            dev_assert_report(__FILE__, (uint32_t)__LINE__,          \
                              DEV_ASSERT_TYPE_CHECK, _dev_check_err); \
            return _dev_check_err;                                    \
        }                                                             \
    } while (false)

#define DEV_CHECK_PTR_RET(pointer) \
    DEV_CHECK_RET(((pointer) != NULL), DEV_ERR_NULL_PTR)

#define DEV_CHECK_OK_RET(expression)                                 \
    do {                                                             \
        dev_err_t _err = (expression);                               \
        if (_err != DEV_OK) {                                        \
            dev_assert_report(__FILE__, (uint32_t)__LINE__,          \
                              DEV_ASSERT_TYPE_CHECK, _err);          \
            return _err;                                              \
        }                                                             \
    } while (false)

#define DEV_ASSERT(condition)                                        \
    do {                                                             \
        if (!(condition)) {                                          \
            dev_assert_report(__FILE__, (uint32_t)__LINE__,          \
                              DEV_ASSERT_TYPE_ASSERT, DEV_ERR_FAIL); \
            /* dev_assert_report MUST NOT return for ASSERT type.    */ \
            /* Compiler safeguard in case of misconfiguration:       */ \
            for (;;) {}                                              \
        }                                                             \
    } while (false)

#ifdef __cplusplus
}
#endif

#endif /* DEV_ASSERT_H */
```

- [ ] **Step 2: Write drivers/dev_common/src/dev_assert.c**

```c
#include "dev_assert.h"
#include <stdio.h>

#define DEV_ASSERT_MSG_MAX_LEN  128U

static dev_assert_config_t g_dev_assert_config;

static void dev_assert_format_message(const dev_assert_info_t *info, char *buf, uint16_t buf_size)
{
    const char *type_str;

    switch (info->type) {
    case DEV_ASSERT_TYPE_ASSERT:
        type_str = "ASSERT";
        break;
    case DEV_ASSERT_TYPE_CHECK:
        type_str = "CHECK";
        break;
    case DEV_ASSERT_TYPE_ERROR:
        type_str = "ERROR";
        break;
    default:
        type_str = "UNKNOWN";
        break;
    }

    (void)snprintf(buf, buf_size, "[%s] %s:%u: error=%d",
                   type_str, info->file, (unsigned int)info->line, (int)info->error);
}

static void dev_assert_fatal_loop(void)
{
    for (;;) {}
}

void dev_assert_init(const dev_assert_config_t *config)
{
    if (config != NULL) {
        g_dev_assert_config = *config;
    } else {
        g_dev_assert_config.backend     = DEV_ASSERT_BACKEND_NONE;
        g_dev_assert_config.output_hook  = NULL;
        g_dev_assert_config.user_hook    = NULL;
        g_dev_assert_config.reset_hook   = NULL;
        g_dev_assert_config.text_buffer  = NULL;
        g_dev_assert_config.text_buffer_size = 0U;
    }
}

void dev_assert_report(const char *file, uint32_t line,
                       dev_assert_type_t type, dev_err_t error)
{
    dev_assert_info_t info;
    char msg[DEV_ASSERT_MSG_MAX_LEN];

    info.file  = file;
    info.line  = line;
    info.type  = type;
    info.error = error;

    dev_assert_format_message(&info, msg, (uint16_t)DEV_ARRAY_SIZE(msg));

    if (type == DEV_ASSERT_TYPE_ASSERT) {
        /* Fatal — must not return */
        switch (g_dev_assert_config.backend) {
        case DEV_ASSERT_BACKEND_NONE:
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_UART:
            if (g_dev_assert_config.output_hook != NULL) {
                g_dev_assert_config.output_hook(msg);
            }
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_TEXT_BUFFER:
            if ((g_dev_assert_config.text_buffer != NULL) &&
                (g_dev_assert_config.text_buffer_size > 0U)) {
                (void)snprintf(g_dev_assert_config.text_buffer,
                               g_dev_assert_config.text_buffer_size,
                               "%s", msg);
            }
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_BREAKPOINT:
            if (g_dev_assert_config.output_hook != NULL) {
                g_dev_assert_config.output_hook(msg);
            }
            DEV_BREAKPOINT();
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_RESET:
            if (g_dev_assert_config.user_hook != NULL) {
                g_dev_assert_config.user_hook(&info);
            }
            if (g_dev_assert_config.reset_hook != NULL) {
                g_dev_assert_config.reset_hook();
            }
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_USER_HOOK:
            if (g_dev_assert_config.user_hook != NULL) {
                g_dev_assert_config.user_hook(&info);
            }
            dev_assert_fatal_loop();
            break;
        default:
            dev_assert_fatal_loop();
            break;
        }
    } else {
        /* Non-fatal — return after reporting */
        switch (g_dev_assert_config.backend) {
        case DEV_ASSERT_BACKEND_NONE:
            break;
        case DEV_ASSERT_BACKEND_UART:
            if (g_dev_assert_config.output_hook != NULL) {
                g_dev_assert_config.output_hook(msg);
            }
            break;
        case DEV_ASSERT_BACKEND_TEXT_BUFFER:
            if ((g_dev_assert_config.text_buffer != NULL) &&
                (g_dev_assert_config.text_buffer_size > 0U)) {
                (void)snprintf(g_dev_assert_config.text_buffer,
                               g_dev_assert_config.text_buffer_size,
                               "%s", msg);
            }
            break;
        case DEV_ASSERT_BACKEND_BREAKPOINT:
            /* No breakpoint for non-fatal checks */
            break;
        case DEV_ASSERT_BACKEND_RESET:
        case DEV_ASSERT_BACKEND_USER_HOOK:
            if (g_dev_assert_config.user_hook != NULL) {
                g_dev_assert_config.user_hook(&info);
            }
            break;
        default:
            break;
        }
    }
}
```

- [ ] **Step 3: Write drivers/dev_common/include/dev_common.h**

```c
#ifndef DEV_COMMON_H
#define DEV_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_error.h"
#include "dev_assert.h"
#include "dev_compiler.h"
#include "dev_version.h"

#ifdef __cplusplus
}
#endif

#endif /* DEV_COMMON_H */
```

- [ ] **Step 4: Write drivers/dev_common/src/dev_common.c**

```c
#include "dev_common.h"

/* Placeholder for shared helpers. Currently all functionality is
 * provided via headers (types, macros) and dev_assert.c. */
```

- [ ] **Step 5: Commit**

```bash
git add drivers/dev_common/
git commit -m "feat: add dev_assert system, dev_common umbrella header and source

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 4: dev_gpio_types.h + dev_gpio_cfg.h

**Files:**
- Create: `drivers/dev_gpio/include/dev_gpio_types.h`
- Create: `drivers/dev_gpio/include/dev_gpio_cfg.h`

- [ ] **Step 1: Write drivers/dev_gpio/include/dev_gpio_types.h**

```c
#ifndef DEV_GPIO_TYPES_H
#define DEV_GPIO_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef uint16_t dev_gpio_channel_t;

typedef enum {
    DEV_GPIO_LEVEL_LOW = 0,
    DEV_GPIO_LEVEL_HIGH = 1
} dev_gpio_level_t;

typedef enum {
    DEV_GPIO_DIRECTION_INPUT = 0,
    DEV_GPIO_DIRECTION_OUTPUT,
    DEV_GPIO_DIRECTION_INPUT_OUTPUT
} dev_gpio_direction_t;

typedef enum {
    DEV_GPIO_PULL_NONE = 0,
    DEV_GPIO_PULL_UP,
    DEV_GPIO_PULL_DOWN
} dev_gpio_pull_t;

typedef enum {
    DEV_GPIO_INTR_DISABLE = 0,
    DEV_GPIO_INTR_RISING_EDGE,
    DEV_GPIO_INTR_FALLING_EDGE,
    DEV_GPIO_INTR_BOTH_EDGES,
    DEV_GPIO_INTR_LOW_LEVEL,
    DEV_GPIO_INTR_HIGH_LEVEL
} dev_gpio_intr_type_t;

typedef void (*dev_gpio_isr_callback_t)(dev_gpio_channel_t channel, void *user_arg);

typedef struct {
    dev_gpio_channel_t      channel;
    dev_gpio_direction_t    direction;
    dev_gpio_pull_t         pull;
    dev_gpio_level_t        default_level;
    dev_gpio_intr_type_t    interrupt;
    dev_gpio_isr_callback_t callback;
    void                   *callback_arg;
} dev_gpio_channel_config_t;

typedef struct {
    const dev_gpio_channel_config_t *channels;
    uint16_t                         channel_count;
} dev_gpio_config_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_TYPES_H */
```

- [ ] **Step 2: Write drivers/dev_gpio/include/dev_gpio_cfg.h**

```c
#ifndef DEV_GPIO_CFG_H
#define DEV_GPIO_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_GPIO_CFG_MAX_CHANNELS          (32U)
#define DEV_GPIO_CFG_INTERRUPT_ENABLED     (1U)
#define DEV_GPIO_CFG_VALIDATE_DUPLICATES   (1U)
#define DEV_GPIO_CFG_ENABLE_RUNTIME_CHECKS (1U)

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_CFG_H */
```

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_gpio/include/dev_gpio_types.h drivers/dev_gpio/include/dev_gpio_cfg.h
git commit -m "feat: add dev_gpio_types.h and dev_gpio_cfg.h

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 5: dev_gpio_port.h (port interface)

**Files:**
- Create: `drivers/dev_gpio/include/dev_gpio_port.h`

- [ ] **Step 1: Write drivers/dev_gpio/include/dev_gpio_port.h**

```c
#ifndef DEV_GPIO_PORT_H
#define DEV_GPIO_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "dev_gpio_cfg.h"
#include "dev_error.h"

/* ── Port-implemented functions (called by common driver) ── */

dev_err_t dev_gpio_port_init(const dev_gpio_config_t *config);

dev_err_t dev_gpio_port_deinit(void);

dev_err_t dev_gpio_port_config_channel(const dev_gpio_channel_config_t *channel_config);

dev_err_t dev_gpio_port_read(dev_gpio_channel_t channel,
                             dev_gpio_level_t *level);

dev_err_t dev_gpio_port_write(dev_gpio_channel_t channel,
                              dev_gpio_level_t level);

dev_err_t dev_gpio_port_toggle(dev_gpio_channel_t channel);

dev_err_t dev_gpio_port_set_direction(dev_gpio_channel_t channel,
                                      dev_gpio_direction_t direction);

dev_err_t dev_gpio_port_set_pull(dev_gpio_channel_t channel,
                                 dev_gpio_pull_t pull);

dev_err_t dev_gpio_port_config_interrupt(dev_gpio_channel_t channel,
                                         dev_gpio_intr_type_t interrupt);

dev_err_t dev_gpio_port_enable_interrupt(dev_gpio_channel_t channel);

dev_err_t dev_gpio_port_disable_interrupt(dev_gpio_channel_t channel);

/* ── Common-driver service (called by port ISR handlers) ── */

/* PORT-ONLY: called from vendor ISR handlers. Do not call from application code. */
void dev_gpio_dispatch_isr(dev_gpio_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_H */
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_gpio/include/dev_gpio_port.h
git commit -m "feat: add dev_gpio_port.h port interface

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 6: Mock port implementation

**Files:**
- Create: `drivers/dev_gpio/port/mock/dev_gpio_port_mock.h`
- Create: `drivers/dev_gpio/port/mock/dev_gpio_port_mock.c`

- [ ] **Step 1: Write drivers/dev_gpio/port/mock/dev_gpio_port_mock.h**

```c
#ifndef DEV_GPIO_PORT_MOCK_H
#define DEV_GPIO_PORT_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"

/* Operations that can be targeted for error injection */
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

/* Global error injection: all subsequent calls fail with this error */
void dev_gpio_port_mock_set_error(dev_err_t error);

/* Per-operation injection: only the specified operation fails */
void dev_gpio_port_mock_set_error_for_op(dev_gpio_port_mock_op_t op, dev_err_t error);

/* Fail-after-N: the Nth call to any port operation fails */
void dev_gpio_port_mock_set_fail_after(uint16_t call_count, dev_err_t error);

/* Clear all injected errors, reset call counter */
void dev_gpio_port_mock_clear_error(void);

/* ISR simulation: invoke the common ISR dispatch */
void dev_gpio_port_mock_trigger_isr(dev_gpio_channel_t channel);

/* State inspection for test assertions */
dev_gpio_level_t     dev_gpio_port_mock_get_level(dev_gpio_channel_t channel);
dev_gpio_direction_t dev_gpio_port_mock_get_direction(dev_gpio_channel_t channel);
dev_gpio_pull_t      dev_gpio_port_mock_get_pull(dev_gpio_channel_t channel);
bool                 dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_MOCK_H */
```

- [ ] **Step 2: Write drivers/dev_gpio/port/mock/dev_gpio_port_mock.c**

```c
#include "dev_gpio_port_mock.h"

/* ── Internal state (indexed by config index, NOT raw channel ID) ── */

static dev_gpio_level_t      m_levels[DEV_GPIO_CFG_MAX_CHANNELS];
static dev_gpio_direction_t  m_directions[DEV_GPIO_CFG_MAX_CHANNELS];
static dev_gpio_pull_t       m_pulls[DEV_GPIO_CFG_MAX_CHANNELS];
static dev_gpio_intr_type_t  m_interrupts[DEV_GPIO_CFG_MAX_CHANNELS];
static bool                  m_interrupt_enabled[DEV_GPIO_CFG_MAX_CHANNELS];
static uint16_t              m_channel_count;
static bool                  m_initialized;

/* ── Channel ID → config index mapping ── */

static const dev_gpio_config_t *m_config;

static uint16_t mock_find_index(dev_gpio_channel_t channel)
{
    uint16_t i;
    for (i = 0U; i < m_channel_count; i++) {
        if (m_config->channels[i].channel == channel) {
            return i;
        }
    }
    return m_channel_count; /* not found */
}

/* ── Error injection ── */

static dev_err_t m_global_error   = DEV_OK;
static dev_err_t m_op_errors[DEV_GPIO_PORT_MOCK_OP_COUNT];
static bool      m_op_error_set[DEV_GPIO_PORT_MOCK_OP_COUNT];
static uint16_t  m_fail_after_n   = 0U;
static uint16_t  m_call_counter   = 0U;
static dev_err_t m_fail_after_error = DEV_OK;
static bool      m_fail_after_active = false;

static dev_err_t mock_check_error(dev_gpio_port_mock_op_t op)
{
    m_call_counter++;

    if (m_fail_after_active && (m_call_counter == m_fail_after_n)) {
        return m_fail_after_error;
    }

    if (m_op_error_set[op]) {
        return m_op_errors[op];
    }

    if (m_global_error != DEV_OK) {
        return m_global_error;
    }

    return DEV_OK;
}

void dev_gpio_port_mock_set_error(dev_err_t error)
{
    m_global_error = error;
}

void dev_gpio_port_mock_set_error_for_op(dev_gpio_port_mock_op_t op, dev_err_t error)
{
    if (op < DEV_GPIO_PORT_MOCK_OP_COUNT) {
        m_op_errors[op]   = error;
        m_op_error_set[op] = true;
    }
}

void dev_gpio_port_mock_set_fail_after(uint16_t call_count, dev_err_t error)
{
    m_fail_after_n      = call_count;
    m_fail_after_error  = error;
    m_fail_after_active = true;
    m_call_counter      = 0U;
}

void dev_gpio_port_mock_clear_error(void)
{
    uint16_t i;
    m_global_error = DEV_OK;
    for (i = 0U; i < DEV_GPIO_PORT_MOCK_OP_COUNT; i++) {
        m_op_errors[i]    = DEV_OK;
        m_op_error_set[i] = false;
    }
    m_fail_after_active = false;
    m_fail_after_n      = 0U;
    m_call_counter      = 0U;
}

/* ── ISR simulation ── */

void dev_gpio_port_mock_trigger_isr(dev_gpio_channel_t channel)
{
    dev_gpio_dispatch_isr(channel);
}

/* ── State inspection ── */

dev_gpio_level_t dev_gpio_port_mock_get_level(dev_gpio_channel_t channel)
{
    uint16_t idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_GPIO_LEVEL_LOW;
    }
    return m_levels[idx];
}

dev_gpio_direction_t dev_gpio_port_mock_get_direction(dev_gpio_channel_t channel)
{
    uint16_t idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_GPIO_DIRECTION_INPUT;
    }
    return m_directions[idx];
}

dev_gpio_pull_t dev_gpio_port_mock_get_pull(dev_gpio_channel_t channel)
{
    uint16_t idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_GPIO_PULL_NONE;
    }
    return m_pulls[idx];
}

bool dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_channel_t channel)
{
    uint16_t idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return false;
    }
    return m_interrupt_enabled[idx];
}

/* ── Port API implementation ── */

dev_err_t dev_gpio_port_init(const dev_gpio_config_t *config)
{
    dev_err_t err;
    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_INIT);
    if (err != DEV_OK) {
        return err;
    }

    m_config   = config;
    m_initialized = true;
    return DEV_OK;
}

dev_err_t dev_gpio_port_deinit(void)
{
    dev_err_t err;
    uint16_t i;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_DEINIT);
    if (err != DEV_OK) {
        return err;
    }

    for (i = 0U; i < m_channel_count; i++) {
        m_levels[i]             = DEV_GPIO_LEVEL_LOW;
        m_directions[i]         = DEV_GPIO_DIRECTION_INPUT;
        m_pulls[i]              = DEV_GPIO_PULL_NONE;
        m_interrupts[i]         = DEV_GPIO_INTR_DISABLE;
        m_interrupt_enabled[i]  = false;
    }
    m_initialized   = false;
    m_channel_count = 0U;
    m_config        = NULL;
    return DEV_OK;
}

dev_err_t dev_gpio_port_config_channel(const dev_gpio_channel_config_t *channel_config)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_CONFIG_CHANNEL);
    if (err != DEV_OK) {
        return err;
    }

    if (channel_config == NULL) {
        return DEV_ERR_NULL_PTR;
    }

    idx = m_channel_count;
    if (idx >= DEV_GPIO_CFG_MAX_CHANNELS) {
        return DEV_ERR_OUT_OF_RANGE;
    }

    m_levels[idx]     = channel_config->default_level;
    m_directions[idx] = channel_config->direction;
    m_pulls[idx]      = channel_config->pull;
    m_interrupts[idx] = channel_config->interrupt;
    m_channel_count++;
    return DEV_OK;
}

dev_err_t dev_gpio_port_read(dev_gpio_channel_t channel,
                             dev_gpio_level_t *level)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_READ);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    *level = m_levels[idx];
    return DEV_OK;
}

dev_err_t dev_gpio_port_write(dev_gpio_channel_t channel,
                              dev_gpio_level_t level)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_WRITE);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    m_levels[idx] = level;
    return DEV_OK;
}

dev_err_t dev_gpio_port_toggle(dev_gpio_channel_t channel)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_TOGGLE);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    if (m_levels[idx] == DEV_GPIO_LEVEL_LOW) {
        m_levels[idx] = DEV_GPIO_LEVEL_HIGH;
    } else {
        m_levels[idx] = DEV_GPIO_LEVEL_LOW;
    }
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_direction(dev_gpio_channel_t channel,
                                      dev_gpio_direction_t direction)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_SET_DIRECTION);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Simulate unsupported direction for negative testing */
    if (direction == DEV_GPIO_DIRECTION_INPUT_OUTPUT) {
        return DEV_ERR_NOT_SUPPORTED;
    }

    m_directions[idx] = direction;
    return DEV_OK;
}

dev_err_t dev_gpio_port_set_pull(dev_gpio_channel_t channel,
                                 dev_gpio_pull_t pull)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_SET_PULL);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Simulate unsupported pull for negative testing */
    if (pull == DEV_GPIO_PULL_DOWN) {
        return DEV_ERR_NOT_SUPPORTED;
    }

    m_pulls[idx] = pull;
    return DEV_OK;
}

dev_err_t dev_gpio_port_config_interrupt(dev_gpio_channel_t channel,
                                         dev_gpio_intr_type_t interrupt)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_CONFIG_INTERRUPT);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Simulate unsupported both-edges and level interrupts for negative testing */
    if ((interrupt == DEV_GPIO_INTR_BOTH_EDGES) ||
        (interrupt == DEV_GPIO_INTR_LOW_LEVEL) ||
        (interrupt == DEV_GPIO_INTR_HIGH_LEVEL)) {
        return DEV_ERR_NOT_SUPPORTED;
    }

    m_interrupts[idx] = interrupt;
    return DEV_OK;
}

dev_err_t dev_gpio_port_enable_interrupt(dev_gpio_channel_t channel)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_ENABLE_INTERRUPT);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    m_interrupt_enabled[idx] = true;
    return DEV_OK;
}

dev_err_t dev_gpio_port_disable_interrupt(dev_gpio_channel_t channel)
{
    dev_err_t err;
    uint16_t idx;

    err = mock_check_error(DEV_GPIO_PORT_MOCK_OP_DISABLE_INTERRUPT);
    if (err != DEV_OK) {
        return err;
    }

    idx = mock_find_index(channel);
    if (idx >= m_channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    m_interrupt_enabled[idx] = false;
    return DEV_OK;
}
```

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_gpio/port/mock/
git commit -m "feat: add mock GPIO port with error injection and state inspection

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 7: dev_gpio.h (public API header)

**Files:**
- Create: `drivers/dev_gpio/include/dev_gpio.h`

- [ ] **Step 1: Write drivers/dev_gpio/include/dev_gpio.h**

```c
#ifndef DEV_GPIO_H
#define DEV_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "dev_gpio_cfg.h"
#include "dev_error.h"

/**
 * @brief Initialize the GPIO driver with a board-provided configuration.
 *
 * Validates all channels, initializes the port layer, and configures each channel.
 * Must be called before any other GPIO API.
 *
 * @param config Pointer to board-provided GPIO configuration.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NULL_PTR if config or config->channels is NULL.
 * @return DEV_ERR_INVALID_ARG if channel_count is zero or exceeds DEV_GPIO_CFG_MAX_CHANNELS.
 * @return DEV_ERR_ALREADY_INITIALIZED if already initialized.
 * @return DEV_ERR_CONFIG if duplicate channels are detected.
 * @return DEV_ERR_NOT_SUPPORTED if interrupts are compiled out but a channel requests interrupts.
 * @return DEV_ERR_HW_FAILURE if port initialization fails.
 *
 * @note Must be called before any other GPIO API.
 * @note Not reentrant. Call once during system startup.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_init(const dev_gpio_config_t *config);

/**
 * @brief De-initialize the GPIO driver.
 *
 * Disables all interrupts, clears callbacks, deinitializes the port,
 * and forces the module to UNINITIALIZED state regardless of port errors.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_HW_FAILURE if port deinit fails (module is still deinitialized).
 *
 * @note Must be called when GPIO is initialized.
 * @note Not reentrant.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_deinit(void);

/**
 * @brief Read the logic level of a GPIO channel.
 *
 * Reads into a local temporary; *level is written only on success.
 *
 * @param channel Logical GPIO channel ID.
 * @param level   Output pointer for the read level.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_NULL_PTR if level is NULL.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 * @return DEV_ERR_HW_FAILURE if port read fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe unless the port documents support.
 * @note *level is NOT modified on failure.
 */
dev_err_t dev_gpio_read(dev_gpio_channel_t channel,
                        dev_gpio_level_t *level);

/**
 * @brief Write a logic level to a GPIO channel.
 *
 * @param channel Logical GPIO channel ID.
 * @param level   Output level to write (LOW or HIGH).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found or level is invalid.
 * @return DEV_ERR_INVALID_STATE if channel is configured as input-only.
 * @return DEV_ERR_HW_FAILURE if port write fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe unless the port documents support.
 */
dev_err_t dev_gpio_write(dev_gpio_channel_t channel,
                         dev_gpio_level_t level);

/**
 * @brief Toggle the output level of a GPIO channel.
 *
 * @param channel Logical GPIO channel ID.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 * @return DEV_ERR_INVALID_STATE if channel is configured as input-only.
 * @return DEV_ERR_HW_FAILURE if port toggle fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe unless the port documents support.
 */
dev_err_t dev_gpio_toggle(dev_gpio_channel_t channel);

/**
 * @brief Set the direction of a GPIO channel at runtime.
 *
 * @param channel   Logical GPIO channel ID.
 * @param direction Requested direction.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found or direction is invalid.
 * @return DEV_ERR_NOT_SUPPORTED if the port does not support the requested direction.
 * @return DEV_ERR_HW_FAILURE if port fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_set_direction(dev_gpio_channel_t channel,
                                 dev_gpio_direction_t direction);

/**
 * @brief Set the pull mode of a GPIO channel at runtime.
 *
 * @param channel Logical GPIO channel ID.
 * @param pull    Requested pull mode.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found or pull is invalid.
 * @return DEV_ERR_NOT_SUPPORTED if the port does not support the requested pull mode.
 * @return DEV_ERR_HW_FAILURE if port fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_set_pull(dev_gpio_channel_t channel,
                            dev_gpio_pull_t pull);

/**
 * @brief Configure the interrupt mode for a GPIO channel.
 *
 * @param channel   Logical GPIO channel ID.
 * @param interrupt Requested interrupt mode.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found or interrupt mode is invalid.
 * @return DEV_ERR_NOT_SUPPORTED if the port does not support the requested mode.
 * @return DEV_ERR_HW_FAILURE if port fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_config_interrupt(dev_gpio_channel_t channel,
                                    dev_gpio_intr_type_t interrupt);

/**
 * @brief Register an ISR callback for a GPIO channel.
 *
 * Callback may be NULL to clear a previous registration.
 * The interrupt must be enabled separately via dev_gpio_enable_interrupt().
 *
 * @param channel   Logical GPIO channel ID.
 * @param callback  ISR callback function (may be NULL).
 * @param user_arg  User argument passed to callback (may be NULL).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_register_callback(dev_gpio_channel_t channel,
                                     dev_gpio_isr_callback_t callback,
                                     void *user_arg);

/**
 * @brief Enable interrupt for a GPIO channel.
 *
 * Marks common state enabled BEFORE calling port, so any immediate
 * hardware interrupt is not dropped. Rolls back on port failure.
 *
 * @param channel Logical GPIO channel ID.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 * @return DEV_ERR_HW_FAILURE if port enable fails (common state rolled back).
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_enable_interrupt(dev_gpio_channel_t channel);

/**
 * @brief Disable interrupt for a GPIO channel.
 *
 * Marks common state disabled BEFORE calling port. Safe to call
 * even if already disabled (idempotent).
 *
 * @param channel Logical GPIO channel ID.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_INVALID_ARG if channel is not found.
 * @return DEV_ERR_HW_FAILURE if port disable fails.
 *
 * @note Module must be initialized.
 * @note Not ISR-safe.
 */
dev_err_t dev_gpio_disable_interrupt(dev_gpio_channel_t channel);

/**
 * @brief Check if the GPIO driver is initialized.
 *
 * @return true if initialized, false otherwise.
 *
 * @note Reentrant.
 * @note ISR-safe.
 */
bool dev_gpio_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_H */
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_gpio/include/dev_gpio.h
git commit -m "feat: add dev_gpio.h public API header with Doxygen documentation

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 8: dev_gpio.c — common driver implementation

**Files:**
- Create: `drivers/dev_gpio/src/dev_gpio.c`

- [ ] **Step 1: Write drivers/dev_gpio/src/dev_gpio.c**

```c
#include "dev_gpio.h"
#include "dev_gpio_port.h"
#include "dev_common.h"
#include <stddef.h>

/* ── Module state ── */

static bool g_initialized = false;
static const dev_gpio_config_t *g_config = NULL;

/* ── Callback tables (owned by common driver, indexed by config index) ── */

static dev_gpio_isr_callback_t g_callbacks[DEV_GPIO_CFG_MAX_CHANNELS];
static void                   *g_callback_args[DEV_GPIO_CFG_MAX_CHANNELS];
static bool                    g_interrupt_enabled[DEV_GPIO_CFG_MAX_CHANNELS];

/* ── Channel ID → config index lookup ── */

static uint16_t dev_gpio_find_index(dev_gpio_channel_t channel)
{
    uint16_t i;
    if (g_config == NULL) {
        return g_config->channel_count; /* never matches */
    }
    for (i = 0U; i < g_config->channel_count; i++) {
        if (g_config->channels[i].channel == channel) {
            return i;
        }
    }
    return g_config->channel_count;
}

/* ── Enum validation helpers ── */

static bool dev_gpio_is_valid_direction(dev_gpio_direction_t direction)
{
    return (direction == DEV_GPIO_DIRECTION_INPUT)  ||
           (direction == DEV_GPIO_DIRECTION_OUTPUT) ||
           (direction == DEV_GPIO_DIRECTION_INPUT_OUTPUT);
}

static bool dev_gpio_is_valid_pull(dev_gpio_pull_t pull)
{
    return (pull == DEV_GPIO_PULL_NONE) ||
           (pull == DEV_GPIO_PULL_UP)   ||
           (pull == DEV_GPIO_PULL_DOWN);
}

static bool dev_gpio_is_valid_interrupt(dev_gpio_intr_type_t interrupt)
{
    return (interrupt == DEV_GPIO_INTR_DISABLE)      ||
           (interrupt == DEV_GPIO_INTR_RISING_EDGE)  ||
           (interrupt == DEV_GPIO_INTR_FALLING_EDGE) ||
           (interrupt == DEV_GPIO_INTR_BOTH_EDGES)   ||
           (interrupt == DEV_GPIO_INTR_LOW_LEVEL)    ||
           (interrupt == DEV_GPIO_INTR_HIGH_LEVEL);
}

static bool dev_gpio_is_valid_level(dev_gpio_level_t level)
{
    return (level == DEV_GPIO_LEVEL_LOW) || (level == DEV_GPIO_LEVEL_HIGH);
}

/* ── Duplicate detection ── */

#if (DEV_GPIO_CFG_VALIDATE_DUPLICATES == 1U)
static bool dev_gpio_has_duplicates(void)
{
    uint16_t i;
    uint16_t j;

    for (i = 0U; i < g_config->channel_count; i++) {
        for (j = i + 1U; j < g_config->channel_count; j++) {
            if (g_config->channels[i].channel == g_config->channels[j].channel) {
                return true;
            }
        }
    }
    return false;
}
#endif /* DEV_GPIO_CFG_VALIDATE_DUPLICATES */

/* ── Port error mapping ── */

static dev_err_t dev_gpio_map_port_error(dev_err_t port_err)
{
    if (port_err == DEV_OK) {
        return DEV_OK;
    }
    if (port_err == DEV_ERR_NOT_SUPPORTED) {
        return DEV_ERR_NOT_SUPPORTED;
    }
    return DEV_ERR_HW_FAILURE;
}

/* ── Public API ── */

dev_err_t dev_gpio_init(const dev_gpio_config_t *config)
{
    uint16_t i;
    dev_err_t err;

    /* Step 1: check already initialized */
    if (g_initialized) {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    /* Step 2-3: validate pointers */
    if (config == NULL) {
        return DEV_ERR_NULL_PTR;
    }
    if (config->channels == NULL) {
        return DEV_ERR_NULL_PTR;
    }

    /* Step 4-5: validate channel_count */
    if (config->channel_count == 0U) {
        return DEV_ERR_INVALID_ARG;
    }
    if (config->channel_count > DEV_GPIO_CFG_MAX_CHANNELS) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Steps 6-9: validate each channel's enum fields + interrupt compiled-out check */
    for (i = 0U; i < config->channel_count; i++) {
        if (!dev_gpio_is_valid_direction(config->channels[i].direction)) {
            return DEV_ERR_INVALID_ARG;
        }
        if (!dev_gpio_is_valid_pull(config->channels[i].pull)) {
            return DEV_ERR_INVALID_ARG;
        }
        if (!dev_gpio_is_valid_interrupt(config->channels[i].interrupt)) {
            return DEV_ERR_INVALID_ARG;
        }
#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U)
        if (config->channels[i].interrupt != DEV_GPIO_INTR_DISABLE) {
            return DEV_ERR_NOT_SUPPORTED;
        }
#endif
    }

    /* Step 10: duplicate detection */
#if (DEV_GPIO_CFG_VALIDATE_DUPLICATES == 1U)
    g_config = config; /* temporarily set so dev_gpio_has_duplicates can scan */
    if (dev_gpio_has_duplicates()) {
        g_config = NULL;
        return DEV_ERR_CONFIG;
    }
#endif

    g_config = config;

    /* Step 11: port init */
    err = dev_gpio_port_init(config);
    if (err != DEV_OK) {
        g_config = NULL;
        return dev_gpio_map_port_error(err);
    }

    /* Step 12-13: configure each channel, cleanup on failure */
    for (i = 0U; i < config->channel_count; i++) {
        err = dev_gpio_port_config_channel(&config->channels[i]);
        if (err != DEV_OK) {
            (void)dev_gpio_port_deinit();
            g_config = NULL;
            return dev_gpio_map_port_error(err);
        }
    }

    /* Step 14: initialize callback tables */
    for (i = 0U; i < config->channel_count; i++) {
        g_callbacks[i]         = config->channels[i].callback;
        g_callback_args[i]     = config->channels[i].callback_arg;
        g_interrupt_enabled[i] = false;
    }

    /* Step 15-16: success */
    g_initialized = true;
    return DEV_OK;
}

dev_err_t dev_gpio_deinit(void)
{
    uint16_t i;
    dev_err_t port_err;
    dev_err_t result = DEV_OK;

    /* Step 1 */
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Step 2: best-effort disable all interrupts */
    for (i = 0U; i < g_config->channel_count; i++) {
        port_err = dev_gpio_port_disable_interrupt(g_config->channels[i].channel);
        if (port_err != DEV_OK) {
            dev_assert_report(__FILE__, (uint32_t)__LINE__,
                              DEV_ASSERT_TYPE_ERROR, port_err);
        }
    }

    /* Step 3-4: clear callbacks and interrupt state */
    for (i = 0U; i < DEV_GPIO_CFG_MAX_CHANNELS; i++) {
        g_callbacks[i]         = NULL;
        g_callback_args[i]     = NULL;
        g_interrupt_enabled[i] = false;
    }

    /* Step 5-6-7-8: port deinit, force UNINITIALIZED */
    port_err = dev_gpio_port_deinit();
    g_config      = NULL;
    g_initialized = false;

    if (port_err != DEV_OK) {
        result = dev_gpio_map_port_error(port_err);
    }

    return result;
}

dev_err_t dev_gpio_read(dev_gpio_channel_t channel,
                        dev_gpio_level_t *level)
{
    dev_gpio_level_t temp;
    uint16_t idx;
    dev_err_t err;

    /* Step 1 */
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Step 2 */
    if (level == NULL) {
        return DEV_ERR_NULL_PTR;
    }

    /* Step 3 */
    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 4-5: read into local temp, write *level only on success */
    err = dev_gpio_port_read(channel, &temp);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    *level = temp;
    return DEV_OK;
}

dev_err_t dev_gpio_write(dev_gpio_channel_t channel,
                         dev_gpio_level_t level)
{
    uint16_t idx;
    dev_err_t err;

    /* Step 1 */
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Step 2 */
    if (!dev_gpio_is_valid_level(level)) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 3 */
    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 4 */
    if (g_config->channels[idx].direction == DEV_GPIO_DIRECTION_INPUT) {
        return DEV_ERR_INVALID_STATE;
    }

    /* Step 5-6 */
    err = dev_gpio_port_write(channel, level);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_toggle(dev_gpio_channel_t channel)
{
    uint16_t idx;
    dev_err_t err;

    /* Step 1 */
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Step 2 */
    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 3 */
    if (g_config->channels[idx].direction == DEV_GPIO_DIRECTION_INPUT) {
        return DEV_ERR_INVALID_STATE;
    }

    /* Step 4-5 */
    err = dev_gpio_port_toggle(channel);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_set_direction(dev_gpio_channel_t channel,
                                 dev_gpio_direction_t direction)
{
    uint16_t idx;
    dev_err_t err;

    /* Step 1 */
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Step 2 */
    if (!dev_gpio_is_valid_direction(direction)) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 3 */
    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 4-5-6 */
    err = dev_gpio_port_set_direction(channel, direction);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    /* Update internal direction tracking (cast away const — internal-only mutation) */
    /* The config struct is logically mutable for direction tracking at runtime.
     * We store a pointer to the original config. For mock tests, this is safe
     * because the config lives in writable static memory. */
    ((dev_gpio_channel_config_t *)&g_config->channels[idx])->direction = direction;

    return DEV_OK;
}

dev_err_t dev_gpio_set_pull(dev_gpio_channel_t channel,
                            dev_gpio_pull_t pull)
{
    uint16_t idx;
    dev_err_t err;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (!dev_gpio_is_valid_pull(pull)) {
        return DEV_ERR_INVALID_ARG;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    err = dev_gpio_port_set_pull(channel, pull);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 1U)

dev_err_t dev_gpio_config_interrupt(dev_gpio_channel_t channel,
                                    dev_gpio_intr_type_t interrupt)
{
    uint16_t idx;
    dev_err_t err;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (!dev_gpio_is_valid_interrupt(interrupt)) {
        return DEV_ERR_INVALID_ARG;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    err = dev_gpio_port_config_interrupt(channel, interrupt);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_register_callback(dev_gpio_channel_t channel,
                                     dev_gpio_isr_callback_t callback,
                                     void *user_arg)
{
    uint16_t idx;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    g_callbacks[idx]     = callback;
    g_callback_args[idx] = user_arg;

    return DEV_OK;
}

dev_err_t dev_gpio_enable_interrupt(dev_gpio_channel_t channel)
{
    uint16_t idx;
    dev_err_t err;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Mark enabled BEFORE port call to avoid dropping immediate IRQs */
    g_interrupt_enabled[idx] = true;

    err = dev_gpio_port_enable_interrupt(channel);
    if (err != DEV_OK) {
        /* Roll back on failure */
        g_interrupt_enabled[idx] = false;
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_disable_interrupt(dev_gpio_channel_t channel)
{
    uint16_t idx;
    dev_err_t err;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Mark disabled BEFORE port call */
    g_interrupt_enabled[idx] = false;

    err = dev_gpio_port_disable_interrupt(channel);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

void dev_gpio_dispatch_isr(dev_gpio_channel_t channel)
{
    uint16_t idx;
    dev_gpio_isr_callback_t cb;
    void *arg;

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return;
    }

    if (!g_interrupt_enabled[idx]) {
        return;
    }

    cb  = g_callbacks[idx];
    arg = g_callback_args[idx];

    if (cb != NULL) {
        cb(channel, arg);
    }
}

#else /* DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U */

dev_err_t dev_gpio_config_interrupt(dev_gpio_channel_t channel,
                                    dev_gpio_intr_type_t interrupt)
{
    DEV_UNUSED(channel);
    DEV_UNUSED(interrupt);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_register_callback(dev_gpio_channel_t channel,
                                     dev_gpio_isr_callback_t callback,
                                     void *user_arg)
{
    DEV_UNUSED(channel);
    DEV_UNUSED(callback);
    DEV_UNUSED(user_arg);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_enable_interrupt(dev_gpio_channel_t channel)
{
    DEV_UNUSED(channel);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_disable_interrupt(dev_gpio_channel_t channel)
{
    DEV_UNUSED(channel);
    return DEV_ERR_NOT_SUPPORTED;
}

void dev_gpio_dispatch_isr(dev_gpio_channel_t channel)
{
    DEV_UNUSED(channel);
    /* No-op when interrupts are compiled out */
}

#endif /* DEV_GPIO_CFG_INTERRUPT_ENABLED */

bool dev_gpio_is_initialized(void)
{
    return g_initialized;
}
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_gpio/src/dev_gpio.c
git commit -m "feat: add dev_gpio.c common driver with validation, port dispatch, callback tables

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 9: Board configuration (board_mock)

**Files:**
- Create: `boards/board_mock/dev_board_cfg.h`
- Create: `boards/board_mock/dev_gpio_board_cfg.h`
- Create: `boards/board_mock/dev_gpio_board_cfg.c`

- [ ] **Step 1: Write boards/board_mock/dev_board_cfg.h**

```c
#ifndef DEV_BOARD_CFG_H
#define DEV_BOARD_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Board-level feature enables for mock/testing platform */
#define DEV_BOARD_MOCK  (1U)

#ifdef __cplusplus
}
#endif

#endif /* DEV_BOARD_CFG_H */
```

- [ ] **Step 2: Write boards/board_mock/dev_gpio_board_cfg.h**

```c
#ifndef DEV_GPIO_BOARD_CFG_H
#define DEV_GPIO_BOARD_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio.h"

/* Logical channel IDs — sparse to test channel→index mapping */
#define DEV_GPIO_CHANNEL_LED_STATUS      ((dev_gpio_channel_t)0U)
#define DEV_GPIO_CHANNEL_BUTTON_USER     ((dev_gpio_channel_t)10U)

/* Extern the board GPIO config for application use */
extern const dev_gpio_config_t g_dev_gpio_config;

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_BOARD_CFG_H */
```

- [ ] **Step 3: Write boards/board_mock/dev_gpio_board_cfg.c**

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
        .callback      = NULL,
        .callback_arg  = NULL,
    },
};

const dev_gpio_config_t g_dev_gpio_config = {
    .channels      = m_channels,
    .channel_count = (uint16_t)DEV_ARRAY_SIZE(m_channels),
};
```

- [ ] **Step 4: Commit**

```bash
git add boards/board_mock/
git commit -m "feat: add board_mock configuration with sparse channel IDs

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 10: Host build + test suite (all 36 tests)

**Files:**
- Create: `tests/dev_gpio/CMakeLists.txt`
- Create: `tests/dev_gpio/test_gpio.c`

- [ ] **Step 1: Write tests/dev_gpio/CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(gpio_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(gpio_test_host
    ${CMAKE_SOURCE_DIR}/drivers/dev_common/src/dev_common.c
    ${CMAKE_SOURCE_DIR}/drivers/dev_common/src/dev_assert.c
    ${CMAKE_SOURCE_DIR}/drivers/dev_gpio/src/dev_gpio.c
    ${CMAKE_SOURCE_DIR}/drivers/dev_gpio/port/mock/dev_gpio_port_mock.c
    ${CMAKE_SOURCE_DIR}/boards/board_mock/dev_gpio_board_cfg.c
    test_gpio.c
)

target_include_directories(gpio_test_host PRIVATE
    ${CMAKE_SOURCE_DIR}/drivers/dev_common/include
    ${CMAKE_SOURCE_DIR}/drivers/dev_gpio/include
    ${CMAKE_SOURCE_DIR}/drivers/dev_gpio/port/mock
    ${CMAKE_SOURCE_DIR}/boards/board_mock
)

target_compile_options(gpio_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
```

- [ ] **Step 2: Write tests/dev_gpio/test_gpio.c**

```c
#include <stdio.h>
#include <string.h>
#include "dev_gpio.h"
#include "dev_gpio_board_cfg.h"
#include "dev_gpio_port_mock.h"
#include "dev_assert.h"

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)

#define CHECK(cond, msg) do {                              \
    if (!(cond)) {                                         \
        printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define CHECK_EQ(a, b, msg) do {                           \
    if ((a) != (b)) {                                      \
        printf("  FAIL: %s (expected %d, got %d) (%s:%d)\n", \
               msg, (int)(b), (int)(a), __FILE__, __LINE__); \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define CHECK_NEQ(a, b, msg) do {                          \
    if ((a) == (b)) {                                      \
        printf("  FAIL: %s (unexpected %d) (%s:%d)\n",    \
               msg, (int)(a), __FILE__, __LINE__);          \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define RUN_TEST(name) do {                                \
    printf("  test_%s...\n", #name);                       \
    dev_gpio_port_mock_clear_error();                       \
    test_##name();                                         \
} while (false)

/* ── Setup / teardown helpers ── */

static void setup(void)
{
    /* Deinit if previously initialized so each test starts clean */
    if (dev_gpio_is_initialized()) {
        (void)dev_gpio_deinit();
    }
    dev_gpio_port_mock_clear_error();
}

/* ── ISR callback test helper ── */

static dev_gpio_channel_t g_last_isr_channel;
static void              *g_last_isr_arg;
static int                g_isr_call_count;

static void test_isr_callback(dev_gpio_channel_t channel, void *user_arg)
{
    g_last_isr_channel = channel;
    g_last_isr_arg     = user_arg;
    g_isr_call_count++;
}

static void reset_isr_state(void)
{
    g_last_isr_channel = (dev_gpio_channel_t)0xFFFFU;
    g_last_isr_arg     = NULL;
    g_isr_call_count   = 0;
}

/* ── Test 1: init with valid config ── */

TEST(1_init_valid_config)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    CHECK(dev_gpio_is_initialized(), "should be initialized");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 2: init with NULL config ── */

TEST(2_init_null_config)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(NULL);
    CHECK_EQ(err, DEV_ERR_NULL_PTR, "null config should fail");
    CHECK(!dev_gpio_is_initialized(), "should not be initialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 3: init with NULL channels ── */

TEST(3_init_null_channels)
{
    dev_err_t err;
    dev_gpio_config_t bad_cfg = { NULL, 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_NULL_PTR, "null channels should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 4: init with channel_count=0 ── */

TEST(4_init_zero_channels)
{
    dev_err_t err;
    dev_gpio_channel_config_t ch = {0};
    dev_gpio_config_t bad_cfg = { &ch, 0U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "zero channels should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 5: init with channel_count > MAX_CHANNELS ── */

TEST(5_init_too_many_channels)
{
    dev_err_t err;
    dev_gpio_config_t bad_cfg = { NULL, DEV_GPIO_CFG_MAX_CHANNELS + 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "too many channels should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 6: init with sparse channel IDs ── */

TEST(6_init_sparse_channels)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "sparse channel init should succeed");
    /* Channels 0 and 10 should both be found by scan, not array index */
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 7: unknown channel at runtime ── */

TEST(7_unknown_channel)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_read((dev_gpio_channel_t)99U, &lvl);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "unknown channel should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 8: init with invalid direction enum ── */

TEST(8_init_invalid_direction)
{
    dev_err_t err;
    dev_gpio_channel_config_t ch = {
        .channel = 0U, .direction = (dev_gpio_direction_t)99U,
        .pull = DEV_GPIO_PULL_NONE, .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt = DEV_GPIO_INTR_DISABLE
    };
    dev_gpio_config_t bad_cfg = { &ch, 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "invalid direction should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 9: init with invalid pull enum ── */

TEST(9_init_invalid_pull)
{
    dev_err_t err;
    dev_gpio_channel_config_t ch = {
        .channel = 0U, .direction = DEV_GPIO_DIRECTION_INPUT,
        .pull = (dev_gpio_pull_t)99U, .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt = DEV_GPIO_INTR_DISABLE
    };
    dev_gpio_config_t bad_cfg = { &ch, 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "invalid pull should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 10: init with invalid interrupt enum ── */

TEST(10_init_invalid_interrupt)
{
    dev_err_t err;
    dev_gpio_channel_config_t ch = {
        .channel = 0U, .direction = DEV_GPIO_DIRECTION_INPUT,
        .pull = DEV_GPIO_PULL_NONE, .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt = (dev_gpio_intr_type_t)99U
    };
    dev_gpio_config_t bad_cfg = { &ch, 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "invalid interrupt should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 11: double init ── */

TEST(11_double_init)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "first init should succeed");
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_ERR_ALREADY_INITIALIZED, "double init should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 12: duplicate channel detection ── */

TEST(12_duplicate_channels)
{
    dev_err_t err;
    dev_gpio_channel_config_t chs[2] = {
        { .channel = 5U, .direction = DEV_GPIO_DIRECTION_OUTPUT,
          .pull = DEV_GPIO_PULL_NONE, .default_level = DEV_GPIO_LEVEL_LOW,
          .interrupt = DEV_GPIO_INTR_DISABLE },
        { .channel = 5U, .direction = DEV_GPIO_DIRECTION_INPUT,
          .pull = DEV_GPIO_PULL_UP, .default_level = DEV_GPIO_LEVEL_LOW,
          .interrupt = DEV_GPIO_INTR_DISABLE },
    };
    dev_gpio_config_t bad_cfg = { chs, 2U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_CONFIG, "duplicate channels should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 13: port init failure propagates ── */

TEST(13_port_init_failure)
{
    dev_err_t err;
    setup();
    dev_gpio_port_mock_set_error_for_op(DEV_GPIO_PORT_MOCK_OP_INIT, DEV_ERR_HW_FAILURE);
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "port init failure should propagate");
    CHECK(!dev_gpio_is_initialized(), "should remain uninitialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 14: port channel config failure + cleanup ── */

TEST(14_port_config_channel_failure)
{
    dev_err_t err;
    setup();
    dev_gpio_port_mock_set_error_for_op(DEV_GPIO_PORT_MOCK_OP_CONFIG_CHANNEL,
                                        DEV_ERR_HW_FAILURE);
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "config channel failure should propagate");
    CHECK(!dev_gpio_is_initialized(), "should remain uninitialized after cleanup");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 15: read before init ── */

TEST(15_read_before_init)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_ERR_NOT_INITIALIZED, "read before init should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 16: write before init ── */

TEST(16_write_before_init)
{
    dev_err_t err;
    setup();
    err = dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, DEV_GPIO_LEVEL_HIGH);
    CHECK_EQ(err, DEV_ERR_NOT_INITIALIZED, "write before init should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 17: read with NULL level pointer ── */

TEST(17_read_null_level)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, NULL);
    CHECK_EQ(err, DEV_ERR_NULL_PTR, "null level pointer should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 18: read from unknown channel ── */

TEST(18_read_unknown_channel)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_read((dev_gpio_channel_t)99U, &lvl);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "unknown channel read should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 19: write with invalid level ── */

TEST(19_write_invalid_level)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, (dev_gpio_level_t)99U);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "invalid level should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 20: write to input-only channel ── */

TEST(20_write_to_input)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_write(DEV_GPIO_CHANNEL_BUTTON_USER, DEV_GPIO_LEVEL_HIGH);
    CHECK_EQ(err, DEV_ERR_INVALID_STATE, "write to input should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 21: toggle input-only channel ── */

TEST(21_toggle_input)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_toggle(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_ERR_INVALID_STATE, "toggle input should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 22: read from output channel ── */

TEST(22_read_output_default)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_OK, "read output should succeed");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_LOW, "default output should be LOW");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 23: write then read output ── */

TEST(23_write_then_read)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, DEV_GPIO_LEVEL_HIGH);
    CHECK_EQ(err, DEV_OK, "write should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_OK, "read should succeed");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_HIGH, "level should be HIGH");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 24: toggle output ── */

TEST(24_toggle_output)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    /* Start LOW, toggle to HIGH */
    err = dev_gpio_toggle(DEV_GPIO_CHANNEL_LED_STATUS);
    CHECK_EQ(err, DEV_OK, "toggle should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_OK, "read should succeed");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_HIGH, "should be HIGH after toggle from LOW");
    /* Toggle back to LOW */
    err = dev_gpio_toggle(DEV_GPIO_CHANNEL_LED_STATUS);
    CHECK_EQ(err, DEV_OK, "toggle should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_OK, "read should succeed");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_LOW, "should be LOW after toggle from HIGH");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 25: set pull mode ── */

TEST(25_set_pull)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_set_pull(DEV_GPIO_CHANNEL_BUTTON_USER, DEV_GPIO_PULL_UP);
    CHECK_EQ(err, DEV_OK, "set pull should succeed");
    CHECK_EQ(dev_gpio_port_mock_get_pull(DEV_GPIO_CHANNEL_BUTTON_USER),
             DEV_GPIO_PULL_UP, "pull should be UP");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 26: register callback ── */

TEST(26_register_callback)
{
    dev_err_t err;
    setup();
    reset_isr_state();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER,
                                     test_isr_callback, (void *)0x1234U);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 27: enable interrupt, mock trigger ISR ── */

TEST(27_enable_interrupt_and_trigger)
{
    dev_err_t err;
    setup();
    reset_isr_state();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER,
                                     test_isr_callback, (void *)0xABCDU);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    err = dev_gpio_enable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_OK, "enable interrupt should succeed");
    /* Trigger ISR */
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 1, "callback should be called once");
    CHECK_EQ(g_last_isr_channel, DEV_GPIO_CHANNEL_BUTTON_USER, "correct channel");
    CHECK_EQ(g_last_isr_arg, (void *)0xABCDU, "correct user arg");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 28: disable interrupt ── */

TEST(28_disable_interrupt)
{
    dev_err_t err;
    setup();
    reset_isr_state();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER,
                                     test_isr_callback, NULL);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    err = dev_gpio_enable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_OK, "enable should succeed");
    err = dev_gpio_disable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_OK, "disable should succeed");
    /* Trigger ISR — should NOT invoke callback */
    g_isr_call_count = 0;
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "callback should NOT be called when disabled");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 29: deinit ── */

TEST(29_deinit)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_deinit();
    CHECK_EQ(err, DEV_OK, "deinit should succeed");
    CHECK(!dev_gpio_is_initialized(), "should be uninitialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 30: reinit after deinit ── */

TEST(30_reinit_after_deinit)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "first init should succeed");
    err = dev_gpio_deinit();
    CHECK_EQ(err, DEV_OK, "deinit should succeed");
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "reinit should succeed");
    CHECK(dev_gpio_is_initialized(), "should be initialized");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 31: unsupported both-edges interrupt ── */

TEST(31_unsupported_both_edges)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_config_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER,
                                    DEV_GPIO_INTR_BOTH_EDGES);
    CHECK_EQ(err, DEV_ERR_NOT_SUPPORTED, "both-edges should be unsupported");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 32: unsupported pull-down ── */

TEST(32_unsupported_pull_down)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_set_pull(DEV_GPIO_CHANNEL_BUTTON_USER, DEV_GPIO_PULL_DOWN);
    CHECK_EQ(err, DEV_ERR_NOT_SUPPORTED, "pull-down should be unsupported");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 33: error injection — port write fails ── */

TEST(33_error_injection_write)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    err = dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, DEV_GPIO_LEVEL_HIGH);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "port write failure should propagate");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 34: error injection — port read fails, *level unchanged ── */

TEST(34_error_injection_read)
{
    dev_err_t err;
    dev_gpio_level_t lvl = (dev_gpio_level_t)0xFFU;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "port read failure should propagate");
    /* lvl should be unchanged — still 0xFF (not valid LOW or HIGH) */
    CHECK((lvl != DEV_GPIO_LEVEL_LOW) && (lvl != DEV_GPIO_LEVEL_HIGH),
          "level should be unchanged on failure");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 35: enable interrupt — port fails, common rolls back ── */

TEST(35_enable_interrupt_port_fail_rollback)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER,
                                     test_isr_callback, NULL);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    /* Inject failure for enable */
    dev_gpio_port_mock_set_error_for_op(DEV_GPIO_PORT_MOCK_OP_ENABLE_INTERRUPT,
                                        DEV_ERR_HW_FAILURE);
    err = dev_gpio_enable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "enable should fail with HW_FAILURE");
    /* Trigger ISR — should NOT invoke callback because common state rolled back */
    reset_isr_state();
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "callback should NOT be called after rollback");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 36: deinit with port failure, forced UNINITIALIZED ── */

TEST(36_deinit_port_fail_forced_uninit)
{
    dev_err_t err;
    setup();
    reset_isr_state();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER,
                                     test_isr_callback, (void *)0xDEADU);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    err = dev_gpio_enable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_OK, "enable should succeed");
    /* Inject port deinit failure */
    dev_gpio_port_mock_set_error_for_op(DEV_GPIO_PORT_MOCK_OP_DEINIT,
                                        DEV_ERR_HW_FAILURE);
    err = dev_gpio_deinit();
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "deinit should return port error");
    CHECK(!dev_gpio_is_initialized(), "state must be UNINITIALIZED regardless");
    /* Callbacks should be cleared: trigger ISR via mock, no callback should fire */
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "callbacks should be cleared");
    /* Reinit should be allowed */
    dev_gpio_port_mock_clear_error();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "reinit after failed deinit should succeed");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Main ── */

int main(void)
{
    /* Init assert with NONE backend for clean test output */
    dev_assert_config_t assert_cfg = {
        .backend = DEV_ASSERT_BACKEND_NONE,
        .output_hook = NULL,
        .user_hook = NULL,
        .reset_hook = NULL,
        .text_buffer = NULL,
        .text_buffer_size = 0U
    };
    dev_assert_init(&assert_cfg);

    printf("=== GPIO Driver Test Suite ===\n\n");

    RUN_TEST(1_init_valid_config);
    RUN_TEST(2_init_null_config);
    RUN_TEST(3_init_null_channels);
    RUN_TEST(4_init_zero_channels);
    RUN_TEST(5_init_too_many_channels);
    RUN_TEST(6_init_sparse_channels);
    RUN_TEST(7_unknown_channel);
    RUN_TEST(8_init_invalid_direction);
    RUN_TEST(9_init_invalid_pull);
    RUN_TEST(10_init_invalid_interrupt);
    RUN_TEST(11_double_init);
    RUN_TEST(12_duplicate_channels);
    RUN_TEST(13_port_init_failure);
    RUN_TEST(14_port_config_channel_failure);
    RUN_TEST(15_read_before_init);
    RUN_TEST(16_write_before_init);
    RUN_TEST(17_read_null_level);
    RUN_TEST(18_read_unknown_channel);
    RUN_TEST(19_write_invalid_level);
    RUN_TEST(20_write_to_input);
    RUN_TEST(21_toggle_input);
    RUN_TEST(22_read_output_default);
    RUN_TEST(23_write_then_read);
    RUN_TEST(24_toggle_output);
    RUN_TEST(25_set_pull);
    RUN_TEST(26_register_callback);
    RUN_TEST(27_enable_interrupt_and_trigger);
    RUN_TEST(28_disable_interrupt);
    RUN_TEST(29_deinit);
    RUN_TEST(30_reinit_after_deinit);
    RUN_TEST(31_unsupported_both_edges);
    RUN_TEST(32_unsupported_pull_down);
    RUN_TEST(33_error_injection_write);
    RUN_TEST(34_error_injection_read);
    RUN_TEST(35_enable_interrupt_port_fail_rollback);
    RUN_TEST(36_deinit_port_fail_forced_uninit);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);

    return (g_failures > 0) ? 1 : 0;
}
```

- [ ] **Step 3: Commit**

```bash
git add tests/dev_gpio/
git commit -m "feat: add host CMake build and 36-test GPIO test suite

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 11: Build and verify all tests pass

- [ ] **Step 1: Configure the host build**

```bash
cd tests/dev_gpio && mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug
```

Expected: CMake configures successfully with no errors.

- [ ] **Step 2: Build**

```bash
cmake --build .
```

Expected: Compiles with zero warnings (Wall, Wextra, Werror, pedantic enabled).

- [ ] **Step 3: Run tests**

```bash
./gpio_test_host
```

Expected output:

```
=== GPIO Driver Test Suite ===
  test_1_init_valid_config... PASS
  ...
  test_36_deinit_port_fail_forced_uninit... PASS
=== Results: 36 passed, 0 failed ===
```

- [ ] **Step 4: Verify no vendor headers leak**

```bash
grep -r 'stm32\|esp_idf\|nrfx\|hal\.h' drivers/dev_common/include/ drivers/dev_gpio/include/ drivers/dev_gpio/src/ || echo "No vendor leaks found"
```

Expected: "No vendor leaks found"

- [ ] **Step 5: Commit final verification**

```bash
git add -A
git commit -m "verify: all 36 tests pass, no vendor leaks in common layer

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

## Plan Summary

| Task | Files | Description |
|------|-------|-------------|
| 1 | 2 | Directory structure, dev_types.h, dev_error.h |
| 2 | 2 | dev_compiler.h, dev_version.h |
| 3 | 4 | dev_assert.h, dev_assert.c, dev_common.h, dev_common.c |
| 4 | 2 | dev_gpio_types.h, dev_gpio_cfg.h |
| 5 | 1 | dev_gpio_port.h (port interface) |
| 6 | 2 | Mock port implementation with error injection |
| 7 | 1 | dev_gpio.h (public API header with Doxygen) |
| 8 | 1 | dev_gpio.c (full common driver) |
| 9 | 3 | Board mock config with sparse channel IDs |
| 10 | 2 | Host CMake build + 36 tests |
| 11 | — | Build, run, verify all 36 pass |
| **Total** | **20 files** | **11 tasks** |
