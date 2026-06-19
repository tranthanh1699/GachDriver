# dev_eep I2C EEPROM Driver — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement `dev_eep` — an EEPROM service layer for external I2C EEPROM devices with RAM mirror, dirty-page tracking, field-based access, CRC validation, and wear-reducing write behavior.

**Architecture:** `dev_eep` sits above `dev_i2c` and `dev_common`/`dev_crc`. It keeps a static RAM mirror of EEPROM content, marks dirty pages when data changes, and only flushes dirty pages to physical EEPROM on explicit flush/shutdown. No dynamic allocation, no vendor headers in public APIs.

**Tech Stack:** C11, `dev_i2c` (mem read/write, probe), `dev_crc` (CRC-16 compute), `dev_common` (types, errors, assert macros, weak delay hook), host-based unit tests with mock I2C.

## Global Constraints

- No dynamic memory allocation — all buffers are static arrays sized by config macros
- No recursion, no goto, no continue, no magic numbers
- No vendor HAL headers in public headers
- No vendor types exposed in public APIs
- Fixed-width integer types only (`stdint.h`)
- Use `dev_common` macros: `DEV_CHECK_RET`, `DEV_CHECK_PTR_RET`, `DEV_CHECK_OK_RET`, `DEV_RETURN_ON_FALSE`
- Use `static` for all private functions
- Page writes must not cross EEPROM page boundaries
- Write only when data changes (compare before writing)
- All public APIs return `dev_err_t`
- Follow exact component naming rules from CLAUDE.md

---

### Task 1: Add DEV_ERR_CRC to dev_err_t

**Files:**
- Modify: `drivers/dev_common/include/dev_error.h`

**Interfaces:**
- Consumes: nothing
- Produces: `DEV_ERR_CRC` enum member in `dev_err_t`

- [ ] **Step 1: Add DEV_ERR_CRC before the closing brace**

Add `DEV_ERR_CRC` after `DEV_ERR_PARSE`:

```c
    DEV_ERR_PARSE,
    DEV_ERR_CRC
} dev_err_t;
```

- [ ] **Step 2: Verify build**

Run: `cd build && cmake .. && make -j4`
Expected: compiles clean, no regressions.

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_common/include/dev_error.h
git commit -m "feat: add DEV_ERR_CRC to dev_err_t for EEPROM CRC validation"
```

---

### Task 2: Add weak dev_delay_ms hook to dev_common

**Files:**
- Modify: `drivers/dev_common/include/dev_common.h`
- Modify: `drivers/dev_common/src/dev_common.c`

**Interfaces:**
- Consumes: nothing
- Produces: `void dev_delay_ms(uint32_t ms)` — weak no-op, user overrides

- [ ] **Step 1: Add declaration to dev_common.h**

After the existing includes, add:

```c
/**
 * @brief Blocking delay in milliseconds.
 *
 * Default implementation is a weak no-op. The application or board layer
 * shall provide a real implementation (e.g., HAL_Delay, vTaskDelay).
 *
 * @param ms Milliseconds to delay.
 */
void dev_delay_ms(uint32_t ms);
```

- [ ] **Step 2: Add weak default to dev_common.c**

Add to `dev_common.c`:

```c
DEV_WEAK void dev_delay_ms(uint32_t ms)
{
    (void)ms;
    /* Default: no-op. User must provide real implementation. */
}
```

- [ ] **Step 3: Verify build**

Run: `cd build && cmake .. && make -j4`
Expected: compiles clean.

- [ ] **Step 4: Commit**

```bash
git add drivers/dev_common/include/dev_common.h drivers/dev_common/src/dev_common.c
git commit -m "feat: add weak dev_delay_ms hook to dev_common"
```

---

### Task 3: Create dev_eep_types.h

**Files:**
- Create: `drivers/dev_eep/include/dev_eep_types.h`

**Interfaces:**
- Consumes: `dev_i2c_types.h` (for `dev_i2c_bus_t`, `dev_i2c_addr_t`)
- Produces: `dev_eep_id_t`, `dev_eep_addr_t`, `dev_eep_size_t`, `dev_eep_field_id_t`, `dev_eep_mem_addr_size_t`, `dev_eep_field_t`, `dev_eep_device_t`

- [ ] **Step 1: Write dev_eep_types.h**

```c
#ifndef DEV_EEP_TYPES_H
#define DEV_EEP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_i2c_types.h"

typedef uint8_t  dev_eep_id_t;
typedef uint32_t dev_eep_addr_t;
typedef uint32_t dev_eep_size_t;
typedef uint16_t dev_eep_field_id_t;

typedef enum
{
    DEV_EEP_MEM_ADDR_SIZE_8BIT = 0,
    DEV_EEP_MEM_ADDR_SIZE_16BIT,
    DEV_EEP_MEM_ADDR_SIZE_24BIT,
    DEV_EEP_MEM_ADDR_SIZE_32BIT
} dev_eep_mem_addr_size_t;

typedef struct
{
    dev_eep_field_id_t field_id;
    dev_eep_addr_t     offset;
    dev_eep_size_t     size;
    const char        *name;
} dev_eep_field_t;

typedef struct
{
    dev_eep_id_t            eep_id;
    dev_i2c_bus_t           i2c_bus;
    dev_i2c_addr_t          i2c_addr;
    dev_eep_size_t          total_size;
    dev_eep_size_t          page_size;
    dev_eep_mem_addr_size_t mem_addr_size;
    uint32_t                write_cycle_time_ms;
    uint8_t                *mirror;
    dev_eep_size_t          mirror_size;
    uint8_t                *dirty_map;
    dev_eep_size_t          dirty_map_size;
} dev_eep_device_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_TYPES_H */
```

- [ ] **Step 2: Verify build (header-only, no source yet)**

Run: `cd build && cmake .. && make -j4`
Expected: should compile (no .c file linking yet).

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/include/dev_eep_types.h
git commit -m "feat: add dev_eep_types.h — EEPROM types and device descriptor"
```

---

### Task 4: Create dev_eep_cfg.h

**Files:**
- Create: `drivers/dev_eep/include/dev_eep_cfg.h`

**Interfaces:**
- Consumes: `dev_eep_types.h`
- Produces: compile-time configuration macros for dev_eep

- [ ] **Step 1: Write dev_eep_cfg.h**

```c
#ifndef DEV_EEP_CFG_H
#define DEV_EEP_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_eep_types.h"
#include "dev_compiler.h"
#include "dev_i2c_cfg.h"

/* ── Feature toggles ── */
#define DEV_EEP_CFG_MAX_DEVICES                    (1U)
#define DEV_EEP_CFG_RUNTIME_CHECK_ENABLED          DEV_ON
#define DEV_EEP_CFG_MIRROR_ENABLED                 DEV_ON
#define DEV_EEP_CFG_DIRTY_TRACKING_ENABLED         DEV_ON
#define DEV_EEP_CFG_CRC_ENABLED                    DEV_ON
#define DEV_EEP_CFG_AUTO_READ_ALL_ON_INIT          DEV_ON
#define DEV_EEP_CFG_AUTO_FLUSH_ON_SHUTDOWN         DEV_ON
#define DEV_EEP_CFG_WRITE_ONLY_IF_CHANGED          DEV_ON
#define DEV_EEP_CFG_PAGE_WRITE_ENABLED             DEV_ON
#define DEV_EEP_CFG_ACK_POLLING_ENABLED            DEV_ON
#define DEV_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC   DEV_ON

/* ── Timing ── */
#define DEV_EEP_CFG_DEFAULT_WRITE_CYCLE_TIME_MS    (5U)
#define DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS            (10U)
#define DEV_EEP_CFG_ACK_POLL_INTERVAL_US           (100U)

/* ── Device IDs ── */
#define DEV_EEP_MAIN                               ((dev_eep_id_t)0U)

/* ── EEPROM dimensions ── */
#define DEV_EEP_MAIN_TOTAL_SIZE                    (1024U)
#define DEV_EEP_MAIN_PAGE_SIZE                     (16U)
#define DEV_EEP_MAIN_PAGE_COUNT                    (DEV_EEP_MAIN_TOTAL_SIZE / DEV_EEP_MAIN_PAGE_SIZE)
#define DEV_EEP_MAIN_DIRTY_MAP_SIZE                ((DEV_EEP_MAIN_PAGE_COUNT + 7U) / 8U)

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_CFG_H */
```

- [ ] **Step 2: Build check**

Run: `cd build && cmake .. && make -j4`
Expected: compiles.

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/include/dev_eep_cfg.h
git commit -m "feat: add dev_eep_cfg.h — compile-time configuration"
```

---

### Task 5: Create dev_eep_layout.h

**Files:**
- Create: `drivers/dev_eep/include/dev_eep_layout.h`

**Interfaces:**
- Consumes: `dev_eep_types.h`
- Produces: field ID macros, offset/size macros, magic/version constants, extern field table

- [ ] **Step 1: Write dev_eep_layout.h**

```c
#ifndef DEV_EEP_LAYOUT_H
#define DEV_EEP_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_eep_types.h"

/* ── Field IDs ── */
#define DEV_EEP_FIELD_MAGIC                 ((dev_eep_field_id_t)0U)
#define DEV_EEP_FIELD_VERSION               ((dev_eep_field_id_t)1U)
#define DEV_EEP_FIELD_CRC                   ((dev_eep_field_id_t)2U)
#define DEV_EEP_FIELD_BOOT_COUNT            ((dev_eep_field_id_t)3U)
#define DEV_EEP_FIELD_DEVICE_NAME           ((dev_eep_field_id_t)4U)

/* ── Field offsets and sizes ── */
#define DEV_EEP_LAYOUT_MAGIC_OFFSET         ((dev_eep_addr_t)0x0000U)
#define DEV_EEP_LAYOUT_MAGIC_SIZE           ((dev_eep_size_t)4U)

#define DEV_EEP_LAYOUT_VERSION_OFFSET       ((dev_eep_addr_t)0x0004U)
#define DEV_EEP_LAYOUT_VERSION_SIZE         ((dev_eep_size_t)2U)

