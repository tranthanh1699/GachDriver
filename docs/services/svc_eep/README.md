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
| Direct write | Write immediately to EEPROM via `svc_eep_write_direct()` |
| Mirror write | Write to RAM only via `svc_eep_write_mirror()` — sync later |
| Selective sync | Sync only dirty blocks to EEPROM via `svc_eep_sync_block()` or `svc_eep_sync_all()` |

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

Each block has a static configuration descriptor:

```c
typedef struct
{
    uint8_t        block_id;
    uint32_t       eep_offset;
    uint16_t       block_size;
    uint8_t       *mirror;
} svc_eep_block_cfg_t;
```

The config table is indexed by block ID and lives in `svc_eep_blocks.c`.

### Block Runtime State

Each block has runtime state tracking:

```c
typedef struct
{
    bool loaded;   /* mirror contains valid data from EEPROM */
    bool dirty;    /* mirror differs from EEPROM */
    bool valid;    /* mirror data is usable */
} svc_eep_block_state_t;
```

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

- `svc_eep_init()` — validates block config, inits `dev_eep`, initializes all block
  states to not-loaded/not-dirty/not-valid. Does **not** read any EEPROM data.
- `svc_eep_deinit()` — clears block states, deinits `dev_eep`. Does **not** write EEPROM.
- `svc_eep_shutdown()` — syncs all dirty blocks (if `SVC_EEP_CFG_AUTO_SYNC_ON_SHUTDOWN`
  is `DEV_ON`), then deinits `dev_eep`.

### Block Load

```c
dev_err_t svc_eep_load_block(svc_eep_block_id_t block_id);
```

Reads a single block from EEPROM into its mirror via `dev_eep_read()`.
Does not read other blocks. Sets `loaded = true`, `valid = true`, `dirty = false`.

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

### State Queries

```c
bool svc_eep_is_block_loaded(svc_eep_block_id_t block_id);
bool svc_eep_is_block_dirty(svc_eep_block_id_t block_id);
bool svc_eep_is_dirty(void);
```

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
| `SVC_EEP_CFG_DYNAMIC_MIRROR_ENABLED` | `DEV_OFF` | Enable dynamic mirror allocation |
| `SVC_EEP_CFG_AUTO_SYNC_ON_SHUTDOWN` | `DEV_ON` | Sync dirty blocks during shutdown |
| `SVC_EEP_CFG_RUNTIME_CHECK_ENABLED` | `DEV_ON` | Enable parameter validation |

### Block Count Validation

`SVC_EEP_BLOCK_COUNT` must not exceed `SVC_EEP_CFG_MAX_BLOCKS`.
A compile-time `#error` enforces this.

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

2. Define the block size and EEPROM offset in `svc_eep_blocks.c`:

```c
#define SVC_EEP_BLOCK_NEW_BLOCK_SIZE    (48U)
#define SVC_EEP_BLOCK_NEW_BLOCK_OFFSET  (0x0070U)
```

3. Declare a static mirror buffer in `svc_eep_blocks.c`:

```c
uint8_t s_new_block_mirror[SVC_EEP_BLOCK_NEW_BLOCK_SIZE];
```

4. Add the mirror extern declaration in `svc_eep_blocks.h`:

```c
extern uint8_t s_new_block_mirror[];
```

5. Add a config table entry in `svc_eep_blocks.c`:

```c
[SVC_EEP_BLOCK_NEW_BLOCK] =
{
    .block_id   = (uint8_t)SVC_EEP_BLOCK_NEW_BLOCK,
    .eep_offset = SVC_EEP_BLOCK_NEW_BLOCK_OFFSET,
    .block_size = SVC_EEP_BLOCK_NEW_BLOCK_SIZE,
    .mirror     = s_new_block_mirror
},
```

6. Rebuild and verify the block doesn't overlap with existing blocks.

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

- Block IDs are validated against `SVC_EEP_BLOCK_COUNT` before any array access.
- All pointer arguments are validated for NULL.
- Length must match configured block size — partial access is rejected.
- Block EEPROM ranges are validated for overlap during `svc_eep_init()`.
- `svc_eep_mark_dirty()` requires the block to be loaded first.
- No dynamic allocation in default configuration (`SVC_EEP_CFG_DYNAMIC_MIRROR_ENABLED = DEV_OFF`).
- Invalid block ID returns `DEV_ERR_INVALID_ARG` without accessing the config table out of bounds.

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
