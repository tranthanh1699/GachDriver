# svc_eep — EEPROM Block Service

## 1. What It Is

`svc_eep` is a block-based EEPROM management service. It organizes EEPROM data into
independently managed **blocks**, each with its own RAM mirror. Blocks are loaded
from EEPROM on demand — not all at once. Dirty blocks are written back individually
or as a group.

`svc_eep` delegates all I2C communication to `dev_eep` (the EEPROM device driver),
which in turn uses `dev_i2c`. **`svc_eep` never calls I2C APIs directly and never
includes vendor headers.**

### Key Features

| Feature | What it does |
|---------|-------------|
| Per-block RAM mirror | Each block has its own mirror — no full-EEPROM copy |
| Lazy loading | Blocks are loaded from EEPROM only when accessed |
| Per-block dirty tracking | Each block tracks whether its mirror differs from EEPROM |
| Dirty-load protection | Loading a dirty block from EEPROM is rejected — prevents silent data loss |
| Direct write | Write immediately to EEPROM via `svc_eep_write_direct()` |
| Mirror write | Write to RAM only via `svc_eep_write_mirror()` — sync later |
| Selective sync | Sync only dirty blocks to EEPROM via `svc_eep_sync_block()` or `svc_eep_sync_all()` |
| Private mirrors | Mirror buffers and config table are private — external code accesses them only through service APIs |
| Robust ID validation | `svc_eep_block_id_is_valid()` handles negative and wrapped IDs |
| Concurrency model | Documented single-context only |

---

## 2. Architecture

```
                  ┌─────────────┐
                  │  Application│
                  └──────┬──────┘
                         │ svc_eep_read_block / svc_eep_write_mirror
                         │ svc_eep_write_direct / svc_eep_sync_block
                  ┌──────▼──────┐
                  │   svc_eep   │  block management, per-block mirrors
                  │             │  lazy load, dirty tracking, sync
                  └──────┬──────┘
                         │ dev_eep_read / dev_eep_write
                  ┌──────▼──────┐
                  │   dev_eep   │  page splitting, ACK polling, address width
                  └──────┬──────┘
                         │ dev_i2c_mem_read / dev_i2c_mem_write
                  ┌──────▼──────┐
                  │   dev_i2c   │  I2C bus abstraction
                  └──────┬──────┘
                         │ I2C bus (SDA + SCL)
                  ┌──────▼──────┐
                  │ EEPROM chip │  Physical AT24C02 / 24C08 / etc.
                  └─────────────┘
```

### No Full EEPROM Mirror

`svc_eep` does **not** allocate a buffer the size of the entire EEPROM.
Each configured block has its own mirror, sized exactly to that block's data.

```
Block 0 (SYSTEM_CFG):   32-byte mirror
Block 1 (USER_DATA):    64-byte mirror
Block 2 (DEVICE_INFO):  16-byte mirror

Total RAM: 112 bytes (not 256 bytes for full EEPROM)
```

Unused EEPROM regions consume zero RAM.

---

## 3. Block Model

### Block ID

Every block is identified by an enum value. Raw numeric IDs are forbidden.

```c
typedef enum
{
    SVC_EEP_BLOCK_SYSTEM_CFG = 0,
    SVC_EEP_BLOCK_USER_DATA,
    SVC_EEP_BLOCK_DEVICE_INFO,

    SVC_EEP_BLOCK_COUNT
} svc_eep_block_id_t;
```

Application code uses enum names:

```c
svc_eep_read_block(SVC_EEP_BLOCK_SYSTEM_CFG, data, 32);
```

Not raw numbers:

```c
/* FORBIDDEN */
svc_eep_read_block(0, data, 32);
```

### Block Configuration

The public API exposes only metadata through `svc_eep_block_info_t`:

```c
typedef struct
{
    uint8_t        block_id;
    uint32_t       eep_offset;
    uint16_t       block_size;
} svc_eep_block_info_t;
```

The internal configuration type (in `svc_eep_internal.h`, **not** part of the public API) embeds this as a named sub-object to avoid strict-aliasing violations:

```c
typedef struct
{
    svc_eep_block_info_t info;     /* public metadata */
    uint8_t              *mirror;  /* private — only accessible internally */
} svc_eep_block_cfg_t;
```

