# dev_queue Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a lightweight generic static FIFO queue (`dev_queue`) that stores fixed-size items by value in caller-provided static storage.

**Architecture:** Single-file circular FIFO with head/tail/count tracking. No port layer needed — pure logic with no hardware interaction. Depends only on `dev_common`.

**Tech Stack:** C11, no external dependencies, host-based unit tests via CMake + custom test harness (matching existing project patterns).

## Global Constraints

- No dynamic memory allocation
- No vendor HAL headers in public API or implementation
- No RTOS headers or calls
- No recursion
- No magic numbers — all constants named via config macros or enums
- Fixed-width integer types only (`stdint.h`)
- Public APIs return `dev_err_t` where applicable
- Static helpers must be `static`
- No `goto`, no `continue`
- Single return per function preferred
- Public types use `dev_queue_` prefix, private helpers use file-scope names
- `dev_queue` depends only on `dev_common` (types, errors, assert macros)
- Follow existing code style: `DEV_CHECK_PTR_RET`, `DEV_RETURN_ON_FALSE`, early-return validation

---
```

### Task 1: Create dev_queue type and config headers

**Files:**
- Create: `drivers/dev_queue/include/dev_queue_types.h`
- Create: `drivers/dev_queue/include/dev_queue_cfg.h`

**Interfaces:**
- Produces: `dev_queue_size_t`, `dev_queue_item_size_t`, `dev_queue_t` struct
- Produces: `DEV_QUEUE_CFG_RUNTIME_CHECK_ENABLED`, `DEV_QUEUE_CFG_OVERWRITE_API_ENABLED`, `DEV_QUEUE_CFG_PEEK_API_ENABLED`, `DEV_QUEUE_CFG_MANY_API_ENABLED`, `DEV_QUEUE_CFG_CLEAR_ON_RESET_ENABLED`, `DEV_QUEUE_CFG_CRITICAL_SECTION_ENABLED`

- [ ] **Step 1: Write dev_queue_types.h**

```c
#ifndef DEV_QUEUE_TYPES_H
#define DEV_QUEUE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef uint16_t dev_queue_size_t;
typedef uint16_t dev_queue_item_size_t;

typedef struct
{
    uint8_t               *buffer;
    dev_queue_item_size_t  item_size;
    dev_queue_size_t       capacity;
    dev_queue_size_t       head;
    dev_queue_size_t       tail;
    dev_queue_size_t       count;
    bool                   initialized;
} dev_queue_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_QUEUE_TYPES_H */
```

- [ ] **Step 2: Write dev_queue_cfg.h**

```c
#ifndef DEV_QUEUE_CFG_H
#define DEV_QUEUE_CFG_H

#define DEV_QUEUE_CFG_RUNTIME_CHECK_ENABLED      (1U)
#define DEV_QUEUE_CFG_OVERWRITE_API_ENABLED      (1U)
#define DEV_QUEUE_CFG_PEEK_API_ENABLED           (1U)
#define DEV_QUEUE_CFG_MANY_API_ENABLED           (0U)
#define DEV_QUEUE_CFG_CLEAR_ON_RESET_ENABLED     (0U)
#define DEV_QUEUE_CFG_CRITICAL_SECTION_ENABLED   (0U)

#endif /* DEV_QUEUE_CFG_H */
```

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_queue/include/dev_queue_types.h drivers/dev_queue/include/dev_queue_cfg.h
git commit -m "feat(dev_queue): add type and config headers"
```

---

### Task 2: Create dev_queue public API header

**Files:**
- Create: `drivers/dev_queue/include/dev_queue.h`

**Interfaces:**
- Consumes: `dev_queue_t`, `dev_queue_size_t`, `dev_queue_item_size_t` from `dev_queue_types.h`
- Consumes: Config macros from `dev_queue_cfg.h`
- Produces: All public function declarations + `DEV_QUEUE_DEFINE` and `DEV_QUEUE_INIT` macros

- [ ] **Step 1: Write dev_queue.h**