#define DEV_EEP_LAYOUT_CRC_OFFSET           ((dev_eep_addr_t)0x0006U)
#define DEV_EEP_LAYOUT_CRC_SIZE             ((dev_eep_size_t)2U)

#define DEV_EEP_LAYOUT_BOOT_COUNT_OFFSET    ((dev_eep_addr_t)0x0008U)
#define DEV_EEP_LAYOUT_BOOT_COUNT_SIZE      ((dev_eep_size_t)4U)

#define DEV_EEP_LAYOUT_DEVICE_NAME_OFFSET   ((dev_eep_addr_t)0x000CU)
#define DEV_EEP_LAYOUT_DEVICE_NAME_SIZE     ((dev_eep_size_t)32U)

/* ── Data integrity constants ── */
#define DEV_EEP_MAGIC_VALUE                 (0x44564550UL)
#define DEV_EEP_LAYOUT_VERSION              ((uint16_t)1U)

/* ── CRC region (all data except CRC field itself) ── */
#define DEV_EEP_CRC_START_OFFSET            ((dev_eep_addr_t)0x0000U)
#define DEV_EEP_CRC_DATA_LENGTH             (DEV_EEP_LAYOUT_CRC_OFFSET)

/* ── Field table (defined in dev_eep_layout.c) ── */
extern const dev_eep_field_t  g_dev_eep_fields[];
extern const uint16_t         g_dev_eep_field_count;

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_LAYOUT_H */
```

- [ ] **Step 2: Build check**

Run: `cd build && cmake .. && make -j4`
Expected: compiles.

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/include/dev_eep_layout.h
git commit -m "feat: add dev_eep_layout.h — EEPROM field layout definitions"
```

---

### Task 6: Create dev_eep_layout.c — field table

**Files:**
- Create: `drivers/dev_eep/src/dev_eep_layout.c`

**Interfaces:**
- Consumes: `dev_eep_layout.h`
- Produces: `g_dev_eep_fields[]`, `g_dev_eep_field_count`

- [ ] **Step 1: Write dev_eep_layout.c**

```c
#include "dev_eep_layout.h"

const dev_eep_field_t g_dev_eep_fields[] =
{
    {
        DEV_EEP_FIELD_MAGIC,
        DEV_EEP_LAYOUT_MAGIC_OFFSET,
        DEV_EEP_LAYOUT_MAGIC_SIZE,
        "magic"
    },
    {
        DEV_EEP_FIELD_VERSION,
        DEV_EEP_LAYOUT_VERSION_OFFSET,
        DEV_EEP_LAYOUT_VERSION_SIZE,
        "version"
    },
    {
        DEV_EEP_FIELD_CRC,
        DEV_EEP_LAYOUT_CRC_OFFSET,
        DEV_EEP_LAYOUT_CRC_SIZE,
        "crc"
    },
    {
        DEV_EEP_FIELD_BOOT_COUNT,
        DEV_EEP_LAYOUT_BOOT_COUNT_OFFSET,
        DEV_EEP_LAYOUT_BOOT_COUNT_SIZE,
        "boot_count"
    },
    {
        DEV_EEP_FIELD_DEVICE_NAME,
        DEV_EEP_LAYOUT_DEVICE_NAME_OFFSET,
        DEV_EEP_LAYOUT_DEVICE_NAME_SIZE,
        "device_name"
    },
};

const uint16_t g_dev_eep_field_count =
    (uint16_t)(sizeof(g_dev_eep_fields) / sizeof(g_dev_eep_fields[0]));
```

- [ ] **Step 2: Build check**

Run: `cd build && cmake .. && make -j4`
Expected: compiles (need to add to CMake first, or just verify with `gcc -c`).

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/src/dev_eep_layout.c
git commit -m "feat: add dev_eep_layout.c — EEPROM field descriptor table"
```

---

### Task 7: Create dev_eep.h — public API header

**Files:**
- Create: `drivers/dev_eep/include/dev_eep.h`

**Interfaces:**
- Consumes: `dev_eep_types.h`, `dev_eep_cfg.h`, `dev_eep_layout.h`
- Produces: all public `dev_eep_*` function declarations

- [ ] **Step 1: Write dev_eep.h**

```c
#ifndef DEV_EEP_H
#define DEV_EEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_eep_types.h"
#include "dev_eep_cfg.h"
#include "dev_eep_layout.h"
#include "dev_error.h"

/* ── Lifecycle ── */

dev_err_t dev_eep_init(void);
dev_err_t dev_eep_shutdown(void);
dev_err_t dev_eep_deinit(void);
bool      dev_eep_is_initialized(void);

/* ── Raw read/write (RAM mirror) ── */

dev_err_t dev_eep_read(dev_eep_id_t eep_id,
                       dev_eep_addr_t addr,
                       uint8_t *data,
                       dev_eep_size_t length);

dev_err_t dev_eep_write(dev_eep_id_t eep_id,
                        dev_eep_addr_t addr,
                        const uint8_t *data,
                        dev_eep_size_t length);

dev_err_t dev_eep_read_all(dev_eep_id_t eep_id);

dev_err_t dev_eep_write_all(dev_eep_id_t eep_id);

dev_err_t dev_eep_flush(dev_eep_id_t eep_id);

/* ── Field-based read/write ── */

dev_err_t dev_eep_read_field(dev_eep_field_id_t field_id,
                             void *data,
                             dev_eep_size_t length);

dev_err_t dev_eep_write_field(dev_eep_field_id_t field_id,
                              const void *data,
                              dev_eep_size_t length);

dev_err_t dev_eep_get_field_info(dev_eep_field_id_t field_id,
                                 const dev_eep_field_t **field);

/* ── Typed read/write ── */

dev_err_t dev_eep_read_u8(dev_eep_field_id_t field_id, uint8_t *value);
dev_err_t dev_eep_write_u8(dev_eep_field_id_t field_id, uint8_t value);

dev_err_t dev_eep_read_u16(dev_eep_field_id_t field_id, uint16_t *value);
dev_err_t dev_eep_write_u16(dev_eep_field_id_t field_id, uint16_t value);

dev_err_t dev_eep_read_u32(dev_eep_field_id_t field_id, uint32_t *value);
dev_err_t dev_eep_write_u32(dev_eep_field_id_t field_id, uint32_t value);

/* ── Dirty state ── */

bool      dev_eep_is_dirty(dev_eep_id_t eep_id);
dev_err_t dev_eep_mark_dirty(dev_eep_id_t eep_id,
                             dev_eep_addr_t addr,
                             dev_eep_size_t length);
dev_err_t dev_eep_clear_dirty(dev_eep_id_t eep_id);
uint16_t  dev_eep_get_dirty_page_count(dev_eep_id_t eep_id);

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_H */
```

- [ ] **Step 2: Build check**

Run: `cd build && cmake .. && make -j4`
Expected: compiles (header-only).

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/include/dev_eep.h
git commit -m "feat: add dev_eep.h — public API header"
```

---

### Task 8: Create dev_eep.c — skeleton, static data, and internal helpers

**Files:**
- Create: `drivers/dev_eep/src/dev_eep.c`

**Interfaces:**
- Consumes: `dev_eep.h`, `dev_i2c.h`, `dev_crc.h`, `dev_assert.h`, `dev_common.h`
- Produces: static device table, mirror, dirty map, internal helpers

- [ ] **Step 1: Write the skeleton with static data and internal helpers**