External code obtains block metadata via `svc_eep_get_block_info(block_id)` which returns `const svc_eep_block_info_t *`. The mirror pointer is never exposed publicly. The config table and mirror buffers are `static` inside `svc_eep_blocks.c`.

### Block Runtime State

Each block has runtime state tracking (`svc_eep_block_state_t` in `svc_eep_internal.h`):

```c
typedef struct
{
    bool loaded;   /* mirror contains data (from EEPROM or from a write) */
    bool dirty;    /* mirror differs from EEPROM — needs sync */
} svc_eep_block_state_t;
```

The `valid` flag previously tracked in this struct has been removed — it was written but never read.

### EEPROM Layout

Blocks are placed at fixed EEPROM offsets. Blocks must not overlap.
The total range of each block (`eep_offset + block_size`) must not exceed
the physical EEPROM size.

Example layout (AT24C02, 256 bytes):

```
0x0000 ┌──────────────────────┐
       │ SYSTEM_CFG  (32 B)   │
0x0020 ├──────────────────────┤
       │ USER_DATA   (64 B)   │
0x0060 ├──────────────────────┤
       │ DEVICE_INFO (16 B)   │
0x0070 ├──────────────────────┤
       │ (unused)             │
0x0100 └──────────────────────┘
```

---

## 4. Public API

### Lifecycle

```c
dev_err_t svc_eep_init(void);
dev_err_t svc_eep_deinit(void);
dev_err_t svc_eep_shutdown(void);
bool      svc_eep_is_initialized(void);
```

- `svc_eep_init()` — validates block config (non-zero size, non-NULL mirrors, non-overlapping EEPROM ranges, total range within device capacity), inits `dev_eep`, initializes all block states to not-loaded/not-dirty. Does **not** read any EEPROM data.
- `svc_eep_deinit()` — deinitializes `dev_eep` **first**, then clears block states only on success. If `dev_eep_deinit()` fails, service state (including dirty flags) is preserved so the caller can retry.
- `svc_eep_shutdown()` — syncs all dirty blocks (if `SVC_EEP_CFG_AUTO_SYNC_ON_SHUTDOWN` is `DEV_ON`), then deinits `dev_eep`. Deinit failure preserves service state.

### Block Load

```c
dev_err_t svc_eep_load_block(svc_eep_block_id_t block_id);
```

Reads a single block from EEPROM into its mirror via `dev_eep_read()`.
Does not read other blocks. Sets `loaded = true`, `dirty = false`.
**Rejects loading if the block is dirty** (returns `DEV_ERR_INVALID_STATE`) —
this prevents silently overwriting unsaved mirror changes. Call `svc_eep_sync_block()`
first to persist, or accept the data loss and deinit/re-init.

### Block Read

```c
dev_err_t svc_eep_read_block(svc_eep_block_id_t block_id,
                             void *data, uint16_t length);
```

If the block is not loaded, loads it from EEPROM first. Copies mirror data to
the output buffer. `length` must equal the configured block size.

### Direct Write (Immediate EEPROM)

```c
dev_err_t svc_eep_write_direct(svc_eep_block_id_t block_id,
                               const void *data, uint16_t length);
```

Writes data immediately to EEPROM via `dev_eep_write()`. Also updates the mirror.
Use this when data must be persisted immediately.

### Mirror Write (RAM Only)

```c
dev_err_t svc_eep_write_mirror(svc_eep_block_id_t block_id,
                               const void *data, uint16_t length);
```

Copies data to the block's RAM mirror only. Sets `dirty = true`. Does **not**
write to EEPROM. Use this to reduce EEPROM wear when data changes frequently.

### Mirror Pointer Access

```c
dev_err_t svc_eep_get_mirror_ptr(svc_eep_block_id_t block_id,
                                 void **ptr, uint16_t *length);

dev_err_t svc_eep_mark_dirty(svc_eep_block_id_t block_id);
```

`svc_eep_get_mirror_ptr()` loads the block if needed and returns a pointer to its
mirror. The application can read/modify data in place. After modifying, the
application **must** call `svc_eep_mark_dirty()`.

### Sync (Write Dirty Mirrors to EEPROM)

```c
dev_err_t svc_eep_sync_block(svc_eep_block_id_t block_id);
dev_err_t svc_eep_sync_all(void);
```