```c
#ifndef DEV_QUEUE_H
#define DEV_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_error.h"
#include "dev_queue_types.h"
#include "dev_queue_cfg.h"

/* ── Lifecycle ── */

dev_err_t dev_queue_init(dev_queue_t *queue,
                         void *buffer,
                         dev_queue_item_size_t item_size,
                         dev_queue_size_t capacity);

dev_err_t dev_queue_deinit(dev_queue_t *queue);

dev_err_t dev_queue_reset(dev_queue_t *queue);

/* ── State queries ── */

bool dev_queue_is_initialized(const dev_queue_t *queue);

bool dev_queue_is_empty(const dev_queue_t *queue);

bool dev_queue_is_full(const dev_queue_t *queue);

dev_queue_size_t dev_queue_get_count(const dev_queue_t *queue);

dev_queue_size_t dev_queue_get_capacity(const dev_queue_t *queue);

dev_queue_size_t dev_queue_get_free_count(const dev_queue_t *queue);

dev_queue_item_size_t dev_queue_get_item_size(const dev_queue_t *queue);

/* ── Core operations ── */

/**
 * @brief Push an item into the queue.
 * @return DEV_OK, DEV_ERR_NOT_INITIALIZED, DEV_ERR_NULL_PTR, DEV_ERR_OVERFLOW
 */
dev_err_t dev_queue_push(dev_queue_t *queue, const void *item);

/**
 * @brief Pop the oldest item from the queue.
 * @return DEV_OK, DEV_ERR_NOT_INITIALIZED, DEV_ERR_NULL_PTR, DEV_ERR_EMPTY
 */
dev_err_t dev_queue_pop(dev_queue_t *queue, void *item);

#if (DEV_QUEUE_CFG_PEEK_API_ENABLED == 1U)
/**
 * @brief Read the oldest item without removing it.
 * @return DEV_OK, DEV_ERR_NOT_INITIALIZED, DEV_ERR_NULL_PTR, DEV_ERR_EMPTY
 */
dev_err_t dev_queue_peek(const dev_queue_t *queue, void *item);
#endif

/* ── Optional: overwrite ── */

#if (DEV_QUEUE_CFG_OVERWRITE_API_ENABLED == 1U)
/**
 * @brief Push an item, overwriting the oldest if full.
 * @return DEV_OK if pushed normally, DEV_ERR_OVERFLOW if overwrite occurred.
 */
dev_err_t dev_queue_push_overwrite(dev_queue_t *queue, const void *item);
#endif

/* ── Optional: bulk operations ── */

#if (DEV_QUEUE_CFG_MANY_API_ENABLED == 1U)
dev_err_t dev_queue_push_many(dev_queue_t *queue,
                              const void *items,
                              dev_queue_size_t item_count);

dev_err_t dev_queue_pop_many(dev_queue_t *queue,
                             void *items,
                             dev_queue_size_t item_count,
                             dev_queue_size_t *actual_count);
#endif

/* ── Static declaration helpers ── */

#define DEV_QUEUE_DEFINE(name, type, capacity)          \
    static type name##_storage[(capacity)];              \
    static dev_queue_t name

#define DEV_QUEUE_INIT(name, type, capacity)                 \
    dev_queue_init(&(name),                                  \
                   (void *)(name##_storage),                 \
                   (dev_queue_item_size_t)sizeof(type),      \
                   (dev_queue_size_t)(capacity))

#ifdef __cplusplus
}
#endif

#endif /* DEV_QUEUE_H */
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_queue/include/dev_queue.h
git commit -m "feat(dev_queue): add public API header with static helpers"
```

---

### Task 3: Implement dev_queue.c

**Files:**
- Create: `drivers/dev_queue/src/dev_queue.c`

**Interfaces:**
- Consumes: All types and declarations from `dev_queue.h`, `dev_queue_types.h`, `dev_queue_cfg.h`
- Consumes: `DEV_CHECK_PTR_RET`, `DEV_RETURN_ON_FALSE` from `dev_assert.h`
- Produces: Complete implementation of all public APIs

- [ ] **Step 1: Write dev_queue.c**