```c
#include "dev_eep.h"
#include "dev_i2c.h"
#include "dev_crc.h"
#include "dev_assert.h"
#include "dev_common.h"
#include <string.h>

/* ── Static RAM buffers ── */

static uint8_t s_eep_main_mirror[DEV_EEP_MAIN_TOTAL_SIZE];
static uint8_t s_eep_main_dirty_map[DEV_EEP_MAIN_DIRTY_MAP_SIZE];

/* ── Device table ── */

static const dev_eep_device_t s_devices[DEV_EEP_CFG_MAX_DEVICES] =
{
    {
        DEV_EEP_MAIN,
        DEV_I2C_BUS_EEPROM,
        ((dev_i2c_addr_t)0x50U),
        DEV_EEP_MAIN_TOTAL_SIZE,
        DEV_EEP_MAIN_PAGE_SIZE,
        DEV_EEP_MEM_ADDR_SIZE_16BIT,
        DEV_EEP_CFG_DEFAULT_WRITE_CYCLE_TIME_MS,
        s_eep_main_mirror,
        DEV_EEP_MAIN_TOTAL_SIZE,
        s_eep_main_dirty_map,
        DEV_EEP_MAIN_DIRTY_MAP_SIZE
    },
};

/* ── Internal state ── */

static bool s_initialized = false;

/* ── Internal helpers: forward declarations ── */

static const dev_eep_device_t *dev_eep_find_device(dev_eep_id_t eep_id);
static const dev_eep_field_t  *dev_eep_find_field(dev_eep_field_id_t field_id);
static dev_err_t dev_eep_validate_addr(const dev_eep_device_t *dev,
                                       dev_eep_addr_t addr,
                                       dev_eep_size_t length);
static dev_err_t dev_eep_validate_layout(void);
static bool dev_eep_is_page_dirty(const dev_eep_device_t *dev,
                                  dev_eep_size_t page_index);
static void dev_eep_set_page_dirty(const dev_eep_device_t *dev,
                                   dev_eep_size_t page_index);
static void dev_eep_clear_page_dirty(const dev_eep_device_t *dev,
                                     dev_eep_size_t page_index);
static dev_eep_size_t dev_eep_addr_to_page(const dev_eep_device_t *dev,
                                           dev_eep_addr_t addr);
static dev_err_t dev_eep_i2c_read(const dev_eep_device_t *dev,
                                  dev_eep_addr_t addr,
                                  uint8_t *data,
                                  dev_eep_size_t length);
static dev_err_t dev_eep_i2c_write_page(const dev_eep_device_t *dev,
                                        dev_eep_addr_t addr,
                                        const uint8_t *data,
                                        dev_eep_size_t length);
static dev_err_t dev_eep_wait_write_cycle(const dev_eep_device_t *dev);
static dev_err_t dev_eep_update_crc(const dev_eep_device_t *dev);
static dev_err_t dev_eep_check_crc(const dev_eep_device_t *dev);
static void dev_eep_load_defaults(const dev_eep_device_t *dev);

/* ── Internal helper implementations ── */

static const dev_eep_device_t *dev_eep_find_device(dev_eep_id_t eep_id)
{
    uint8_t i;

    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        if (s_devices[i].eep_id == eep_id)
        {
            return &s_devices[i];
        }
    }
    return NULL;
}

static const dev_eep_field_t *dev_eep_find_field(dev_eep_field_id_t field_id)
{
    uint16_t i;

    for (i = 0U; i < g_dev_eep_field_count; i++)
    {
        if (g_dev_eep_fields[i].field_id == field_id)
        {
            return &g_dev_eep_fields[i];
        }
    }
    return NULL;
}

static dev_err_t dev_eep_validate_addr(const dev_eep_device_t *dev,
                                       dev_eep_addr_t addr,
                                       dev_eep_size_t length)
{
    if (dev == NULL)
    {
        return DEV_ERR_NULL_PTR;
    }
    if (length == 0U)
    {
        return DEV_ERR_INVALID_ARG;
    }
    if (addr >= dev->total_size)
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    /* Check overflow: addr + length must not wrap or exceed total_size */
    if ((addr + length) > dev->total_size)
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    if ((addr + length) < addr) /* overflow check */
    {
        return DEV_ERR_OUT_OF_RANGE;
    }
    return DEV_OK;
}

static dev_err_t dev_eep_validate_layout(void)
{
    uint16_t i;
    uint16_t j;
    const dev_eep_device_t *dev;
    dev_eep_addr_t field_end;
    dev_eep_addr_t other_end;

    dev = dev_eep_find_device(DEV_EEP_MAIN);
    if (dev == NULL)
    {
        return DEV_ERR_CONFIG;
    }

    for (i = 0U; i < g_dev_eep_field_count; i++)
    {
        const dev_eep_field_t *field = &g_dev_eep_fields[i];

        /* Check field fits in device */
        field_end = field->offset + field->size;
        if ((field_end < field->offset) || (field_end > dev->total_size))
        {
            return DEV_ERR_CONFIG; /* field out of bounds or overflow */
        }

        /* Check for overlap with other fields */
        for (j = (uint16_t)(i + 1U); j < g_dev_eep_field_count; j++)
        {
            const dev_eep_field_t *other = &g_dev_eep_fields[j];
            other_end = other->offset + other->size;

            /* Two fields overlap if neither is entirely before the other */
            if (!((field_end <= other->offset) || (other_end <= field->offset)))
            {
                return DEV_ERR_CONFIG; /* overlap detected */
            }
        }
    }

    return DEV_OK;
}

static bool dev_eep_is_page_dirty(const dev_eep_device_t *dev,
                                  dev_eep_size_t page_index)
{
    uint8_t byte_index;
    uint8_t bit_mask;

    if ((dev == NULL) || (dev->dirty_map == NULL) ||
        (page_index >= (dev->total_size / dev->page_size)))
    {
        return false;
    }

    byte_index = (uint8_t)(page_index / 8U);
    bit_mask   = (uint8_t)(1U << (page_index % 8U));

    return ((dev->dirty_map[byte_index] & bit_mask) != 0U);
}

static void dev_eep_set_page_dirty(const dev_eep_device_t *dev,
                                   dev_eep_size_t page_index)
{
    uint8_t byte_index;
    uint8_t bit_mask;

    if ((dev == NULL) || (dev->dirty_map == NULL) ||
        (page_index >= (dev->total_size / dev->page_size)))
    {
        return;
    }

    byte_index = (uint8_t)(page_index / 8U);
    bit_mask   = (uint8_t)(1U << (page_index % 8U));

    dev->dirty_map[byte_index] |= bit_mask;
}

static void dev_eep_clear_page_dirty(const dev_eep_device_t *dev,
                                     dev_eep_size_t page_index)
{
    uint8_t byte_index;
    uint8_t bit_mask;

    if ((dev == NULL) || (dev->dirty_map == NULL) ||
        (page_index >= (dev->total_size / dev->page_size)))
    {
        return;
    }

    byte_index = (uint8_t)(page_index / 8U);
    bit_mask   = (uint8_t)(~((uint8_t)(1U << (page_index % 8U))));

    dev->dirty_map[byte_index] &= bit_mask;
}

static dev_eep_size_t dev_eep_addr_to_page(const dev_eep_device_t *dev,
                                           dev_eep_addr_t addr)
{
    if ((dev == NULL) || (dev->page_size == 0U))
    {
        return 0U;
    }
    return (addr / dev->page_size);
}

/* ── Stub implementations (filled in later tasks) ── */

static dev_err_t dev_eep_i2c_read(const dev_eep_device_t *dev,
                                  dev_eep_addr_t addr,
                                  uint8_t *data,
                                  dev_eep_size_t length)
{
    /* Will be implemented in Task 9 */
    (void)dev; (void)addr; (void)data; (void)length;
    return DEV_ERR_NOT_SUPPORTED;
}

static dev_err_t dev_eep_i2c_write_page(const dev_eep_device_t *dev,
                                        dev_eep_addr_t addr,
                                        const uint8_t *data,
                                        dev_eep_size_t length)
{
    /* Will be implemented in Task 9 */
    (void)dev; (void)addr; (void)data; (void)length;
    return DEV_ERR_NOT_SUPPORTED;
}

static dev_err_t dev_eep_wait_write_cycle(const dev_eep_device_t *dev)
{
    /* Will be implemented in Task 9 */
    (void)dev;
    return DEV_ERR_NOT_SUPPORTED;
}

static dev_err_t dev_eep_update_crc(const dev_eep_device_t *dev)
{
    /* Will be implemented in Task 12 */
    (void)dev;
    return DEV_ERR_NOT_SUPPORTED;
}

static dev_err_t dev_eep_check_crc(const dev_eep_device_t *dev)
{
    /* Will be implemented in Task 12 */
    (void)dev;
    return DEV_ERR_NOT_SUPPORTED;
}

static void dev_eep_load_defaults(const dev_eep_device_t *dev)
{
    uint32_t magic;
    uint16_t version;
    uint32_t boot_count;
    uint8_t  name_buf[DEV_EEP_LAYOUT_DEVICE_NAME_SIZE];

    if (dev == NULL)
    {
        return;
    }

    /* Write magic value */
    magic = DEV_EEP_MAGIC_VALUE;
    (void)memcpy(&dev->mirror[DEV_EEP_LAYOUT_MAGIC_OFFSET],
                 &magic, DEV_EEP_LAYOUT_MAGIC_SIZE);

    /* Write version */
    version = DEV_EEP_LAYOUT_VERSION;
    (void)memcpy(&dev->mirror[DEV_EEP_LAYOUT_VERSION_OFFSET],
                 &version, DEV_EEP_LAYOUT_VERSION_SIZE);

    /* Zero boot count */
    boot_count = 0U;
    (void)memcpy(&dev->mirror[DEV_EEP_LAYOUT_BOOT_COUNT_OFFSET],
                 &boot_count, DEV_EEP_LAYOUT_BOOT_COUNT_SIZE);

    /* Clear device name */
    (void)memset(name_buf, 0, DEV_EEP_LAYOUT_DEVICE_NAME_SIZE);
    (void)memcpy(&dev->mirror[DEV_EEP_LAYOUT_DEVICE_NAME_OFFSET],
                 name_buf, DEV_EEP_LAYOUT_DEVICE_NAME_SIZE);

    /* Mark all pages dirty so defaults get written on next flush */
    {
        dev_eep_size_t page;
        dev_eep_size_t page_count = dev->total_size / dev->page_size;
        for (page = 0U; page < page_count; page++)
        {
            dev_eep_set_page_dirty(dev, page);
        }
    }
}
```

- [ ] **Step 2: Build check**

This won't compile yet (missing real implementations linked against). Just verify the header structure parses:

Run: `gcc -fsyntax-only -Idrivers/dev_eep/include -Idrivers/dev_common/include -Idrivers/dev_i2c/include drivers/dev_eep/src/dev_eep.c 2>&1 || true`
Expected: may show errors about `dev_crc.h` etc., that's OK — we'll fix in subsequent tasks.

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/src/dev_eep.c
git commit -m "feat: add dev_eep.c skeleton with static data, internal helpers"
```

---

### Task 9: Implement I2C communication and write-cycle helpers

**Files:**
- Modify: `drivers/dev_eep/src/dev_eep.c`

**Interfaces:**
- Consumes: `dev_i2c_mem_read`, `dev_i2c_mem_write`, `dev_i2c_write`, `dev_i2c_read`, `dev_i2c_probe`, `dev_delay_ms`
- Produces: real implementations of `dev_eep_i2c_read()`, `dev_eep_i2c_write_page()`, `dev_eep_wait_write_cycle()`

- [ ] **Step 1: Replace the three stub implementations with real code**

Replace `dev_eep_i2c_read`:

```c
static dev_err_t dev_eep_i2c_read(const dev_eep_device_t *dev,
                                  dev_eep_addr_t addr,
                                  uint8_t *data,
                                  dev_eep_size_t length)
{
    dev_err_t result;

    DEV_CHECK_RET((dev != NULL),  DEV_ERR_NULL_PTR);
    DEV_CHECK_RET((data != NULL), DEV_ERR_NULL_PTR);

    /* For 8/16-bit memory address sizes, use mem_read */
    if ((dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT) ||
        (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_16BIT))
    {
        dev_i2c_mem_addr_size_t i2c_addr_size;

        i2c_addr_size = (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT)
                        ? DEV_I2C_MEM_ADDR_SIZE_8BIT
                        : DEV_I2C_MEM_ADDR_SIZE_16BIT;

        result = dev_i2c_mem_read(dev->i2c_bus,
                                  dev->i2c_addr,
                                  (uint16_t)addr,
                                  i2c_addr_size,
                                  data,
                                  (uint16_t)length,
                                  DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }
    else
    {
        /* 24-bit or 32-bit: construct address bytes manually */
        uint8_t addr_bytes[4U];
        uint8_t addr_len;

        if (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_24BIT)
        {
            addr_bytes[0U] = (uint8_t)((addr >> 16U) & 0xFFU);
            addr_bytes[1U] = (uint8_t)((addr >> 8U)  & 0xFFU);
            addr_bytes[2U] = (uint8_t)(addr & 0xFFU);
            addr_len = 3U;
        }
        else /* DEV_EEP_MEM_ADDR_SIZE_32BIT */
        {
            addr_bytes[0U] = (uint8_t)((addr >> 24U) & 0xFFU);
            addr_bytes[1U] = (uint8_t)((addr >> 16U) & 0xFFU);
            addr_bytes[2U] = (uint8_t)((addr >> 8U)  & 0xFFU);
            addr_bytes[3U] = (uint8_t)(addr & 0xFFU);
            addr_len = 4U;
        }

        /* write address, then read data */
        result = dev_i2c_write_read(dev->i2c_bus,
                                    dev->i2c_addr,
                                    addr_bytes, addr_len,
                                    data, (uint16_t)length,
                                    DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }

    return result;
}
```

Replace `dev_eep_i2c_write_page`:

```c
static dev_err_t dev_eep_i2c_write_page(const dev_eep_device_t *dev,
                                        dev_eep_addr_t addr,
                                        const uint8_t *data,
                                        dev_eep_size_t length)
{
    dev_err_t result;

    DEV_CHECK_RET((dev != NULL),  DEV_ERR_NULL_PTR);
    DEV_CHECK_RET((data != NULL), DEV_ERR_NULL_PTR);
    DEV_CHECK_RET((length <= dev->page_size), DEV_ERR_OUT_OF_RANGE);

    /* For 8/16-bit memory address sizes, use mem_write */
    if ((dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT) ||
        (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_16BIT))
    {
        dev_i2c_mem_addr_size_t i2c_addr_size;

        i2c_addr_size = (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_8BIT)
                        ? DEV_I2C_MEM_ADDR_SIZE_8BIT
                        : DEV_I2C_MEM_ADDR_SIZE_16BIT;

        result = dev_i2c_mem_write(dev->i2c_bus,
                                   dev->i2c_addr,
                                   (uint16_t)addr,
                                   i2c_addr_size,
                                   data,
                                   (uint16_t)length,
                                   DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }
    else
    {
        /* 24-bit or 32-bit: buffer address + data into single write */
        uint8_t buf[128U]; /* max page size + 4 bytes address */
        uint8_t addr_len;

        if (dev->mem_addr_size == DEV_EEP_MEM_ADDR_SIZE_24BIT)
        {
            buf[0U] = (uint8_t)((addr >> 16U) & 0xFFU);
            buf[1U] = (uint8_t)((addr >> 8U)  & 0xFFU);
            buf[2U] = (uint8_t)(addr & 0xFFU);
            addr_len = 3U;
        }
        else /* DEV_EEP_MEM_ADDR_SIZE_32BIT */
        {
            buf[0U] = (uint8_t)((addr >> 24U) & 0xFFU);
            buf[1U] = (uint8_t)((addr >> 16U) & 0xFFU);
            buf[2U] = (uint8_t)((addr >> 8U)  & 0xFFU);
            buf[3U] = (uint8_t)(addr & 0xFFU);
            addr_len = 4U;
        }

        (void)memcpy(&buf[addr_len], data, (size_t)length);

        result = dev_i2c_write(dev->i2c_bus,
                               dev->i2c_addr,
                               buf,
                               (uint16_t)(addr_len + length),
                               DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS);
    }

    return result;
}
```

Replace `dev_eep_wait_write_cycle`:

```c
static dev_err_t dev_eep_wait_write_cycle(const dev_eep_device_t *dev)
{
    DEV_CHECK_RET((dev != NULL), DEV_ERR_NULL_PTR);

#if (DEV_EEP_CFG_ACK_POLLING_ENABLED == DEV_ON)
    {
        uint32_t elapsed_ms = 0U;
        dev_err_t probe_result;

        while (elapsed_ms < DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS)
        {
            probe_result = dev_i2c_probe(dev->i2c_bus,
                                         dev->i2c_addr,
                                         (dev_i2c_timeout_t)1U);
            if (probe_result == DEV_OK)
            {
                /* Device ACKed — write cycle complete */
                return DEV_OK;
            }

            /* Small delay between probes */
            dev_delay_ms(1U);
            elapsed_ms++;
        }

        return DEV_ERR_TIMEOUT;
    }
#else
    {
        /* Fallback: use write cycle time delay */
        dev_delay_ms(dev->write_cycle_time_ms);
        return DEV_OK;
    }
#endif
}
```

- [ ] **Step 2: Verify build**

At this point the file should parse correctly but may not link without the other function implementations. Syntax check only:

Run: `gcc -fsyntax-only -Wall -Idrivers/dev_eep/include -Idrivers/dev_common/include -Idrivers/dev_i2c/include -Idrivers/dev_crc/include drivers/dev_eep/src/dev_eep.c 2>&1`
Expected: may show warnings about unused functions — acceptable at this stage.

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/src/dev_eep.c
git commit -m "feat: implement I2C read/write/page and write-cycle helpers"
```

---

### Task 10: Implement lifecycle functions — init, deinit, shutdown, is_initialized

**Files:**
- Modify: `drivers/dev_eep/src/dev_eep.c`

**Interfaces:**
- Consumes: internal helpers from Tasks 8-9
- Produces: `dev_eep_init()`, `dev_eep_deinit()`, `dev_eep_shutdown()`, `dev_eep_is_initialized()`

- [ ] **Step 1: Append lifecycle implementations to dev_eep.c**

```c
/* ── Lifecycle ── */

dev_err_t dev_eep_init(void)
{
    dev_err_t result;
    uint8_t i;

    if (s_initialized)
    {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    /* Validate device configuration */
    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        const dev_eep_device_t *dev = &s_devices[i];

        if (dev->mirror == NULL)
        {
            return DEV_ERR_CONFIG;
        }
        if (dev->mirror_size != dev->total_size)
        {
            return DEV_ERR_CONFIG;
        }
        if (dev->page_size == 0U)
        {
            return DEV_ERR_CONFIG;
        }
        if (dev->dirty_map == NULL)
        {
            return DEV_ERR_CONFIG;
        }
    }

    /* Validate layout (field overlap, bounds) */
    result = dev_eep_validate_layout();
    if (result != DEV_OK)
    {
        return result;
    }

    /* Clear dirty maps and mirrors */
    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        const dev_eep_device_t *dev = &s_devices[i];
        (void)memset(dev->mirror, 0, (size_t)dev->mirror_size);
        (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);
    }

    /* Read all EEPROM data into RAM mirror */
#if (DEV_EEP_CFG_AUTO_READ_ALL_ON_INIT == DEV_ON)
    {
        for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
        {
            result = dev_eep_read_all(s_devices[i].eep_id);
            if (result != DEV_OK)
            {
                return result;
            }
        }
    }
#endif

    /* Validate CRC / magic / version, load defaults if needed */
#if (DEV_EEP_CFG_CRC_ENABLED == DEV_ON)
    {
        for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
        {
            const dev_eep_device_t *dev = &s_devices[i];
            uint32_t mirror_magic;

            /* Check magic first */
            (void)memcpy(&mirror_magic,
                         &dev->mirror[DEV_EEP_LAYOUT_MAGIC_OFFSET],
                         DEV_EEP_LAYOUT_MAGIC_SIZE);

            if (mirror_magic != DEV_EEP_MAGIC_VALUE)
            {
#if (DEV_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC == DEV_ON)
                dev_eep_load_defaults(dev);
                continue;
#else
                return DEV_ERR_CRC;
#endif
            }

            /* Check CRC */
            result = dev_eep_check_crc(dev);
            if (result != DEV_OK)
            {
#if (DEV_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC == DEV_ON)
                dev_eep_load_defaults(dev);
                continue;
#else
                return DEV_ERR_CRC;
#endif
            }
        }
    }
#endif

    s_initialized = true;
    return DEV_OK;
}

dev_err_t dev_eep_shutdown(void)
{
    uint8_t i;
    dev_err_t result;
    dev_err_t first_error = DEV_OK;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

#if (DEV_EEP_CFG_AUTO_FLUSH_ON_SHUTDOWN == DEV_ON)
    {
        for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
        {
#if (DEV_EEP_CFG_CRC_ENABLED == DEV_ON)
            /* Update CRC before flushing */
            result = dev_eep_update_crc(&s_devices[i]);
            if (result != DEV_OK)
            {
                if (first_error == DEV_OK)
                {
                    first_error = result;
                }
                continue;
            }
#endif
            result = dev_eep_flush(s_devices[i].eep_id);
            if (result != DEV_OK)
            {
                if (first_error == DEV_OK)
                {
                    first_error = result;
                }
                /* Don't clear initialized state on partial failure */
                continue;
            }
        }

        if (first_error != DEV_OK)
        {
            return first_error;
        }
    }
#endif

    s_initialized = false;
    return DEV_OK;
}

dev_err_t dev_eep_deinit(void)
{
    uint8_t i;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Clear mirrors and dirty maps */
    for (i = 0U; i < DEV_EEP_CFG_MAX_DEVICES; i++)
    {
        const dev_eep_device_t *dev = &s_devices[i];
        (void)memset(dev->mirror, 0, (size_t)dev->mirror_size);
        (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);
    }

    s_initialized = false;
    return DEV_OK;
}

bool dev_eep_is_initialized(void)
{
    return s_initialized;
}
```

- [ ] **Step 2: Verify build**

Run: `gcc -fsyntax-only -Wall -Idrivers/dev_eep/include -Idrivers/dev_common/include -Idrivers/dev_i2c/include -Idrivers/dev_crc/include drivers/dev_eep/src/dev_eep.c 2>&1`
Expected: clean compile (no errors, warnings OK for unused functions).

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/src/dev_eep.c
git commit -m "feat: implement dev_eep lifecycle — init, deinit, shutdown"
```

---

### Task 11: Implement raw read/write/flush APIs

**Files:**
- Modify: `drivers/dev_eep/src/dev_eep.c`

**Interfaces:**
- Consumes: lifecycle state, internal helpers
- Produces: `dev_eep_read()`, `dev_eep_write()`, `dev_eep_read_all()`, `dev_eep_write_all()`, `dev_eep_flush()`

- [ ] **Step 1: Append raw read/write implementations**

```c
/* ── Raw read/write (RAM mirror) ── */

dev_err_t dev_eep_read(dev_eep_id_t eep_id,
                       dev_eep_addr_t addr,
                       uint8_t *data,
                       dev_eep_size_t length)
{
    const dev_eep_device_t *dev;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    DEV_CHECK_PTR_RET(data);

    result = dev_eep_validate_addr(dev, addr, length);
    DEV_CHECK_OK_RET(result);

    /* Copy from RAM mirror to output buffer */
    (void)memcpy(data, &dev->mirror[addr], (size_t)length);

    return DEV_OK;
}

dev_err_t dev_eep_write(dev_eep_id_t eep_id,
                        dev_eep_addr_t addr,
                        const uint8_t *data,
                        dev_eep_size_t length)
{
    const dev_eep_device_t *dev;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    DEV_CHECK_PTR_RET(data);

    result = dev_eep_validate_addr(dev, addr, length);
    DEV_CHECK_OK_RET(result);

#if (DEV_EEP_CFG_WRITE_ONLY_IF_CHANGED == DEV_ON)
    {
        /* Compare: if identical, skip */
        if (memcmp(&dev->mirror[addr], data, (size_t)length) == 0)
        {
            return DEV_OK; /* No change, no dirty marking */
        }
    }
#endif

    /* Copy to mirror */
    (void)memcpy(&dev->mirror[addr], data, (size_t)length);

    /* Mark dirty pages */
    (void)dev_eep_mark_dirty(eep_id, addr, length);

    return DEV_OK;
}

dev_err_t dev_eep_read_all(dev_eep_id_t eep_id)
{
    const dev_eep_device_t *dev;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

#if (DEV_EEP_CFG_MIRROR_ENABLED == DEV_ON)
    {
        dev_err_t result;

        result = dev_eep_i2c_read(dev, 0U,
                                  dev->mirror,
                                  dev->total_size);
        if (result != DEV_OK)
        {
            return result;
        }

        /* Clear dirty map — data is fresh from EEPROM */
        (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);
    }
#endif

    return DEV_OK;
}

dev_err_t dev_eep_write_all(dev_eep_id_t eep_id)
{
    const dev_eep_device_t *dev;
    dev_eep_size_t offset;
    dev_eep_size_t remaining;
    dev_eep_size_t chunk;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    offset    = 0U;
    remaining = dev->total_size;

    while (remaining > 0U)
    {
        /* Determine chunk size: up to page_size, and don't cross page boundary */
        chunk = dev->page_size - (offset % dev->page_size);
        if (chunk > remaining)
        {
            chunk = remaining;
        }

        result = dev_eep_i2c_write_page(dev, offset,
                                        &dev->mirror[offset], chunk);
        if (result != DEV_OK)
        {
            return result;
        }

        /* Wait for write cycle */
        result = dev_eep_wait_write_cycle(dev);
        if (result != DEV_OK)
        {
            return result;
        }

        offset    += chunk;
        remaining -= chunk;
    }

#if (DEV_EEP_CFG_CRC_ENABLED == DEV_ON)
    {
        result = dev_eep_update_crc(dev);
        if (result != DEV_OK)
        {
            return result;
        }

        /* Write CRC field to EEPROM */
        {
            dev_eep_size_t crc_chunk;
            dev_eep_addr_t crc_addr = DEV_EEP_LAYOUT_CRC_OFFSET;

            crc_chunk = dev->page_size - (crc_addr % dev->page_size);
            if (crc_chunk > DEV_EEP_LAYOUT_CRC_SIZE)
            {
                crc_chunk = DEV_EEP_LAYOUT_CRC_SIZE;
            }

            result = dev_eep_i2c_write_page(dev, crc_addr,
                                            &dev->mirror[crc_addr], crc_chunk);
            if (result != DEV_OK)
            {
                return result;
            }

            result = dev_eep_wait_write_cycle(dev);
            if (result != DEV_OK)
            {
                return result;
            }
        }
    }
#endif

    /* Clear dirty map — all data written */
    (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);

    return DEV_OK;
}

dev_err_t dev_eep_flush(dev_eep_id_t eep_id)
{
    const dev_eep_device_t *dev;
    dev_eep_size_t page_index;
    dev_eep_size_t page_count;
    dev_eep_addr_t page_addr;
    dev_eep_size_t chunk_size;
    dev_err_t result;
    bool any_dirty = false;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    page_count = dev->total_size / dev->page_size;

    for (page_index = 0U; page_index < page_count; page_index++)
    {
        if (!dev_eep_is_page_dirty(dev, page_index))
        {
            continue;
        }

        any_dirty = true;
        page_addr = page_index * dev->page_size;

        /* Determine chunk size for this page */
        chunk_size = dev->page_size;
        if ((page_addr + chunk_size) > dev->total_size)
        {
            chunk_size = dev->total_size - page_addr;
        }

        /* Write the page to EEPROM */
        result = dev_eep_i2c_write_page(dev, page_addr,
                                        &dev->mirror[page_addr], chunk_size);
        if (result != DEV_OK)
        {
            /* Dirty bit remains set on failure */
            return result;
        }

        /* Wait for write cycle */
        result = dev_eep_wait_write_cycle(dev);
        if (result != DEV_OK)
        {
            return result;
        }

        /* Clear dirty bit only after successful write */
        dev_eep_clear_page_dirty(dev, page_index);
    }

    if (!any_dirty)
    {
        /* Nothing to flush — success */
        return DEV_OK;
    }

    return DEV_OK;
}
```

- [ ] **Step 2: Verify build**

Run: `gcc -fsyntax-only -Wall -Idrivers/dev_eep/include -Idrivers/dev_common/include -Idrivers/dev_i2c/include -Idrivers/dev_crc/include drivers/dev_eep/src/dev_eep.c 2>&1`
Expected: clean (still has stub CRC functions, but they'll compile).

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/src/dev_eep.c
git commit -m "feat: implement raw read/write/read_all/write_all/flush APIs"
```

---

### Task 12: Implement CRC, field, typed, and dirty-state APIs

**Files:**
- Modify: `drivers/dev_eep/src/dev_eep.c`

**Interfaces:**
- Consumes: all prior internals
- Produces: CRC helpers, field APIs, typed APIs, dirty-state APIs

- [ ] **Step 1: Replace CRC stubs and append remaining public APIs**

Replace `dev_eep_update_crc` stub:

```c
static dev_err_t dev_eep_update_crc(const dev_eep_device_t *dev)
{
    uint16_t crc_value;
    dev_err_t result;

    DEV_CHECK_RET((dev != NULL), DEV_ERR_NULL_PTR);

    /* Compute CRC-16 over the defined CRC region (excludes CRC field itself) */
    result = dev_crc16_compute(&dev->mirror[DEV_EEP_CRC_START_OFFSET],
                               (size_t)DEV_EEP_CRC_DATA_LENGTH,
                               &crc_value);
    DEV_CHECK_OK_RET(result);

    /* Write CRC value into mirror at CRC field offset */
    (void)memcpy(&dev->mirror[DEV_EEP_LAYOUT_CRC_OFFSET],
                 &crc_value,
                 DEV_EEP_LAYOUT_CRC_SIZE);

    return DEV_OK;
}
```

Replace `dev_eep_check_crc` stub:

```c
static dev_err_t dev_eep_check_crc(const dev_eep_device_t *dev)
{
    uint16_t computed_crc;
    uint16_t stored_crc;
    dev_err_t result;

    DEV_CHECK_RET((dev != NULL), DEV_ERR_NULL_PTR);

    /* Compute CRC-16 over the defined CRC region */
    result = dev_crc16_compute(&dev->mirror[DEV_EEP_CRC_START_OFFSET],
                               (size_t)DEV_EEP_CRC_DATA_LENGTH,
                               &computed_crc);
    DEV_CHECK_OK_RET(result);

    /* Read stored CRC from mirror */
    (void)memcpy(&stored_crc,
                 &dev->mirror[DEV_EEP_LAYOUT_CRC_OFFSET],
                 DEV_EEP_LAYOUT_CRC_SIZE);

    if (computed_crc != stored_crc)
    {
        return DEV_ERR_CRC;
    }

    return DEV_OK;
}
```

Append field-based APIs:

```c
/* ── Field-based read/write ── */

dev_err_t dev_eep_read_field(dev_eep_field_id_t field_id,
                             void *data,
                             dev_eep_size_t length)
{
    const dev_eep_field_t *field;
    const dev_eep_device_t *dev;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    DEV_CHECK_PTR_RET(data);

    field = dev_eep_find_field(field_id);
    DEV_CHECK_RET((field != NULL), DEV_ERR_INVALID_ARG);

    if (length != field->size)
    {
        return DEV_ERR_INVALID_ARG;
    }

    /* Use DEV_EEP_MAIN as the default device */
    dev = dev_eep_find_device(DEV_EEP_MAIN);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_CONFIG);

    /* Read from mirror */
    (void)memcpy(data, &dev->mirror[field->offset], (size_t)field->size);

    return DEV_OK;
}

dev_err_t dev_eep_write_field(dev_eep_field_id_t field_id,
                              const void *data,
                              dev_eep_size_t length)
{
    const dev_eep_field_t *field;
    const dev_eep_device_t *dev;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    DEV_CHECK_PTR_RET(data);

    field = dev_eep_find_field(field_id);
    DEV_CHECK_RET((field != NULL), DEV_ERR_INVALID_ARG);

    if (length != field->size)
    {
        return DEV_ERR_INVALID_ARG;
    }

    /* Use DEV_EEP_MAIN as the default device */
    dev = dev_eep_find_device(DEV_EEP_MAIN);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_CONFIG);

    /* Delegate to raw write (which does compare-before-write + dirty tracking) */
    return dev_eep_write(DEV_EEP_MAIN, field->offset,
                         (const uint8_t *)data, field->size);
}

dev_err_t dev_eep_get_field_info(dev_eep_field_id_t field_id,
                                 const dev_eep_field_t **field)
{
    DEV_CHECK_PTR_RET(field);

    *field = dev_eep_find_field(field_id);
    DEV_CHECK_RET(((*field) != NULL), DEV_ERR_INVALID_ARG);

    return DEV_OK;
}
```

Append typed APIs:

```c
/* ── Typed read/write ── */

dev_err_t dev_eep_read_u8(dev_eep_field_id_t field_id, uint8_t *value)
{
    DEV_CHECK_PTR_RET(value);
    return dev_eep_read_field(field_id, (void *)value, (dev_eep_size_t)sizeof(uint8_t));
}

dev_err_t dev_eep_write_u8(dev_eep_field_id_t field_id, uint8_t value)
{
    return dev_eep_write_field(field_id, (const void *)&value, (dev_eep_size_t)sizeof(uint8_t));
}

dev_err_t dev_eep_read_u16(dev_eep_field_id_t field_id, uint16_t *value)
{
    DEV_CHECK_PTR_RET(value);
    return dev_eep_read_field(field_id, (void *)value, (dev_eep_size_t)sizeof(uint16_t));
}

dev_err_t dev_eep_write_u16(dev_eep_field_id_t field_id, uint16_t value)
{
    return dev_eep_write_field(field_id, (const void *)&value, (dev_eep_size_t)sizeof(uint16_t));
}

dev_err_t dev_eep_read_u32(dev_eep_field_id_t field_id, uint32_t *value)
{
    DEV_CHECK_PTR_RET(value);
    return dev_eep_read_field(field_id, (void *)value, (dev_eep_size_t)sizeof(uint32_t));
}

dev_err_t dev_eep_write_u32(dev_eep_field_id_t field_id, uint32_t value)
{
    return dev_eep_write_field(field_id, (const void *)&value, (dev_eep_size_t)sizeof(uint32_t));
}
```

Append dirty-state APIs:

```c
/* ── Dirty state ── */

bool dev_eep_is_dirty(dev_eep_id_t eep_id)
{
    const dev_eep_device_t *dev;
    dev_eep_size_t page_index;
    dev_eep_size_t page_count;

    dev = dev_eep_find_device(eep_id);
    if (dev == NULL)
    {
        return false;
    }

    page_count = dev->total_size / dev->page_size;

    for (page_index = 0U; page_index < page_count; page_index++)
    {
        if (dev_eep_is_page_dirty(dev, page_index))
        {
            return true;
        }
    }

    return false;
}

dev_err_t dev_eep_mark_dirty(dev_eep_id_t eep_id,
                             dev_eep_addr_t addr,
                             dev_eep_size_t length)
{
    const dev_eep_device_t *dev;
    dev_eep_size_t start_page;
    dev_eep_size_t end_page;
    dev_eep_size_t page;
    dev_err_t result;

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    result = dev_eep_validate_addr(dev, addr, length);
    DEV_CHECK_OK_RET(result);

#if (DEV_EEP_CFG_DIRTY_TRACKING_ENABLED == DEV_ON)
    {
        start_page = dev_eep_addr_to_page(dev, addr);
        end_page   = dev_eep_addr_to_page(dev, addr + length - 1U);

        for (page = start_page; page <= end_page; page++)
        {
            dev_eep_set_page_dirty(dev, page);
        }
    }
#endif

    return DEV_OK;
}

dev_err_t dev_eep_clear_dirty(dev_eep_id_t eep_id)
{
    const dev_eep_device_t *dev;

    dev = dev_eep_find_device(eep_id);
    DEV_CHECK_RET((dev != NULL), DEV_ERR_INVALID_ARG);

    (void)memset(dev->dirty_map, 0, (size_t)dev->dirty_map_size);

    return DEV_OK;
}

uint16_t dev_eep_get_dirty_page_count(dev_eep_id_t eep_id)
{
    const dev_eep_device_t *dev;
    dev_eep_size_t page_index;
    dev_eep_size_t page_count;
    uint16_t count = 0U;

    dev = dev_eep_find_device(eep_id);
    if (dev == NULL)
    {
        return 0U;
    }

    page_count = dev->total_size / dev->page_size;

    for (page_index = 0U; page_index < page_count; page_index++)
    {
        if (dev_eep_is_page_dirty(dev, page_index))
        {
            count++;
        }
    }

    return count;
}
```

- [ ] **Step 2: Verify build**

Run: `gcc -fsyntax-only -Wall -Idrivers/dev_eep/include -Idrivers/dev_common/include -Idrivers/dev_i2c/include -Idrivers/dev_crc/include drivers/dev_eep/src/dev_eep.c 2>&1`
Expected: clean compile, no errors.

- [ ] **Step 3: Commit**

```bash
git add drivers/dev_eep/src/dev_eep.c
git commit -m "feat: implement CRC, field-based, typed, and dirty-state APIs"
```

---

### Task 13: Increase mock I2C memory for EEPROM testing

**Files:**
- Modify: `drivers/dev_i2c/port/mock/dev_i2c_port_mock.c`

**Interfaces:**
- Consumes: nothing
- Produces: `MOCK_MEM_MAX` increased from 256 to 2048

- [ ] **Step 1: Increase MOCK_MEM_MAX**

Change:
```c
#define MOCK_MEM_MAX       (256U)
```
To:
```c
#define MOCK_MEM_MAX       (2048U)
```

- [ ] **Step 2: Commit**

```bash
git add drivers/dev_i2c/port/mock/dev_i2c_port_mock.c
git commit -m "fix: increase mock I2C memory to 2048 bytes for EEPROM tests"
```

---

### Task 14: Write host-based unit tests

**Files:**
- Create: `tests/dev_eep/test_eep.c`
- Create: `tests/dev_eep/CMakeLists.txt`

**Interfaces:**
- Consumes: `dev_eep.h`, `dev_i2c.h` (mock), `dev_common.h`, `dev_i2c_port_mock.h`
- Produces: host-based test executable

- [ ] **Step 1: Write CMakeLists.txt for tests**

```cmake
cmake_minimum_required(VERSION 3.16)
project(eep_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)

# Mock I2C port
set(DEV_I2C_PORT "mock" CACHE STRING "I2C port for testing")

add_subdirectory(${PROJECT_ROOT}/drivers/dev_common ${CMAKE_BINARY_DIR}/dev_common)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_crc ${CMAKE_BINARY_DIR}/dev_crc)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_i2c ${CMAKE_BINARY_DIR}/dev_i2c)

# Compile dev_eep as part of the test (no separate library yet)
add_executable(eep_test_host
    test_eep.c
    ${PROJECT_ROOT}/drivers/dev_eep/src/dev_eep.c
    ${PROJECT_ROOT}/drivers/dev_eep/src/dev_eep_layout.c
)

target_include_directories(eep_test_host PRIVATE
    ${PROJECT_ROOT}/drivers/dev_eep/include
    ${PROJECT_ROOT}/drivers/dev_i2c/port/mock
)

target_link_libraries(eep_test_host PRIVATE dev_common dev_crc dev_i2c)
target_compile_options(eep_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
```

- [ ] **Step 2: Write test_eep.c with comprehensive tests**

```c
#include "dev_eep.h"
#include "dev_i2c.h"
#include "dev_i2c_port_mock.h"
#include "dev_crc.h"
#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)

#define CHECK(cond, msg) do {                                  \
    if (!(cond)) {                                             \
        printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        g_failures++; return;                                   \
    }                                                           \
} while (false)

#define CHECK_EQ(a, b, msg) do {                               \
    if ((a) != (b)) {                                          \
        printf("  FAIL: %s (expected 0x%x, got 0x%x) (%s:%d)\n", \
               msg, (unsigned)(b), (unsigned)(a), __FILE__, __LINE__); \
        g_failures++; return;                                   \
    }                                                           \
} while (false)

#define CHECK_ERR(e, expected, msg) do {                       \
    if ((e) != (expected)) {                                   \
        printf("  FAIL: %s (expected %d, got %d) (%s:%d)\n",   \
               msg, (int)(expected), (int)(e), __FILE__, __LINE__); \
        g_failures++; return;                                   \
    }                                                           \
} while (false)

#define RUN_TEST(name) do { printf("  test_%s...\n", #name); test_##name(); } while (false)

/* ── Helper: build valid EEPROM image with magic + version + CRC ── */

static uint8_t s_eeprom_image[DEV_EEP_MAIN_TOTAL_SIZE];

static void build_valid_eeprom_image(void)
{
    uint32_t magic   = DEV_EEP_MAGIC_VALUE;
    uint16_t version = DEV_EEP_LAYOUT_VERSION;
    uint16_t crc;

    (void)memset(s_eeprom_image, 0xFFU, sizeof(s_eeprom_image));

    /* Write magic */
    (void)memcpy(&s_eeprom_image[DEV_EEP_LAYOUT_MAGIC_OFFSET],
                 &magic, DEV_EEP_LAYOUT_MAGIC_SIZE);

    /* Write version */
    (void)memcpy(&s_eeprom_image[DEV_EEP_LAYOUT_VERSION_OFFSET],
                 &version, DEV_EEP_LAYOUT_VERSION_SIZE);

    /* Compute and write CRC */
    (void)dev_crc16_compute(&s_eeprom_image[DEV_EEP_CRC_START_OFFSET],
                            (size_t)DEV_EEP_CRC_DATA_LENGTH, &crc);
    (void)memcpy(&s_eeprom_image[DEV_EEP_LAYOUT_CRC_OFFSET],
                 &crc, DEV_EEP_LAYOUT_CRC_SIZE);
}

/* ── Helper: attach mock EEPROM device ── */

static void setup_mock_eeprom(void)
{
    build_valid_eeprom_image();
    dev_i2c_port_mock_reset();
    dev_i2c_port_mock_attach_device(DEV_I2C_BUS_EEPROM,
                                    ((dev_i2c_addr_t)0x50U),
                                    s_eeprom_image,
                                    DEV_EEP_MAIN_TOTAL_SIZE);
}

/* ── Test cases ── */

TEST(1_not_init_returns_error)
{
    uint8_t data[4U];
    /* All APIs should return DEV_ERR_NOT_INITIALIZED before init */
    CHECK_ERR(dev_eep_read(DEV_EEP_MAIN, 0U, data, 4U),
              DEV_ERR_NOT_INITIALIZED, "read before init");
    CHECK_ERR(dev_eep_write(DEV_EEP_MAIN, 0U, data, 4U),
              DEV_ERR_NOT_INITIALIZED, "write before init");
    CHECK_ERR(dev_eep_shutdown(),
              DEV_ERR_NOT_INITIALIZED, "shutdown before init");
    CHECK_EQ(dev_eep_is_initialized(), false, "not initialized");
    printf("    PASS\n"); g_passes++;
}

TEST(2_init_success)
{
    if (dev_eep_is_initialized()) { (void)dev_eep_shutdown(); }
    setup_mock_eeprom();

    CHECK_ERR(dev_eep_init(), DEV_OK, "init");
    CHECK_EQ(dev_eep_is_initialized(), true, "is initialized");
    printf("    PASS\n"); g_passes++;
}

TEST(3_double_init_fails)
{
    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    CHECK_ERR(dev_eep_init(), DEV_ERR_ALREADY_INITIALIZED, "double init");
    printf("    PASS\n"); g_passes++;
}

TEST(4_shutdown_success)
{
    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    CHECK_ERR(dev_eep_shutdown(), DEV_OK, "shutdown");
    CHECK_EQ(dev_eep_is_initialized(), false, "not initialized after shutdown");
    printf("    PASS\n"); g_passes++;
}

TEST(5_deinit_clears_state)
{
    setup_mock_eeprom();
    (void)dev_eep_init();

    CHECK_ERR(dev_eep_deinit(), DEV_OK, "deinit");
    CHECK_EQ(dev_eep_is_initialized(), false, "not initialized");
    printf("    PASS\n"); g_passes++;
}

TEST(6_deinit_before_init_fails)
{
    if (dev_eep_is_initialized()) { (void)dev_eep_shutdown(); }

    CHECK_ERR(dev_eep_deinit(), DEV_ERR_NOT_INITIALIZED, "deinit before init");
    printf("    PASS\n"); g_passes++;
}

TEST(7_write_and_read_back)
{
    uint8_t write_data[4U] = { 0xAAU, 0xBBU, 0xCCU, 0xDDU };
    uint8_t read_data[4U]  = { 0U };

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    CHECK_ERR(dev_eep_write(DEV_EEP_MAIN, 0U, write_data, 4U), DEV_OK, "write");
    CHECK_ERR(dev_eep_read(DEV_EEP_MAIN, 0U, read_data, 4U), DEV_OK, "read");

    CHECK_EQ(read_data[0U], 0xAAU, "byte 0");
    CHECK_EQ(read_data[1U], 0xBBU, "byte 1");
    CHECK_EQ(read_data[2U], 0xCCU, "byte 2");
    CHECK_EQ(read_data[3U], 0xDDU, "byte 3");
    printf("    PASS\n"); g_passes++;
}

TEST(8_write_marks_dirty)
{
    uint8_t data[4U] = { 0x11U, 0x22U, 0x33U, 0x44U };

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    (void)dev_eep_clear_dirty(DEV_EEP_MAIN);
    CHECK_EQ(dev_eep_is_dirty(DEV_EEP_MAIN), false, "not dirty initially");

    CHECK_ERR(dev_eep_write(DEV_EEP_MAIN, 16U, data, 4U), DEV_OK, "write");
    CHECK_EQ(dev_eep_is_dirty(DEV_EEP_MAIN), true, "dirty after write");
    printf("    PASS\n"); g_passes++;
}

TEST(9_identical_write_does_not_mark_dirty)
{
    uint8_t data[4U] = { 0xDEU, 0xADU, 0xBEU, 0xEFU };

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    /* Write first time */
    CHECK_ERR(dev_eep_write(DEV_EEP_MAIN, 32U, data, 4U), DEV_OK, "first write");
    (void)dev_eep_clear_dirty(DEV_EEP_MAIN);

    /* Write same data again */
    CHECK_ERR(dev_eep_write(DEV_EEP_MAIN, 32U, data, 4U), DEV_OK, "second write");
    CHECK_EQ(dev_eep_is_dirty(DEV_EEP_MAIN), false, "not dirty after identical write");
    printf("    PASS\n"); g_passes++;
}

TEST(10_field_read_write)
{
    uint32_t boot_count;
    uint32_t read_val;

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    boot_count = 42U;
    CHECK_ERR(dev_eep_write_u32(DEV_EEP_FIELD_BOOT_COUNT, boot_count), DEV_OK, "write u32");

    read_val = 0U;
    CHECK_ERR(dev_eep_read_u32(DEV_EEP_FIELD_BOOT_COUNT, &read_val), DEV_OK, "read u32");
    CHECK_EQ(read_val, 42U, "boot_count == 42");
    printf("    PASS\n"); g_passes++;
}

TEST(11_field_u8_read_write)
{
    uint8_t val;

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    /* Use magic field's first byte for u8 test */
    CHECK_ERR(dev_eep_write_u8(DEV_EEP_FIELD_MAGIC, 0x50U), DEV_OK, "write u8");

    val = 0U;
    CHECK_ERR(dev_eep_read_u8(DEV_EEP_FIELD_MAGIC, &val), DEV_OK, "read u8");
    CHECK_EQ(val, 0x50U, "magic byte == 0x50");
    printf("    PASS\n"); g_passes++;
}

TEST(12_invalid_eep_id)
{
    uint8_t data[4U];

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    CHECK_ERR(dev_eep_read(99U, 0U, data, 4U),
              DEV_ERR_INVALID_ARG, "invalid eep_id for read");
    CHECK_ERR(dev_eep_write(99U, 0U, data, 4U),
              DEV_ERR_INVALID_ARG, "invalid eep_id for write");
    printf("    PASS\n"); g_passes++;
}

TEST(13_invalid_field_id)
{
    uint32_t val;

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    CHECK_ERR(dev_eep_read_u32(99U, &val),
              DEV_ERR_INVALID_ARG, "invalid field_id for read");
    CHECK_ERR(dev_eep_write_u32(99U, 0U),
              DEV_ERR_INVALID_ARG, "invalid field_id for write");
    printf("    PASS\n"); g_passes++;
}

TEST(14_null_pointer)
{
    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    CHECK_ERR(dev_eep_read(DEV_EEP_MAIN, 0U, NULL, 4U),
              DEV_ERR_NULL_PTR, "null data for read");
    CHECK_ERR(dev_eep_write(DEV_EEP_MAIN, 0U, NULL, 4U),
              DEV_ERR_NULL_PTR, "null data for write");
    CHECK_ERR(dev_eep_read_u32(DEV_EEP_FIELD_BOOT_COUNT, NULL),
              DEV_ERR_NULL_PTR, "null value for read_u32");
    printf("    PASS\n"); g_passes++;
}

TEST(15_address_out_of_range)
{
    uint8_t data[4U];

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    CHECK_ERR(dev_eep_read(DEV_EEP_MAIN, DEV_EEP_MAIN_TOTAL_SIZE, data, 1U),
              DEV_ERR_OUT_OF_RANGE, "addr == total_size");
    CHECK_ERR(dev_eep_write(DEV_EEP_MAIN, DEV_EEP_MAIN_TOTAL_SIZE + 1U, data, 1U),
              DEV_ERR_OUT_OF_RANGE, "addr > total_size");
    printf("    PASS\n"); g_passes++;
}

TEST(16_length_out_of_range)
{
    uint8_t data[16U];

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    /* addr + length exceeds total size */
    CHECK_ERR(dev_eep_read(DEV_EEP_MAIN,
                           DEV_EEP_MAIN_TOTAL_SIZE - 4U, data, 8U),
              DEV_ERR_OUT_OF_RANGE, "addr + length > total_size");
    printf("    PASS\n"); g_passes++;
}

TEST(17_zero_length)
{
    uint8_t data[1U];

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    CHECK_ERR(dev_eep_read(DEV_EEP_MAIN, 0U, data, 0U),
              DEV_ERR_INVALID_ARG, "zero-length read");
    printf("    PASS\n"); g_passes++;
}

TEST(18_get_field_info)
{
    const dev_eep_field_t *field = NULL;

    CHECK_ERR(dev_eep_get_field_info(DEV_EEP_FIELD_BOOT_COUNT, &field),
              DEV_OK, "get field info");
    CHECK(field != NULL, "field not null");
    CHECK_EQ(field->field_id, DEV_EEP_FIELD_BOOT_COUNT, "field_id match");
    CHECK_EQ(field->offset, DEV_EEP_LAYOUT_BOOT_COUNT_OFFSET, "offset match");
    CHECK_EQ(field->size, DEV_EEP_LAYOUT_BOOT_COUNT_SIZE, "size match");
    printf("    PASS\n"); g_passes++;
}

TEST(19_get_field_info_null_ptr)
{
    CHECK_ERR(dev_eep_get_field_info(DEV_EEP_FIELD_BOOT_COUNT, NULL),
              DEV_ERR_NULL_PTR, "null ptr");
    printf("    PASS\n"); g_passes++;
}

TEST(20_clear_dirty)
{
    uint8_t data[8U];

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    (void)memset(data, 0xFFU, sizeof(data));
    CHECK_ERR(dev_eep_write(DEV_EEP_MAIN, 64U, data, 8U), DEV_OK, "write");
    CHECK_EQ(dev_eep_is_dirty(DEV_EEP_MAIN), true, "dirty");

    CHECK_ERR(dev_eep_clear_dirty(DEV_EEP_MAIN), DEV_OK, "clear_dirty");
    CHECK_EQ(dev_eep_is_dirty(DEV_EEP_MAIN), false, "not dirty after clear");
    printf("    PASS\n"); g_passes++;
}

TEST(21_dirty_page_count)
{
    uint8_t data[32U];

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    (void)dev_eep_clear_dirty(DEV_EEP_MAIN);

    /* Write across 2 pages (page_size = 16, 32 bytes = 2 pages) */
    (void)memset(data, 0xABU, sizeof(data));
    CHECK_ERR(dev_eep_write(DEV_EEP_MAIN, 80U, data, 32U), DEV_OK, "write 32 bytes");

    CHECK_EQ(dev_eep_get_dirty_page_count(DEV_EEP_MAIN), 2U, "2 dirty pages");
    printf("    PASS\n"); g_passes++;
}

TEST(22_field_length_mismatch)
{
    uint8_t val;

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    /* DEV_EEP_FIELD_BOOT_COUNT is 4 bytes, but we pass 1 byte length */
    CHECK_ERR(dev_eep_read_field(DEV_EEP_FIELD_BOOT_COUNT, &val, 1U),
              DEV_ERR_INVALID_ARG, "length mismatch for read_field");
    printf("    PASS\n"); g_passes++;
}

TEST(23_mark_dirty_invalid_eep_id)
{
    CHECK_ERR(dev_eep_mark_dirty(99U, 0U, 4U),
              DEV_ERR_INVALID_ARG, "mark_dirty invalid eep_id");
    printf("    PASS\n"); g_passes++;
}

TEST(24_device_name_field)
{
    const char *name = "GachDriver";
    char read_name[DEV_EEP_LAYOUT_DEVICE_NAME_SIZE];

    if (!dev_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)dev_eep_init();
    }

    CHECK_ERR(dev_eep_write_field(DEV_EEP_FIELD_DEVICE_NAME,
                                  name, (dev_eep_size_t)strlen(name) + 1U),
              DEV_OK, "write device name");

    (void)memset(read_name, 0, sizeof(read_name));
    CHECK_ERR(dev_eep_read_field(DEV_EEP_FIELD_DEVICE_NAME,
                                 read_name, DEV_EEP_LAYOUT_DEVICE_NAME_SIZE),
              DEV_OK, "read device name");

    CHECK(strcmp(read_name, name) == 0, "device name matches");
    printf("    PASS\n"); g_passes++;
}

int main(void)
{
    printf("=== dev_eep host tests ===\n\n");

    /* Initialize I2C (mock) once for all tests */
    (void)dev_i2c_init();

    RUN_TEST(1_not_init_returns_error);
    RUN_TEST(2_init_success);

    /* Reset state for next group */
    if (dev_eep_is_initialized()) { (void)dev_eep_deinit(); }

    RUN_TEST(3_double_init_fails);

    (void)dev_eep_deinit();

    RUN_TEST(4_shutdown_success);
    RUN_TEST(5_deinit_clears_state);
    RUN_TEST(6_deinit_before_init_fails);
    RUN_TEST(7_write_and_read_back);
    RUN_TEST(8_write_marks_dirty);
    RUN_TEST(9_identical_write_does_not_mark_dirty);
    RUN_TEST(10_field_read_write);
    RUN_TEST(11_field_u8_read_write);
    RUN_TEST(12_invalid_eep_id);
    RUN_TEST(13_invalid_field_id);
    RUN_TEST(14_null_pointer);
    RUN_TEST(15_address_out_of_range);
    RUN_TEST(16_length_out_of_range);
    RUN_TEST(17_zero_length);
    RUN_TEST(18_get_field_info);
    RUN_TEST(19_get_field_info_null_ptr);
    RUN_TEST(20_clear_dirty);
    RUN_TEST(21_dirty_page_count);
    RUN_TEST(22_field_length_mismatch);
    RUN_TEST(23_mark_dirty_invalid_eep_id);
    RUN_TEST(24_device_name_field);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);

    return (g_failures > 0) ? 1 : 0;
}
```

- [ ] **Step 3: Build and run tests**

Run:
```bash
cd tests/dev_eep && mkdir -p build && cd build && cmake .. && make -j4 && ./eep_test_host
```
Expected: all 24 tests pass.

- [ ] **Step 4: Commit**

```bash
git add tests/dev_eep/
git commit -m "test: add 24 host-based unit tests for dev_eep"
```

---

### Task 15: Create component documentation

**Files:**
- Create: `docs/dev_eep/README.md`

- [ ] **Step 1: Write docs/dev_eep/README.md**

```markdown
# dev_eep

I2C EEPROM service layer with RAM mirror and dirty-page tracking.

## Purpose

`dev_eep` provides safe, wear-reducing EEPROM access through `dev_i2c`.
It keeps a RAM mirror of EEPROM content, marks dirty pages when data
changes, and flushes only modified pages to physical EEPROM.

## Architecture

```
Application → dev_eep → dev_i2c → dev_i2c_port_<target> → I2C EEPROM
                ↓
           dev_common (types, errors, assert, delay)
           dev_crc    (CRC-16 data integrity)
```

## Public APIs

### Lifecycle

| Function | Description |
|----------|-------------|
| `dev_eep_init()` | Validate config, read EEPROM into mirror, check CRC |
| `dev_eep_shutdown()` | Flush dirty pages, clear state |
| `dev_eep_deinit()` | Clear mirror and state without flushing |
| `dev_eep_is_initialized()` | Return initialization state |

### Raw Read/Write (RAM mirror)

| Function | Description |
|----------|-------------|
| `dev_eep_read()` | Copy from mirror to buffer |
| `dev_eep_write()` | Copy to mirror, mark dirty if changed |
| `dev_eep_read_all()` | Read entire EEPROM into mirror |
| `dev_eep_write_all()` | Write entire mirror to EEPROM |
| `dev_eep_flush()` | Write only dirty pages to EEPROM |

### Field-Based Access

| Function | Description |
|----------|-------------|
| `dev_eep_read_field()` | Read a field from the mirror by field ID |
| `dev_eep_write_field()` | Write a field to the mirror by field ID |
| `dev_eep_get_field_info()` | Get field descriptor by field ID |

### Typed Access

`dev_eep_read_u8/write_u8`, `dev_eep_read_u16/write_u16`,
`dev_eep_read_u32/write_u32` — typed wrappers around field APIs.

### Dirty State

`dev_eep_is_dirty()`, `dev_eep_mark_dirty()`, `dev_eep_clear_dirty()`,
`dev_eep_get_dirty_page_count()`.

## Configuration

See `dev_eep_cfg.h` for all compile-time options. Key macros:

- `DEV_EEP_MAIN_TOTAL_SIZE` — EEPROM size in bytes (default 1024)
- `DEV_EEP_MAIN_PAGE_SIZE` — EEPROM page size (default 16)
- `DEV_EEP_CFG_CRC_ENABLED` — enable CRC-16 validation
- `DEV_EEP_CFG_AUTO_FLUSH_ON_SHUTDOWN` — flush on shutdown
- `DEV_EEP_CFG_WRITE_ONLY_IF_CHANGED` — skip identical writes

## Usage Example

```c
#include "dev_eep.h"
#include "dev_i2c.h"

int main(void) {
    dev_i2c_init();
    dev_eep_init();

    uint32_t boot_count;
    dev_eep_read_u32(DEV_EEP_FIELD_BOOT_COUNT, &boot_count);
    boot_count++;
    dev_eep_write_u32(DEV_EEP_FIELD_BOOT_COUNT, boot_count);

    dev_eep_shutdown();  // flushes dirty pages
}
```

## Safety Notes

- No dynamic memory allocation — all buffers static
- No vendor types in public APIs
- Page writes never cross EEPROM page boundaries
- Write cycle completion handled via ACK polling or delay
- CRC-16 (MODBUS) for data integrity
- Magic value + version for EEPROM validity check

## MISRA-C Notes

- No recursion, no goto, no dynamic allocation
- Fixed-width integer types only
- No pointer arithmetic beyond array indexing
- All return values checked

## Bring-Up Checklist

1. Verify `dev_i2c` works with the EEPROM on the target
2. Configure `DEV_EEP_MAIN_TOTAL_SIZE` and `DEV_EEP_MAIN_PAGE_SIZE`
3. Verify `DEV_I2C_BUS_EEPROM` is mapped correctly
4. Set EEPROM I2C address in device table
5. Provide real `dev_delay_ms()` implementation
6. Build and test init/read/write/flush on hardware

## Review Checklist

- [ ] No vendor HAL headers in public includes
- [ ] No dynamic memory allocation
- [ ] Static mirror and dirty map
- [ ] Page writes respect boundaries
- [ ] Write cycle completion handled
- [ ] CRC region excludes CRC field
- [ ] Identical data not re-written
- [ ] Dirty bit cleared only after successful write
```

- [ ] **Step 2: Commit**

```bash
git add docs/dev_eep/README.md
git commit -m "docs: add dev_eep component documentation"
```

---

### Task 16: Update root README.md

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add dev_eep row to the components table**

Insert after the `dev_adc` row:

```markdown
| `dev_eep` | I2C EEPROM service with RAM mirror and dirty tracking | [docs/dev_eep/README.md](docs/dev_eep/README.md) |
```

- [ ] **Step 2: Add dev_eep to the folder structure section**

Add `dev_eep/` to the drivers listing.

- [ ] **Step 3: Commit**

```bash
git add README.md
git commit -m "docs: add dev_eep to root README"
```

---

### Task 17: Final integration — ensure everything builds together

**Files:**
- Verify: all files compile and link
- Verify: all tests pass
- Verify: no regressions in existing tests

- [ ] **Step 1: Build full project**

```bash
cd build && cmake .. && make -j4
```

- [ ] **Step 2: Run all existing tests**

```bash
cd tests/dev_crc/build && cmake .. && make && ./crc_test_host
cd tests/dev_gpio/build && cmake .. && make && ./gpio_test_host
cd tests/dev_i2c/build && cmake .. && make && ./i2c_test_host
cd tests/dev_eep/build && cmake .. && make && ./eep_test_host
```

- [ ] **Step 3: Run gitnexus detect_changes to verify scope**

Run: `gitnexus_detect_changes()` to confirm changes only affect expected symbols.

- [ ] **Step 4: Final commit if needed**

```bash
git add -A
git commit -m "chore: final integration — all tests pass"
```
```

- [ ] **Step 5 (final task only): Push**

```bash
git push origin main
```
```