- `svc_eep_sync_block()` — writes one dirty block to EEPROM. Clears dirty flag on success.
  No-op if block is not dirty.
- `svc_eep_sync_all()` — iterates all blocks, syncs only dirty ones. Continues on
  individual block failures (returns first error).

### Block Metadata Access

```c
const svc_eep_block_info_t *svc_eep_get_block_info(svc_eep_block_id_t block_id);
bool svc_eep_block_id_is_valid(svc_eep_block_id_t block_id);
uint8_t svc_eep_get_block_count(void);
```

- `svc_eep_get_block_info()` — returns a pointer to a block's public metadata (block_id, eep_offset, block_size). The returned pointer is valid for the lifetime of the application. Returns NULL for invalid IDs. The mirror pointer is NOT exposed — use `svc_eep_get_mirror_ptr()` or `svc_eep_read_block()` for data access.
- `svc_eep_block_id_is_valid()` — validates a block ID against the configured range. Handles negative values and wrapped IDs correctly regardless of the compiler's enum signedness.
- `svc_eep_get_block_count()` — returns `SVC_EEP_BLOCK_COUNT`.

### State Queries

```c
bool svc_eep_is_block_loaded(svc_eep_block_id_t block_id);
bool svc_eep_is_block_dirty(svc_eep_block_id_t block_id);
bool svc_eep_is_dirty(void);
```

All state queries use `svc_eep_block_id_is_valid()` internally — out-of-range IDs (including negative and wrapped values) return `false` without accessing out-of-bounds memory.

---

## 5. Write Modes

### Mirror Write + Sync (Recommended for Frequent Writes)

```c
/* Accumulate changes in RAM */
svc_eep_write_mirror(SVC_EEP_BLOCK_USER_DATA, new_data, 64);
svc_eep_write_mirror(SVC_EEP_BLOCK_USER_DATA, more_data, 64);

/* Persist all at once */
svc_eep_sync_block(SVC_EEP_BLOCK_USER_DATA);
```

Use case: data that changes frequently. Reduces EEPROM write cycles.

### Direct Write (Immediate Persistence)

```c
svc_eep_write_direct(SVC_EEP_BLOCK_SYSTEM_CFG, critical_data, 32);
```

Use case: data that must survive a power loss immediately.

### Shutdown Sync

```c
/* Called during system shutdown */
svc_eep_shutdown();  /* syncs all dirty blocks, then deinits */
```

---

## 6. Configuration

All configuration lives in `svc_eep_cfg.h`.

| Macro | Default | Purpose |
|-------|---------|---------|
| `SVC_EEP_CFG_MAX_BLOCKS` | `16` | Maximum supported block count |
| `SVC_EEP_CFG_AUTO_SYNC_ON_SHUTDOWN` | `DEV_ON` | Sync dirty blocks during shutdown |

### Block Count Validation

`SVC_EEP_BLOCK_COUNT` must not exceed `SVC_EEP_CFG_MAX_BLOCKS`.
A `_Static_assert` (C) / `static_assert` (C++) in `svc_eep_blocks.h` enforces this
at compile time. Unlike a preprocessor `#if`, this correctly evaluates the enum constant.

---

## 7. Adding a New Block

1. Add a new entry to the `svc_eep_block_id_t` enum in `svc_eep_blocks.h` **before**
   `SVC_EEP_BLOCK_COUNT`:

```c
typedef enum
{
    SVC_EEP_BLOCK_SYSTEM_CFG = 0,
    SVC_EEP_BLOCK_USER_DATA,
    SVC_EEP_BLOCK_DEVICE_INFO,
    SVC_EEP_BLOCK_NEW_BLOCK,       /* <-- add here */

    SVC_EEP_BLOCK_COUNT
} svc_eep_block_id_t;
```

2. Define the block size in `svc_eep_blocks.c` (offset is auto-computed):

```c
#define SVC_EEP_BLOCK_NEW_BLOCK_SIZE    (48U)
```

3. Add a **static** mirror buffer in `svc_eep_blocks.c`:

```c
static uint8_t s_new_block_mirror[SVC_EEP_BLOCK_NEW_BLOCK_SIZE];
```

4. Add a config table entry with the embedded `.info` sub-object:

```c
[SVC_EEP_BLOCK_NEW_BLOCK] =
{
    .info =
    {
        .block_id   = (uint8_t)SVC_EEP_BLOCK_NEW_BLOCK,
        .eep_offset = SVC_EEP_BLOCK_NEW_BLOCK_OFFSET,  /* auto-computed */
        .block_size = SVC_EEP_BLOCK_NEW_BLOCK_SIZE,
    },
    .mirror = s_new_block_mirror
},
```

**Do NOT** add `extern` declarations in any header — mirrors and the config table are private to `svc_eep_blocks.c`. External code accesses block metadata only through `svc_eep_get_block_info()`.

5. Rebuild. The `_Static_assert` in `svc_eep_blocks.h` verifies the new `SVC_EEP_BLOCK_COUNT` is within `SVC_EEP_CFG_MAX_BLOCKS`. The init-time validator checks for overlaps and EEPROM bounds.

---

## 8. Dependency Rules

| Rule | Status |
|------|--------|
| `svc_eep` includes `dev_eep.h` only in `.c` files | Required |
| `svc_eep` never includes `dev_i2c.h` | Required |
| `svc_eep` never calls `dev_i2c_*()` directly | Required |
| `svc_eep` only uses `dev_eep_read()` and `dev_eep_write()` for EEPROM access | Required |
| `svc_eep` never includes STM32 HAL, ESP-IDF, nRF SDK, or FreeRTOS headers | Required |
| `dev_eep` handles page size, address width, ACK polling | Required |

---

## 9. Safety Notes

- **Block ID validation** uses `svc_eep_block_id_is_valid()` which correctly handles negative and wrapped values regardless of the compiler's enum signedness. Out-of-range IDs are rejected before any array access.
- **All pointer arguments** are validated for NULL before use.
- **Length must match** configured block size — partial access is rejected.
- **Block overlap detection** validates all EEPROM ranges are disjoint and within device bounds during `svc_eep_init()`.
- **Dirty-load protection**: `svc_eep_load_block()` rejects loading a dirty block (returns `DEV_ERR_INVALID_STATE`) — prevents silently overwriting unsaved mirror changes.
- **Deinit ordering**: `dev_eep_deinit()` is called **before** clearing block states. If the driver fails to deinit, dirty flags and loaded state are preserved so the caller can retry.
- **Mirrors are private**: mirror buffers and the config table are `static` inside `svc_eep_blocks.c`. External code accesses metadata only through `svc_eep_get_block_info()` which returns a mirror-free descriptor. The internal type (`svc_eep_block_cfg_t`) lives in `svc_eep_internal.h` (not on the public include path).
- **Strict-aliasing compliant**: `svc_eep_block_cfg_t` embeds `svc_eep_block_info_t info` as a **named member** — the public getter returns `&cfg->info`, a valid pointer to a real `svc_eep_block_info_t` object.
- **No dynamic allocation** — all mirrors are statically allocated at compile time.
- **Single-context only** — the service is NOT concurrency-safe. Do not call from multiple threads, tasks, or ISR contexts without external synchronization.
- **`svc_eep_mark_dirty()`** requires the block to be loaded first.
- **Fault injection support** — `dev_eep_set_deinit_fault(true)` enables testing deinit failure paths without hardware modification.

---

## 10. Usage Example

```c
#include "svc_eep.h"

void app_save_settings(const uint8_t *settings, uint16_t len)
{
    dev_err_t err;

    /* Write to mirror only — no EEPROM wear yet */
    err = svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, settings, len);
    if (err != DEV_OK)
    {
        /* Handle error */
        return;
    }

    /* Sync to EEPROM when ready */
    err = svc_eep_sync_block(SVC_EEP_BLOCK_SYSTEM_CFG);
    if (err != DEV_OK)
    {
        /* Handle error */
        return;
    }
}

void app_load_settings(uint8_t *settings, uint16_t len)
{
    /* Auto-loads from EEPROM if not already loaded */
    (void)svc_eep_read_block(SVC_EEP_BLOCK_SYSTEM_CFG, settings, len);
}

void app_critical_save(const uint8_t *data, uint16_t len)
{
    /* Write immediately to EEPROM — no mirror-only step */
    (void)svc_eep_write_direct(SVC_EEP_BLOCK_DEVICE_INFO, data, len);
}
```