```c
#include "dev_queue.h"
#include "dev_assert.h"

/* ── Private helpers ── */

static dev_queue_size_t dev_queue_next_idx(dev_queue_size_t idx,
                                           dev_queue_size_t capacity)
{
    dev_queue_size_t next;
    next = idx + 1U;
    if (next >= capacity)
    {
        next = 0U;
    }
    return next;
}

static void dev_queue_copy_bytes(uint8_t *dst,
                                 const uint8_t *src,
                                 dev_queue_item_size_t length)
{
    dev_queue_item_size_t i;
    for (i = 0U; i < length; i++)
    {
        dst[i] = src[i];
    }
}

static void dev_queue_item_ptr(const dev_queue_t *queue,
                               dev_queue_size_t idx,
                               uint8_t **out_ptr)
{
    *out_ptr = &queue->buffer[(dev_queue_size_t)(idx * queue->item_size)];
}

/* ── Lifecycle ── */

dev_err_t dev_queue_init(dev_queue_t *queue,
                         void *buffer,
                         dev_queue_item_size_t item_size,
                         dev_queue_size_t capacity)
{
    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_PTR_RET(buffer);
    DEV_CHECK_RET((item_size > 0U), DEV_ERR_INVALID_ARG);
    DEV_CHECK_RET((capacity > 0U), DEV_ERR_INVALID_ARG);
    DEV_CHECK_RET((!queue->initialized), DEV_ERR_ALREADY_INITIALIZED);

    queue->buffer       = (uint8_t *)buffer;
    queue->item_size    = item_size;
    queue->capacity     = capacity;
    queue->head         = 0U;
    queue->tail         = 0U;
    queue->count        = 0U;
    queue->initialized  = true;

    return DEV_OK;
}

dev_err_t dev_queue_deinit(dev_queue_t *queue)
{
    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);

    queue->buffer       = NULL;
    queue->item_size    = 0U;
    queue->capacity     = 0U;
    queue->head         = 0U;
    queue->tail         = 0U;
    queue->count        = 0U;
    queue->initialized  = false;

    return DEV_OK;
}

dev_err_t dev_queue_reset(dev_queue_t *queue)
{
    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);

    queue->head  = 0U;
    queue->tail  = 0U;
    queue->count = 0U;

#if (DEV_QUEUE_CFG_CLEAR_ON_RESET_ENABLED == 1U)
    {
        dev_queue_size_t total_bytes;
        dev_queue_size_t i;
        total_bytes = (dev_queue_size_t)(queue->capacity * queue->item_size);
        for (i = 0U; i < total_bytes; i++)
        {
            queue->buffer[i] = 0U;
        }
    }
#endif

    return DEV_OK;
}

/* ── State queries ── */

bool dev_queue_is_initialized(const dev_queue_t *queue)
{
    bool result;
    result = false;
    if (queue != NULL)
    {
        result = queue->initialized;
    }
    return result;
}

bool dev_queue_is_empty(const dev_queue_t *queue)
{
    bool result;
    result = true;
    if ((queue != NULL) && queue->initialized)
    {
        result = (queue->count == 0U);
    }
    return result;
}

bool dev_queue_is_full(const dev_queue_t *queue)
{
    bool result;
    result = false;
    if ((queue != NULL) && queue->initialized)
    {
        result = (queue->count >= queue->capacity);
    }
    return result;
}

dev_queue_size_t dev_queue_get_count(const dev_queue_t *queue)
{
    dev_queue_size_t result;
    result = 0U;
    if ((queue != NULL) && queue->initialized)
    {
        result = queue->count;
    }
    return result;
}

dev_queue_size_t dev_queue_get_capacity(const dev_queue_t *queue)
{
    dev_queue_size_t result;
    result = 0U;
    if ((queue != NULL) && queue->initialized)
    {
        result = queue->capacity;
    }
    return result;
}

dev_queue_size_t dev_queue_get_free_count(const dev_queue_t *queue)
{
    dev_queue_size_t result;
    result = 0U;
    if ((queue != NULL) && queue->initialized)
    {
        result = (dev_queue_size_t)(queue->capacity - queue->count);
    }
    return result;
}

dev_queue_item_size_t dev_queue_get_item_size(const dev_queue_t *queue)
{
    dev_queue_item_size_t result;
    result = 0U;
    if ((queue != NULL) && queue->initialized)
    {
        result = queue->item_size;
    }
    return result;
}

/* ── Core operations ── */

dev_err_t dev_queue_push(dev_queue_t *queue, const void *item)
{
    uint8_t *dst;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(item);

    if (queue->count >= queue->capacity)
    {
        return DEV_ERR_OVERFLOW;
    }

    dev_queue_item_ptr(queue, queue->tail, &dst);
    dev_queue_copy_bytes(dst, (const uint8_t *)item, queue->item_size);
    queue->tail = dev_queue_next_idx(queue->tail, queue->capacity);
    queue->count = (dev_queue_size_t)(queue->count + 1U);

    return DEV_OK;
}

dev_err_t dev_queue_pop(dev_queue_t *queue, void *item)
{
    uint8_t *src;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(item);

    if (queue->count == 0U)
    {
        return DEV_ERR_EMPTY;
    }

    dev_queue_item_ptr(queue, queue->head, &src);
    dev_queue_copy_bytes((uint8_t *)item, src, queue->item_size);
    queue->head = dev_queue_next_idx(queue->head, queue->capacity);
    queue->count = (dev_queue_size_t)(queue->count - 1U);

    return DEV_OK;
}

#if (DEV_QUEUE_CFG_PEEK_API_ENABLED == 1U)
dev_err_t dev_queue_peek(const dev_queue_t *queue, void *item)
{
    uint8_t *src;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(item);

    if (queue->count == 0U)
    {
        return DEV_ERR_EMPTY;
    }

    dev_queue_item_ptr(queue, queue->head, &src);
    dev_queue_copy_bytes((uint8_t *)item, src, queue->item_size);

    return DEV_OK;
}
#endif

/* ── Optional: overwrite ── */

#if (DEV_QUEUE_CFG_OVERWRITE_API_ENABLED == 1U)
dev_err_t dev_queue_push_overwrite(dev_queue_t *queue, const void *item)
{
    uint8_t *dst;
    dev_err_t result;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(item);

    result = DEV_OK;

    if (queue->count >= queue->capacity)
    {
        /* Drop oldest item to make room */
        queue->head = dev_queue_next_idx(queue->head, queue->capacity);
        queue->count = (dev_queue_size_t)(queue->count - 1U);
        result = DEV_ERR_OVERFLOW;
    }

    dev_queue_item_ptr(queue, queue->tail, &dst);
    dev_queue_copy_bytes(dst, (const uint8_t *)item, queue->item_size);
    queue->tail = dev_queue_next_idx(queue->tail, queue->capacity);
    queue->count = (dev_queue_size_t)(queue->count + 1U);

    return result;
}
#endif

/* ── Optional: bulk operations ── */

#if (DEV_QUEUE_CFG_MANY_API_ENABLED == 1U)
dev_err_t dev_queue_push_many(dev_queue_t *queue,
                              const void *items,
                              dev_queue_size_t item_count)
{
    const uint8_t *src;
    dev_queue_size_t i;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(items);
    DEV_CHECK_RET((item_count > 0U), DEV_ERR_INVALID_ARG);

    if ((dev_queue_size_t)(queue->count + item_count) > queue->capacity)
    {
        return DEV_ERR_OVERFLOW;
    }

    src = (const uint8_t *)items;
    for (i = 0U; i < item_count; i++)
    {
        uint8_t *dst;
        dev_queue_item_ptr(queue, queue->tail, &dst);
        dev_queue_copy_bytes(dst, src, queue->item_size);
        src = &src[queue->item_size];
        queue->tail = dev_queue_next_idx(queue->tail, queue->capacity);
    }
    queue->count = (dev_queue_size_t)(queue->count + item_count);

    return DEV_OK;
}

dev_err_t dev_queue_pop_many(dev_queue_t *queue,
                             void *items,
                             dev_queue_size_t item_count,
                             dev_queue_size_t *actual_count)
{
    uint8_t *dst;
    dev_queue_size_t i;
    dev_queue_size_t popped;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(items);
    DEV_CHECK_PTR_RET(actual_count);
    DEV_CHECK_RET((item_count > 0U), DEV_ERR_INVALID_ARG);

    popped = 0U;
    dst = (uint8_t *)items;

    for (i = 0U; i < item_count; i++)
    {
        uint8_t *src;
        if (queue->count == 0U)
        {
            break;
        }
        dev_queue_item_ptr(queue, queue->head, &src);
        dev_queue_copy_bytes(dst, src, queue->item_size);
        dst = &dst[queue->item_size];
        queue->head = dev_queue_next_idx(queue->head, queue->capacity);
        queue->count = (dev_queue_size_t)(queue->count - 1U);
        popped = (dev_queue_size_t)(popped + 1U);
    }

    *actual_count = popped;

    if (popped == 0U)
    {
        return DEV_ERR_EMPTY;
    }

    return DEV_OK;
}
#endif
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_queue/src/dev_queue.c
git commit -m "feat(dev_queue): implement generic static FIFO queue"
```

---

### Task 4: Create CMakeLists.txt for dev_queue

**Files:**
- Create: `drivers/dev_queue/CMakeLists.txt`

**Interfaces:**
- Produces: `dev_queue` CMake library target

- [ ] **Step 1: Write CMakeLists.txt**

```cmake
add_library(dev_queue STATIC
    src/dev_queue.c
)

target_include_directories(dev_queue PUBLIC include)
target_link_libraries(dev_queue PUBLIC dev_common)
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_queue/CMakeLists.txt
git commit -m "build(dev_queue): add CMake library target"
```

---

### Task 5: Write tests and test CMakeLists

**Files:**
- Create: `tests/dev_queue/test_queue.c`
- Create: `tests/dev_queue/CMakeLists.txt`

**Interfaces:**
- Consumes: All public APIs from `dev_queue.h`

- [ ] **Step 1: Write test_queue.c**

```c
#include "dev_queue.h"
#include <stdio.h>

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)
#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        g_failures++; return; \
    } \
} while(false)

#define CHECK_EQ(a, b, msg) do { \
    int _a = (int)(a); int _b = (int)(b); \
    if (_a != _b) { \
        printf("  FAIL: %s (exp %d got %d) (%s:%d)\n", msg, _b, _a, __FILE__, __LINE__); \
        g_failures++; return; \
    } \
} while(false)

#define CHECK_ERR(e, exp, msg) do { \
    int _e = (int)(e); \
    if (_e != (int)(exp)) { \
        printf("  FAIL: %s (exp %d got %d) (%s:%d)\n", msg, (int)(exp), _e, __FILE__, __LINE__); \
        g_failures++; return; \
    } \
} while(false)

#define RUN_TEST(name) do { \
    printf("  test_%s...\n", #name); \
    test_##name(); \
} while(false)

/* ── Shared test storage ── */

#define Q_CAP (8U)
static uint8_t    g_storage[Q_CAP * sizeof(uint32_t)];
static dev_queue_t g_q;

/* ── 1-4: init validation ── */

TEST(1_init_ok)
{
    CHECK_ERR(dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP), DEV_OK, "init");
    CHECK(g_q.initialized, "initialized flag");
    CHECK_EQ(g_q.count, 0, "count zero");
    CHECK_EQ(g_q.capacity, Q_CAP, "capacity");
    printf("    PASS\n"); g_passes++;
}

TEST(2_init_null_queue)
{
    CHECK_ERR(dev_queue_init(NULL, g_storage, sizeof(uint32_t), Q_CAP), DEV_ERR_NULL_PTR, "null queue");
    printf("    PASS\n"); g_passes++;
}

TEST(3_init_null_buffer)
{
    CHECK_ERR(dev_queue_init(&g_q, NULL, sizeof(uint32_t), Q_CAP), DEV_ERR_NULL_PTR, "null buffer");
    printf("    PASS\n"); g_passes++;
}

TEST(4_init_zero_item_size)
{
    CHECK_ERR(dev_queue_init(&g_q, g_storage, 0U, Q_CAP), DEV_ERR_INVALID_ARG, "item_size=0");
    printf("    PASS\n"); g_passes++;
}

TEST(5_init_zero_capacity)
{
    CHECK_ERR(dev_queue_init(&g_q, g_storage, sizeof(uint32_t), 0U), DEV_ERR_INVALID_ARG, "capacity=0");
    printf("    PASS\n"); g_passes++;
}

TEST(6_double_init)
{
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK_ERR(dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP), DEV_ERR_ALREADY_INITIALIZED, "double init");
    printf("    PASS\n"); g_passes++;
}

/* ── 7-8: deinit ── */

TEST(7_deinit)
{
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK_ERR(dev_queue_deinit(&g_q), DEV_OK, "deinit");
    CHECK(!g_q.initialized, "!initialized after deinit");
    printf("    PASS\n"); g_passes++;
}

TEST(8_deinit_before_init)
{
    dev_queue_t q = {NULL, 0U, 0U, 0U, 0U, 0U, false};
    CHECK_ERR(dev_queue_deinit(&q), DEV_ERR_NOT_INITIALIZED, "deinit before init");
    printf("    PASS\n"); g_passes++;
}

/* ── 9-11: push/pop before init ── */

TEST(9_push_before_init)
{
    dev_queue_t q = {NULL, 0U, 0U, 0U, 0U, 0U, false};
    uint32_t v = 42U;
    CHECK_ERR(dev_queue_push(&q, &v), DEV_ERR_NOT_INITIALIZED, "push before init");
    printf("    PASS\n"); g_passes++;
}

TEST(10_pop_before_init)
{
    dev_queue_t q = {NULL, 0U, 0U, 0U, 0U, 0U, false};
    uint32_t v;
    CHECK_ERR(dev_queue_pop(&q, &v), DEV_ERR_NOT_INITIALIZED, "pop before init");
    printf("    PASS\n"); g_passes++;
}

TEST(11_null_item_push)
{
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK_ERR(dev_queue_push(&g_q, NULL), DEV_ERR_NULL_PTR, "null item push");
    printf("    PASS\n"); g_passes++;
}

/* ── 12-13: push/pop round trip ── */

TEST(12_push_pop_one)
{
    uint32_t in = 0xDEADBEEFU;
    uint32_t out = 0U;
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK_ERR(dev_queue_push(&g_q, &in), DEV_OK, "push");
    CHECK_EQ(dev_queue_get_count(&g_q), 1, "count=1");
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop");
    CHECK_EQ(out, 0xDEADBEEFU, "value preserved");
    CHECK_EQ(dev_queue_get_count(&g_q), 0, "count=0");
    printf("    PASS\n"); g_passes++;
}

TEST(13_fifo_order)
{
    uint32_t in, out;
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    in = 100U; dev_queue_push(&g_q, &in);
    in = 200U; dev_queue_push(&g_q, &in);
    in = 300U; dev_queue_push(&g_q, &in);

    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop1");
    CHECK_EQ(out, 100U, "first in");
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop2");
    CHECK_EQ(out, 200U, "second in");
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop3");
    CHECK_EQ(out, 300U, "third in");
    printf("    PASS\n"); g_passes++;
}

/* ── 14-15: full/empty edge cases ── */

TEST(14_push_when_full)
{
    uint32_t v;
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    for (v = 0U; v < Q_CAP; v++) { dev_queue_push(&g_q, &v); }
    CHECK(dev_queue_is_full(&g_q), "is_full");
    v = 99U;
    CHECK_ERR(dev_queue_push(&g_q, &v), DEV_ERR_OVERFLOW, "push full");
    printf("    PASS\n"); g_passes++;
}

TEST(15_pop_when_empty)
{
    uint32_t v;
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK(dev_queue_is_empty(&g_q), "is_empty");
    CHECK_ERR(dev_queue_pop(&g_q, &v), DEV_ERR_EMPTY, "pop empty");
    printf("    PASS\n"); g_passes++;
}

/* ── 16: peek ── */

TEST(16_peek)
{
    uint32_t in = 0xCAFEU;
    uint32_t out;
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    /* Peek empty */
    CHECK_ERR(dev_queue_peek(&g_q, &out), DEV_ERR_EMPTY, "peek empty");

    /* Peek does not remove */
    dev_queue_push(&g_q, &in);
    CHECK_ERR(dev_queue_peek(&g_q, &out), DEV_OK, "peek");
    CHECK_EQ(out, 0xCAFEU, "peek value");
    CHECK_EQ(dev_queue_get_count(&g_q), 1, "count still 1 after peek");

    /* Pop confirms item still there */
    out = 0U;
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop after peek");
    CHECK_EQ(out, 0xCAFEU, "pop value matches peek");
    printf("    PASS\n"); g_passes++;
}

/* ── 17: reset ── */

TEST(17_reset)
{
    uint32_t v;
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    v = 1U; dev_queue_push(&g_q, &v);
    v = 2U; dev_queue_push(&g_q, &v);
    CHECK_EQ(dev_queue_get_count(&g_q), 2, "count before reset");

    CHECK_ERR(dev_queue_reset(&g_q), DEV_OK, "reset");
    CHECK_EQ(dev_queue_get_count(&g_q), 0, "count after reset");
    CHECK(dev_queue_is_empty(&g_q), "empty after reset");

    /* Push/pop works after reset */
    v = 42U;
    CHECK_ERR(dev_queue_push(&g_q, &v), DEV_OK, "push after reset");
    CHECK_ERR(dev_queue_pop(&g_q, &v), DEV_OK, "pop after reset");
    CHECK_EQ(v, 42U, "value after reset");
    printf("    PASS\n"); g_passes++;
}

/* ── 18: wrap-around ── */

TEST(18_wraparound)
{
    uint32_t in, out;
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    /* Fill queue */
    for (in = 0U; in < Q_CAP; in++) { dev_queue_push(&g_q, &in); }

    /* Pop half */
    for (in = 0U; in < (Q_CAP / 2U); in++) { dev_queue_pop(&g_q, &out); }

    /* Push new values to cause wrap */
    for (in = 100U; in < 100U + (Q_CAP / 2U); in++) { dev_queue_push(&g_q, &in); }

    /* Verify FIFO order across wrap boundary */
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop after wrap");
    CHECK_EQ(out, (Q_CAP / 2U), "wrapped first val");

    for (in = 1U; in < (Q_CAP / 2U); in++) { dev_queue_pop(&g_q, &out); }

    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop new val");
    CHECK_EQ(out, 100U, "new first val");
    printf("    PASS\n"); g_passes++;
}

/* ── 19: query APIs ── */

TEST(19_query_apis)
{
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK(dev_queue_is_initialized(&g_q), "is_initialized");
    CHECK_EQ(dev_queue_get_capacity(&g_q), Q_CAP, "capacity");
    CHECK_EQ(dev_queue_get_item_size(&g_q), sizeof(uint32_t), "item_size");
    CHECK_EQ(dev_queue_get_free_count(&g_q), Q_CAP, "free=capacity");

    uint32_t v = 5U;
    dev_queue_push(&g_q, &v);
    CHECK_EQ(dev_queue_get_count(&g_q), 1, "count=1");
    CHECK_EQ(dev_queue_get_free_count(&g_q), Q_CAP - 1U, "free=cap-1");
    printf("    PASS\n"); g_passes++;
}

/* ── 20: queue of struct ── */

TEST(20_struct_queue)
{
    typedef struct { uint8_t id; uint32_t ts; } event_t;
    static event_t    evt_storage[4];
    static dev_queue_t evt_q;

    dev_queue_init(&evt_q, evt_storage, sizeof(event_t), 4U);

    event_t in  = { .id = 7U, .ts = 12345U };
    event_t out = { .id = 0U, .ts = 0U };

    CHECK_ERR(dev_queue_push(&evt_q, &in), DEV_OK, "struct push");
    CHECK_ERR(dev_queue_pop(&evt_q, &out), DEV_OK, "struct pop");
    CHECK_EQ(out.id, 7U, "struct id");
    CHECK_EQ(out.ts, 12345U, "struct ts");
    printf("    PASS\n"); g_passes++;
}

/* ── 21: queue of pointers ── */

TEST(21_pointer_queue)
{
    static void *ptr_storage[4];
    static dev_queue_t ptr_q;

    dev_queue_init(&ptr_q, ptr_storage, sizeof(void *), 4U);

    int a = 1, b = 2;
    void *in  = &a;
    void *out = NULL;

    CHECK_ERR(dev_queue_push(&ptr_q, &in), DEV_OK, "ptr push");
    CHECK_ERR(dev_queue_pop(&ptr_q, &out), DEV_OK, "ptr pop");
    CHECK_EQ((int)(out == &a), 1, "ptr matches");

    /* Push second, verify FIFO for pointers */
    in = &b;
    dev_queue_push(&ptr_q, &in);
    dev_queue_pop(&ptr_q, &out);
    CHECK_EQ((int)(out == &b), 1, "ptr2 matches");
    printf("    PASS\n"); g_passes++;
}

/* ── 22: push_overwrite ── */

TEST(22_push_overwrite)
{
    uint32_t in, out;
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    /* Fill queue */
    for (in = 10U; in < 10U + Q_CAP; in++) { dev_queue_push(&g_q, &in); }
    CHECK(dev_queue_is_full(&g_q), "full");

    /* Overwrite: oldest (10) dropped, 99 inserted */
    in = 99U;
    CHECK_ERR(dev_queue_push_overwrite(&g_q, &in), DEV_ERR_OVERFLOW, "overwrite signaled");

    /* Oldest should now be 11 (10 was dropped) */
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop after overwrite");
    CHECK_EQ(out, 11U, "oldest is now 11");

    /* Drain remaining, last should be 99 */
    while (dev_queue_get_count(&g_q) > 1U) { dev_queue_pop(&g_q, &out); }
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop last");
    CHECK_EQ(out, 99U, "new item is at tail");
    printf("    PASS\n"); g_passes++;
}

/* ── 23: static macros ── */

TEST(23_static_macros)
{
    typedef struct { uint8_t a; uint16_t b; } item_t;

    DEV_QUEUE_DEFINE(macro_q, item_t, 4U);
    CHECK_ERR(DEV_QUEUE_INIT(macro_q, item_t, 4U), DEV_OK, "macro init");

    item_t in  = { .a = 3U, .b = 500U };
    item_t out = { .a = 0U, .b = 0U };

    CHECK_ERR(dev_queue_push(&macro_q, &in), DEV_OK, "macro push");
    CHECK_ERR(dev_queue_pop(&macro_q, &out), DEV_OK, "macro pop");
    CHECK_EQ(out.a, 3U, "macro a");
    CHECK_EQ(out.b, 500U, "macro b");
    printf("    PASS\n"); g_passes++;
}

/* ── 24: reset not initialized ── */

TEST(24_reset_not_init)
{
    dev_queue_t q = {NULL, 0U, 0U, 0U, 0U, 0U, false};
    CHECK_ERR(dev_queue_reset(&q), DEV_ERR_NOT_INITIALIZED, "reset not init");
    printf("    PASS\n"); g_passes++;
}

/* ── 25: null pointer safety ── */

TEST(25_null_safety)
{
    CHECK(!dev_queue_is_initialized(NULL), "is_init(NULL)");
    CHECK(dev_queue_is_empty(NULL), "is_empty(NULL)");
    CHECK(!dev_queue_is_full(NULL), "is_full(NULL)");
    CHECK_EQ(dev_queue_get_count(NULL), 0, "count(NULL)");
    CHECK_EQ(dev_queue_get_capacity(NULL), 0, "capacity(NULL)");
    CHECK_EQ(dev_queue_get_free_count(NULL), 0, "free(NULL)");
    CHECK_EQ(dev_queue_get_item_size(NULL), 0, "item_size(NULL)");
    printf("    PASS\n"); g_passes++;
}

/* ── Main ── */

int main(void)
{
    printf("=== dev_queue Test Suite ===\n\n");

    RUN_TEST(1_init_ok);
    RUN_TEST(2_init_null_queue);
    RUN_TEST(3_init_null_buffer);
    RUN_TEST(4_init_zero_item_size);
    RUN_TEST(5_init_zero_capacity);
    RUN_TEST(6_double_init);
    RUN_TEST(7_deinit);
    RUN_TEST(8_deinit_before_init);
    RUN_TEST(9_push_before_init);
    RUN_TEST(10_pop_before_init);
    RUN_TEST(11_null_item_push);
    RUN_TEST(12_push_pop_one);
    RUN_TEST(13_fifo_order);
    RUN_TEST(14_push_when_full);
    RUN_TEST(15_pop_when_empty);
    RUN_TEST(16_peek);
    RUN_TEST(17_reset);
    RUN_TEST(18_wraparound);
    RUN_TEST(19_query_apis);
    RUN_TEST(20_struct_queue);
    RUN_TEST(21_pointer_queue);
    RUN_TEST(22_push_overwrite);
    RUN_TEST(23_static_macros);
    RUN_TEST(24_reset_not_init);
    RUN_TEST(25_null_safety);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
```

- [ ] **Step 2: Write tests/dev_queue/CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(queue_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_common ${CMAKE_BINARY_DIR}/dev_common)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_queue ${CMAKE_BINARY_DIR}/dev_queue)

add_executable(queue_test_host test_queue.c)
target_link_libraries(queue_test_host PRIVATE dev_common dev_queue)
target_compile_options(queue_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
```

- [ ] **Step 3: Build and run tests**

```bash
cd tests/dev_queue && mkdir -p build && cd build && cmake .. && make && ./queue_test_host
```

Expected: All 25 tests pass.

- [ ] **Step 4: Commit**

```bash
git add tests/dev_queue/test_queue.c tests/dev_queue/CMakeLists.txt
git commit -m "test(dev_queue): add 25 host-based unit tests"
```

---

### Task 6: Create dev_queue documentation

**Files:**
- Create: `docs/dev_queue/README.md`

- [ ] **Step 1: Write docs/dev_queue/README.md**

```markdown
# dev_queue

## Purpose

`dev_queue` is a lightweight generic static FIFO queue for embedded C projects. It stores fixed-size items by value in caller-provided static storage.

Key properties:
- No dynamic memory allocation
- No vendor HAL dependency
- No RTOS dependency
- Supports any fixed-size type (primitives, structs, enums, pointers)
- Circular FIFO with head/tail/count tracking
- Optional APIs gated by compile-time config macros

## Public APIs

### Lifecycle

| Function | Description |
|----------|-------------|
| `dev_queue_init()` | Initialize queue with user-provided buffer |
| `dev_queue_deinit()` | Deinitialize queue |
| `dev_queue_reset()` | Reset queue to empty state |

### State Queries

| Function | Description |
|----------|-------------|
| `dev_queue_is_initialized()` | Check if queue is initialized |
| `dev_queue_is_empty()` | Check if queue is empty |
| `dev_queue_is_full()` | Check if queue is full |
| `dev_queue_get_count()` | Get current item count |
| `dev_queue_get_capacity()` | Get maximum item capacity |
| `dev_queue_get_free_count()` | Get free slots remaining |
| `dev_queue_get_item_size()` | Get item size in bytes |

### Core Operations

| Function | Description |
|----------|-------------|
| `dev_queue_push()` | Push item (fails if full) |
| `dev_queue_pop()` | Pop oldest item (fails if empty) |
| `dev_queue_peek()` | Read oldest item without removing |

### Optional Operations

| Function | Config Gate | Description |
|----------|------------|-------------|
| `dev_queue_push_overwrite()` | `DEV_QUEUE_CFG_OVERWRITE_API_ENABLED` | Push, overwriting oldest if full |
| `dev_queue_push_many()` | `DEV_QUEUE_CFG_MANY_API_ENABLED` | Push multiple items atomically |
| `dev_queue_pop_many()` | `DEV_QUEUE_CFG_MANY_API_ENABLED` | Pop multiple items |

### Static Helpers

```c
DEV_QUEUE_DEFINE(name, type, capacity)  // Declare queue + storage
DEV_QUEUE_INIT(name, type, capacity)    // Initialize declared queue
```

## Configuration

| Macro | Default | Description |
|-------|---------|-------------|
| `DEV_QUEUE_CFG_RUNTIME_CHECK_ENABLED` | `1U` | Enable runtime validation |
| `DEV_QUEUE_CFG_OVERWRITE_API_ENABLED` | `1U` | Enable push_overwrite |
| `DEV_QUEUE_CFG_PEEK_API_ENABLED` | `1U` | Enable peek |
| `DEV_QUEUE_CFG_MANY_API_ENABLED` | `0U` | Enable bulk push/pop |
| `DEV_QUEUE_CFG_CLEAR_ON_RESET_ENABLED` | `0U` | Zero-fill buffer on reset |
| `DEV_QUEUE_CFG_CRITICAL_SECTION_ENABLED` | `0U` | Future: critical section hooks |

## Error Mapping

| Condition | Error Code |
|-----------|-----------|
| Null pointer argument | `DEV_ERR_NULL_PTR` |
| Zero item_size or capacity | `DEV_ERR_INVALID_ARG` |
| Queue not initialized | `DEV_ERR_NOT_INITIALIZED` |
| Double initialization | `DEV_ERR_ALREADY_INITIALIZED` |
| Push when full | `DEV_ERR_OVERFLOW` |
| Pop/peek when empty | `DEV_ERR_EMPTY` |

## Usage Example

### Queue of structs

```c
typedef struct {
    uint8_t  id;
    uint32_t timestamp;
} app_event_t;

DEV_QUEUE_DEFINE(s_event_queue, app_event_t, 8U);

void app_init(void)
{
    DEV_QUEUE_INIT(s_event_queue, app_event_t, 8U);
}

void app_send_event(uint8_t id)
{
    app_event_t event = { .id = id, .timestamp = 12345U };
    (void)dev_queue_push(&s_event_queue, &event);
}

void app_process_event(void)
{
    app_event_t event;
    if (dev_queue_pop(&s_event_queue, &event) == DEV_OK)
    {
        /* Handle event */
    }
}
```

## Safety Notes

- Not thread-safe. Caller must protect concurrent access.
- Not ISR-safe by default. Wrap calls in critical sections if needed.
- Stores items by value (byte copy). Pointers stored as pointer values only.
- No blocking behavior. All APIs return immediately.
- `dev_queue_t` must be zero-initialized or explicitly initialized before use.

## MISRA-C Notes

- No dynamic memory allocation
- No recursion
- Fixed-width integer types used throughout
- Private helpers are `static`
- No vendor or RTOS headers included
- All public APIs return `dev_err_t` where applicable

## Dependencies

- `dev_common` (types, errors, assert macros)
```

- [ ] **Step 2: Commit**

```bash
git add docs/dev_queue/README.md
git commit -m "docs(dev_queue): add component documentation"
```

---

### Task 7: Update root README.md

**Files:**
- Modify: `README.md`

**Interfaces:**
- Consumes: Existing component table format in README.md

- [ ] **Step 1: Add dev_queue row to component table**

Read `README.md` and add this row to the components table:

```markdown
| `dev_queue` | Generic static FIFO queue for fixed-size items | [docs/dev_queue/README.md](docs/dev_queue/README.md) |
```

Insert it alphabetically after `dev_log` and before `dev_ringbuf`.

- [ ] **Step 2: Build and run full test suite to verify nothing is broken**

```bash
cd tests/dev_queue/build && cmake .. && make && ./queue_test_host
```

Expected: All 25 tests pass, zero failures.

- [ ] **Step 3: Commit**

```bash
git add README.md
git commit -m "docs: add dev_queue to component table"
```

---

### Final Verification

- [ ] Run full test suite: `cd tests/dev_queue/build && ./queue_test_host` — all 25 tests pass
- [ ] All source files follow MISRA-C oriented rules (no malloc, no recursion, no vendor headers)
- [ ] `dev_queue` depends only on `dev_common`
- [ ] No magic numbers in implementation
- [ ] All public APIs return `dev_err_t` where applicable
