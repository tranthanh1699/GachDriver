# svc_eep — EEPROM Service

## 1. What It Is

`svc_eep` is a safe, wear-reducing service layer for external I2C EEPROM chips. Instead of reading and writing the physical EEPROM on every access, it keeps a **RAM mirror** of the entire EEPROM content. Your application reads and writes the mirror; only when you explicitly flush (or shut down) are changed bytes written back to the physical chip.

This protects EEPROM lifetime — each byte is rated for ~1,000,000 write cycles. Writing only changed pages, and only when you say so, dramatically reduces wear.

`svc_eep` delegates all I2C communication to `dev_eep` (the EEPROM device driver), which in turn uses `dev_i2c`. **`svc_eep` never calls I2C APIs directly and never includes vendor headers.**

### Key Features

| Feature | What it does |
|---------|-------------|
| RAM mirror | All reads come from RAM; zero I2C traffic during normal operation |
| Dirty-page tracking | Only pages with changed data are written to EEPROM |
| Compare-before-write | Identical data is silently skipped — zero wear |
| CRC-16 integrity | Detects corruption at boot |
| Magic + version | Detects blank or corrupted EEPROM on first boot |
| Field-based access | Read/write by logical name (`SVC_EEP_FIELD_BOOT_COUNT`), not raw offset |
| Typed helpers | `svc_eep_read_u32()`, `svc_eep_write_u8()`, etc. — no manual `sizeof` |

---

## 2. Architecture — How the Layers Fit

```
                  ┌─────────────┐
                  │  Application│
                  └──────┬──────┘
                         │ svc_eep_read_field / svc_eep_write_field
                         │ (reads/writes RAM only — no I2C!)
                  ┌──────▼──────┐
                  │  RAM mirror │  256 bytes (configurable per chip)
                  │  + dirty map│  4 bytes (1 bit per page)
                  └──────┬──────┘
                         │ svc_eep_flush / svc_eep_shutdown
                         │ (calls dev_eep_write for each dirty page)
                  ┌──────▼──────┐
                  │   dev_eep   │  page splitting, ACK polling, address width
                  └──────┬──────┘
                         │ dev_eep_read / dev_eep_write
                  ┌──────▼──────┐
                  │   dev_i2c   │  I2C bus abstraction
                  └──────┬──────┘
                         │ I2C bus (SDA + SCL)
                  ┌──────▼──────┐
                  │ EEPROM chip │  Physical AT24C02 / 24C08 / etc.
                  └─────────────┘
```

**Key rule:** `svc_eep` → `dev_eep` → `dev_i2c` → port. Each layer only talks to the one below it.

---

## 3. Lifecycle

```
POWER ON
   │
   ▼
dev_i2c_init()          ← initialize the I2C bus (port layer)
   │
   ▼
svc_eep_init()          ← internally calls dev_eep_init() to probe the chip
   │                     ← reads entire EEPROM → RAM mirror via dev_eep_read()
   │                     ← validates magic, version, CRC
   │                     ← loads defaults if EEPROM is blank or corrupt
   │
   ▼
┌──────────────────────────────────────────────────┐
│  APPLICATION RUNS                                │
│                                                  │
│  svc_eep_read_field()   ← reads from RAM mirror  │
│  svc_eep_write_field()  ← writes to RAM mirror   │
│  svc_eep_read_u32()     ← typed convenience      │
│  svc_eep_write_u32()    ← marks pages dirty       │
│                                                  │
│  svc_eep_flush()        ← writes dirty pages     │
│                            to EEPROM via dev_eep  │
└──────────────────────────────────────────────────┘
   │
   ▼
svc_eep_shutdown()       ← updates CRC, flushes all dirty pages, deinits dev_eep
   │
   ▼
POWER OFF
```

---

## 4. Write Flow — What Happens on `svc_eep_write_field`

```
Application calls svc_eep_write_field(BOOT_COUNT, &val, 4)
   │
   ▼
Is service initialized? ──NO──→ DEV_ERR_NOT_INITIALIZED
   │YES
   ▼
Is field_id valid? ──NO──→ DEV_ERR_INVALID_ARG
   │YES
   ▼
Is length ≤ field.size? ──NO──→ DEV_ERR_INVALID_ARG
   │YES
   ▼
Is new data == existing mirror data? ──YES──→ DEV_OK (no-op, zero wear!)
   │NO
   ▼
memcpy(data → mirror[field.offset])
   │
   ▼
Mark affected pages dirty in dirty_map
   │
   ▼
DEV_OK  (physical EEPROM NOT written yet!)
```

The data is now in RAM. It will be written to EEPROM only when you call `svc_eep_flush()` or `svc_eep_shutdown()`.

---

## 5. Flush Flow — What Happens on `svc_eep_flush`

```
svc_eep_flush()
   │
   ▼
For each page (0 .. DEV_EEP_MAIN_PAGE_COUNT - 1):
   │
   ▼
Is page dirty? ──NO──→ skip to next page
   │YES
   ▼
dev_eep_write(page_addr, mirror[page_addr], page_size)
   │  ↑ dev_eep handles page splitting and ACK polling internally
   ▼
Write succeeded? ──NO──→ return error (dirty bit STAYS SET — can retry)
   │YES
   ▼
Clear dirty bit for this page
   │
   ▼
Next page...
   │
   ▼
DEV_OK
```

---

## 6. Dirty Map — How Dirty Tracking Works

The dirty map is a bit-array. Each bit represents one EEPROM page. Bit = 1 means "this page has been modified in RAM but not yet written to EEPROM."

```
EEPROM pages:    [0] [1] [2] [3] [4] [5] [6] [7] ... [31]
                     ↓
Dirty map bytes: byte[0] = 0b00101100
                   bits:   7 6 5 4 3 2 1 0
                           ↑       ↑ ↑
                      page 5   page 3 page 2 are dirty
```

- The dirty map size is computed automatically: `(DEV_EEP_MAIN_PAGE_COUNT + 7) / 8` bytes.
- For a 256-byte EEPROM with 8-byte pages: 32 pages → 4 bytes.
- Dirty bits are set by `svc_eep_write()` / `svc_eep_write_field()`.
- Dirty bits are cleared only after a successful `dev_eep_write()` in `svc_eep_flush()`.
- If a flush fails mid-way, dirty bits for failed pages remain set — retrying the flush will re-write those pages.

---

## 7. EEPROM Memory Layout

### 7.1 Visual Map

The first 44 bytes are metadata. The remaining space is free for application fields.

```
Offset  Size  Field              Description
──────  ────  ─────────────────  ───────────────────────────────────────
0x0000   4    MAGIC              0x44564550 ("DEVP") — proves valid data
0x0004   2    VERSION            Layout version (currently 1)
0x0006   2    CRC                CRC-16 over bytes 0x0000–0x0005
0x0008   4    BOOT_COUNT         Incremented on each boot
0x000C  32    DEVICE_NAME        Null-terminated device name string
──────  ────  ─────────────────  ───────────────────────────────────────
0x002C 212    (free space)       Add your own fields here
──────  ────  ─────────────────  ───────────────────────────────────────
TOTAL 256                        0x0000 + 4 + 2 + 2 + 4 + 32 = 0x002C (44 bytes used)
```

### 7.2 Built-in Fields

| Field ID macro | C type | Size (bytes) | Purpose |
|---------------|--------|-------------|---------|
| `SVC_EEP_FIELD_MAGIC` | `uint32_t` | 4 | `0x44564550` — proves this EEPROM was programmed by our firmware |
| `SVC_EEP_FIELD_VERSION` | `uint16_t` | 2 | Layout version — bump this when you reorganize fields |
| `SVC_EEP_FIELD_CRC` | `uint16_t` | 2 | CRC-16 over magic + version |
| `SVC_EEP_FIELD_BOOT_COUNT` | `uint32_t` | 4 | Boot counter — application increments each power-on |
| `SVC_EEP_FIELD_DEVICE_NAME` | `char[32]` | 32 | Human-readable name ("Controller-A") |

### 7.3 CRC Coverage

```
CRC covers:  bytes 0x0000 through 0x0005  (magic + version = 6 bytes)
CRC stored:  bytes 0x0006 through 0x0007  (excluded from calculation)
```

The CRC is recalculated and written to EEPROM just before each flush/shutdown.

### 7.4 Field Offset Chain

Each field's offset = previous field's offset + previous field's size. This is a self-verifying chain — if you add a field with the wrong offset, `svc_eep_init()` detects the overlap and returns `DEV_ERR_CONFIG`.

```c
#define SVC_EEP_LAYOUT_MAGIC_OFFSET         (0x0000U)
#define SVC_EEP_LAYOUT_MAGIC_SIZE           (4U)    // → ends at 0x0004

#define SVC_EEP_LAYOUT_VERSION_OFFSET       (0x0004U)
#define SVC_EEP_LAYOUT_VERSION_SIZE         (2U)    // → ends at 0x0006

#define SVC_EEP_LAYOUT_CRC_OFFSET           (0x0006U)
#define SVC_EEP_LAYOUT_CRC_SIZE             (2U)    // → ends at 0x0008

#define SVC_EEP_LAYOUT_BOOT_COUNT_OFFSET    (0x0008U)
#define SVC_EEP_LAYOUT_BOOT_COUNT_SIZE      (4U)    // → ends at 0x000C

#define SVC_EEP_LAYOUT_DEVICE_NAME_OFFSET   (0x000CU)
#define SVC_EEP_LAYOUT_DEVICE_NAME_SIZE     (32U)   // → ends at 0x002C
```

---

## 8. Configuration Guide

Configuration is split across two files:

| File | What it controls |
|------|-----------------|
| `drivers/dev_eep/include/dev_eep_cfg.h` | EEPROM dimensions, I2C address, page size, ACK polling timing |
| `services/svc_eep/include/svc_eep_cfg.h` | Service-level toggles (mirror, CRC, auto-flush, etc.) |

### 8.1 EEPROM Dimensions — in `dev_eep_cfg.h`

These MUST match your physical chip. Get them wrong and writes will silently corrupt data.

```c
#define DEV_EEP_MAIN_TOTAL_SIZE    (256U)
```
Total EEPROM capacity in bytes. Read from the datasheet. Common values:
- AT24C01: 128, AT24C02: 256, AT24C04: 512
- AT24C08: 1024, AT24C16: 2048, AT24C32: 4096
- AT24C64: 8192, AT24C128: 16384, AT24C256: 32768, AT24C512: 65536

```c
#define DEV_EEP_MAIN_PAGE_SIZE     (8U)
```
**This is the most critical setting.** The EEPROM's internal write buffer is this many bytes. If you try to write more than this in one I2C transaction, the excess bytes wrap around to the beginning of the same page — corrupting data silently. `dev_eep` uses this value to split writes safely.

Common values:
- AT24C01/02: **8 bytes**
- AT24C04/08/16: **16 bytes**
- AT24C32/64: **32 bytes**
- AT24C128/256: **64 bytes**
- AT24C512: **128 bytes**

```c
#define DEV_EEP_MAIN_PAGE_COUNT    (DEV_EEP_MAIN_TOTAL_SIZE / DEV_EEP_MAIN_PAGE_SIZE)
```
Auto-computed. For 256 bytes / 8-byte pages = 32 pages.

### 8.2 I2C Address — in `dev_eep.c`

The 7-bit I2C address is set in the static config table inside `drivers/dev_eep/src/dev_eep.c`:

```c
static const dev_eep_config_t s_configs[] = {
    {
        .eep_id        = DEV_EEP_MAIN,
        .i2c_bus       = DEV_I2C_BUS_EEPROM,   // from dev_i2c_cfg.h
        .i2c_addr      = ((dev_i2c_addr_t)0x50U),  // 7-bit, unshifted
        .total_size    = DEV_EEP_MAIN_TOTAL_SIZE,
        .page_size     = DEV_EEP_MAIN_PAGE_SIZE,
        .mem_addr_size = DEV_EEP_MEM_ADDR_SIZE_8BIT,
        .write_cycle_time_ms = 5U,
    },
};
```

**How to find your EEPROM's address:**
- 24Cxx base address is `0x50` (7-bit).
- A0/A1/A2 pins select upper bits: A0→VCC = `0x51`, A1→VCC = `0x52`, etc.
- Always use the **7-bit** address, not the 8-bit shifted address. Datasheet `0xA0` → code `0x50`.

### 8.3 Memory Address Width — in `dev_eep.c`

```c
.mem_addr_size = DEV_EEP_MEM_ADDR_SIZE_8BIT,   // ≤2KB chips
.mem_addr_size = DEV_EEP_MEM_ADDR_SIZE_16BIT,  // ≥4KB chips
```

| Chips | Address width | Enum |
|-------|--------------|------|
| AT24C01–AT24C16 | 8-bit (1 byte) | `DEV_EEP_MEM_ADDR_SIZE_8BIT` |
| AT24C32–AT24C512 | 16-bit (2 bytes) | `DEV_EEP_MEM_ADDR_SIZE_16BIT` |
| Rare >64KB chips | 24-bit | `DEV_EEP_MEM_ADDR_SIZE_24BIT` |

### 8.4 Service-Level Feature Toggles — in `svc_eep_cfg.h`

Every toggle is either `DEV_ON` (1) or `DEV_OFF` (0).

```c
#define SVC_EEP_CFG_RUNTIME_CHECK_ENABLED          DEV_ON
```
**ON:** All public API functions validate parameters (null pointers, address ranges, init state).
**OFF:** Parameter checks are skipped. Smaller/faster code but unsafe. Only disable in extreme flash-constrained situations.

```c
#define SVC_EEP_CFG_MIRROR_ENABLED                 DEV_ON
```
**ON:** Reads come from a RAM mirror. Writes go to the mirror, not to EEPROM. This is the core of the wear-reduction strategy.
**OFF:** Every read/write goes directly to EEPROM via `dev_eep`. Defeats wear reduction. Only for debugging.

```c
#define SVC_EEP_CFG_DIRTY_TRACKING_ENABLED         DEV_ON
```
**ON:** Each write marks affected pages in a dirty bitmap. Flush only writes dirty pages.
**OFF:** All pages are always considered dirty. Every flush writes the entire EEPROM. Defeats wear reduction.

```c
#define SVC_EEP_CFG_CRC_ENABLED                    DEV_ON
```
**ON:** CRC-16 is validated on init, and recalculated before each flush/shutdown. Catches data corruption from power loss during writes.
**OFF:** No CRC check. Saves 2 bytes of EEPROM and some CPU cycles. Only disable if you have another integrity mechanism.

```c
#define SVC_EEP_CFG_AUTO_READ_ALL_ON_INIT          DEV_ON
```
**ON:** `svc_eep_init()` reads the entire EEPROM into the RAM mirror automatically.
**OFF:** You must call `svc_eep_read_all()` manually after init. Use when you want to defer the I2C read (e.g., to meet a boot-time deadline).

```c
#define SVC_EEP_CFG_AUTO_FLUSH_ON_SHUTDOWN         DEV_ON
```
**ON:** `svc_eep_shutdown()` automatically flushes dirty pages before clearing state.
**OFF:** You must call `svc_eep_flush()` manually before `svc_eep_shutdown()`. If you forget, dirty data is lost.

```c
#define SVC_EEP_CFG_WRITE_ONLY_IF_CHANGED          DEV_ON
```
**ON:** If `svc_eep_write()` detects the new data is byte-for-byte identical to what's already in the mirror, it returns `DEV_OK` without marking anything dirty. Zero EEPROM wear for redundant writes.
**OFF:** Every write marks pages dirty, even if the data hasn't changed. Slightly simpler but wears the EEPROM unnecessarily.

```c
#define SVC_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC   DEV_ON
```
**ON:** If CRC or magic check fails at init, the driver loads default values into the mirror (magic, version, boot_count=0, empty device name) and marks all pages dirty. On the next flush, a valid EEPROM image is written. This handles the first-boot-with-blank-EEPROM case automatically.
**OFF:** CRC/magic failure returns `DEV_ERR_CRC`. Your application must handle it (e.g., by calling a manual "format EEPROM" routine).

### 8.5 Providing a Real `dev_delay_ms`

The driver needs a real millisecond delay. The default weak implementation is a no-op — **you must override it** in your application or board layer:

```c
/* STM32 + FreeRTOS example */
#include "dev_common.h"
#include "stm32h7xx_hal.h"

void dev_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);                         // bare-metal STM32
    // vTaskDelay(pdMS_TO_TICKS(ms));      // FreeRTOS
    // esp_rom_delay_us(ms * 1000);        // ESP32
}
```

Without this, ACK polling never actually waits (probe loop spins at CPU speed) and the fixed-delay fallback doesn't delay at all.

---

## 9. How to Add Your Own Fields

### Step 1 — Add field ID macros in `svc_eep_layout.h`

```c
/* After the built-in ones (IDs 0–4): */
#define SVC_EEP_FIELD_MY_SETTINGS          ((svc_eep_field_id_t)5U)
#define SVC_EEP_FIELD_CALIBRATION_DATA      ((svc_eep_field_id_t)6U)
#define SVC_EEP_FIELD_SERIAL_NUMBER         ((svc_eep_field_id_t)7U)
```

### Step 2 — Define offset and size

First free byte is `0x002C` (44 decimal). Chain your offsets:

```c
#define SVC_EEP_LAYOUT_MY_SETTINGS_OFFSET       ((svc_eep_addr_t)0x002CU)
#define SVC_EEP_LAYOUT_MY_SETTINGS_SIZE         ((svc_eep_size_t)64U)
/* → ends at 0x002C + 64 = 0x006C */

#define SVC_EEP_LAYOUT_CALIBRATION_OFFSET       ((svc_eep_addr_t)0x006CU)
#define SVC_EEP_LAYOUT_CALIBRATION_SIZE         ((svc_eep_size_t)128U)
/* → ends at 0x006C + 128 = 0x00EC */

#define SVC_EEP_LAYOUT_SERIAL_NUMBER_OFFSET     ((svc_eep_addr_t)0x00ECU)
#define SVC_EEP_LAYOUT_SERIAL_NUMBER_SIZE       ((svc_eep_size_t)16U)
/* → ends at 0x00EC + 16 = 0x00FC */
```

Double-check: `0x00FC ≤ 256` ✓ (all within the EEPROM).

### Step 3 — Add entries to the field table in `svc_eep_layout.c`

```c
const svc_eep_field_t g_svc_eep_fields[] = {
    /* ... existing fields ... */
    {
        SVC_EEP_FIELD_MY_SETTINGS,
        SVC_EEP_LAYOUT_MY_SETTINGS_OFFSET,
        SVC_EEP_LAYOUT_MY_SETTINGS_SIZE,
        "my_settings"          // debug name
    },
    {
        SVC_EEP_FIELD_CALIBRATION_DATA,
        SVC_EEP_LAYOUT_CALIBRATION_OFFSET,
        SVC_EEP_LAYOUT_CALIBRATION_SIZE,
        "calibration"
    },
    {
        SVC_EEP_FIELD_SERIAL_NUMBER,
        SVC_EEP_LAYOUT_SERIAL_NUMBER_OFFSET,
        SVC_EEP_LAYOUT_SERIAL_NUMBER_SIZE,
        "serial_number"
    },
};
```

### Step 4 — Validate

`svc_eep_init()` automatically checks for overlaps and out-of-bounds fields. If it returns `DEV_ERR_CONFIG`, one of your offsets or sizes is wrong.

---

## 10. Usage Examples

### 10.1 Minimal Boot Counter

```c
#include "svc_eep.h"
#include "dev_i2c.h"
#include "dev_common.h"
#include <stdio.h>

/* Required: provide a real delay */
void dev_delay_ms(uint32_t ms) { HAL_Delay(ms); }

int main(void)
{
    dev_err_t  err;
    uint32_t   boot_count;

    /* 1. Hardware + I2C init (vendor/board layer) */
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();

    /* 2. Initialize I2C bus driver */
    err = dev_i2c_init();
    if (err != DEV_OK) { printf("I2C: %d\n", err); return 1; }

    /* 3. Initialize EEPROM service (probes chip, reads mirror, validates CRC) */
    err = svc_eep_init();
    if (err != DEV_OK) {
        /* On first-ever boot with blank EEPROM, defaults are loaded and
         * init still returns DEV_OK (if LOAD_DEFAULTS_ON_INVALID_CRC is ON).
         * If it returns an error here, the EEPROM is genuinely unreachable. */
        printf("EEPROM: %d\n", err);
        return 1;
    }

    /* 4. Read boot count, increment, save to mirror */
    err = svc_eep_read_u32(SVC_EEP_FIELD_BOOT_COUNT, &boot_count);
    if (err != DEV_OK) { printf("read: %d\n", err); return 1; }

    boot_count++;
    err = svc_eep_write_u32(SVC_EEP_FIELD_BOOT_COUNT, boot_count);
    if (err != DEV_OK) { printf("write: %d\n", err); return 1; }

    printf("Boot #%lu\n", (unsigned long)boot_count);

    /* 5. Shutdown — flushes dirty pages (including boot_count) to EEPROM */
    svc_eep_shutdown();

    return 0;
}
```

### 10.2 Device Name (String Field)

```c
void set_device_name(const char *name)
{
    size_t len = strlen(name);
    if (len >= SVC_EEP_LAYOUT_DEVICE_NAME_SIZE) return;  // too long

    svc_eep_write_field(SVC_EEP_FIELD_DEVICE_NAME,
                        name,
                        (svc_eep_size_t)(len + 1U));  // include null terminator
}

void get_device_name(char *buf, size_t buf_size)
{
    svc_eep_read_field(SVC_EEP_FIELD_DEVICE_NAME, buf,
                       (svc_eep_size_t)buf_size);
    buf[buf_size - 1U] = '\0';  // safety: always null-terminate
}
```

### 10.3 Settings Struct (Multi-Byte Field)

```c
typedef struct {
    uint8_t  brightness;      // 0–100
    uint8_t  volume;          // 0–100
    uint16_t auto_off_min;    // auto power-off after N minutes
    uint32_t baud_rate;       // UART baud rate
} device_settings_t;

/* Compile-time size check */
_Static_assert(sizeof(device_settings_t) <= SVC_EEP_LAYOUT_MY_SETTINGS_SIZE,
               "Settings struct too large for EEPROM field");

dev_err_t load_settings(device_settings_t *s)
{
    return svc_eep_read_field(SVC_EEP_FIELD_MY_SETTINGS,
                              s, sizeof(device_settings_t));
}

dev_err_t save_settings(const device_settings_t *s)
{
    return svc_eep_write_field(SVC_EEP_FIELD_MY_SETTINGS,
                               s, sizeof(device_settings_t));
}
```

### 10.4 Manual Flush Control (Write-Intensive Applications)

```c
void update_sensor_log(uint32_t value)
{
    /* Write to mirror only — fast, zero I2C traffic */
    svc_eep_write_u32(SVC_EEP_FIELD_SENSOR_LATEST, value);

    /* Only flush every 100 updates to reduce EEPROM wear */
    static uint32_t update_count = 0;
    update_count++;

    if ((update_count % 100U) == 0U) {
        svc_eep_flush();   // writes only dirty pages to EEPROM
    }
}

/* Always flush remaining changes before shutdown */
void on_shutdown(void)
{
    if (svc_eep_is_dirty(SVC_EEP_MAIN)) {
        uint16_t dirty = svc_eep_get_dirty_page_count(SVC_EEP_MAIN);
        printf("Flushing %u dirty pages...\n", dirty);
        svc_eep_flush();
    }
    svc_eep_shutdown();   // also flushes if AUTO_FLUSH_ON_SHUTDOWN is ON
}
```

### 10.5 Raw Mirror Access (Advanced)

```c
/* Read/write raw bytes to the RAM mirror at a specific address.
 * Useful for bulk operations or accessing data outside the field table. */
uint8_t buf[16];
svc_eep_read(SVC_EEP_MAIN, 0x0080U, buf, 16U);   // reads from mirror
svc_eep_write(SVC_EEP_MAIN, 0x0080U, buf, 16U);  // writes to mirror, marks dirty

/* Check if anything needs flushing */
if (svc_eep_is_dirty(SVC_EEP_MAIN)) {
    svc_eep_flush();
}
```

### 10.6 Error Handling — Every Return Code

```c
dev_err_t err = svc_eep_write_u32(SVC_EEP_FIELD_BOOT_COUNT, new_count);
switch (err) {
case DEV_OK:
    break;  // success — data in mirror, pages marked dirty
case DEV_ERR_NOT_INITIALIZED:
    // Forgot to call svc_eep_init(), or called after svc_eep_shutdown()
    break;
case DEV_ERR_INVALID_ARG:
    // Invalid field_id, or length > field size, or zero-length write
    break;
case DEV_ERR_NULL_PTR:
    // data pointer is NULL
    break;
case DEV_ERR_OUT_OF_RANGE:
    // address + length exceeds mirror size
    break;
case DEV_ERR_CONFIG:
    // Field layout overlap or device config mismatch (caught at init)
    break;
case DEV_ERR_CRC:
    // CRC check failed at init (only if LOAD_DEFAULTS_ON_INVALID_CRC is OFF)
    break;
default:
    // Unexpected — log and investigate
    break;
}
```

---

## 11. API Reference

### 11.1 Lifecycle

| Function | Description |
|----------|-------------|
| `svc_eep_init()` | Probe chip via dev_eep, read all into mirror, validate magic/CRC, load defaults if needed |
| `svc_eep_shutdown()` | Update CRC, flush all dirty pages, deinit dev_eep, clear state |
| `svc_eep_deinit()` | Clear mirror and dirty map without flushing (abandon changes) |
| `svc_eep_is_initialized()` | Returns `true` if init was called and not yet shutdown |

### 11.2 Field-Based Access

| Function | Description |
|----------|-------------|
| `svc_eep_read_field(id, data, len)` | Read from a named field in the mirror. `len` can be ≤ field size |
| `svc_eep_write_field(id, data, len)` | Write to a named field in the mirror. Marks pages dirty |
| `svc_eep_get_field_info(id, &field)` | Get pointer to field descriptor (offset, size, name) |

### 11.3 Typed Helpers

```c
dev_err_t svc_eep_read_u8 (svc_eep_field_id_t id, uint8_t  *value);
dev_err_t svc_eep_write_u8(svc_eep_field_id_t id, uint8_t   value);

dev_err_t svc_eep_read_u16 (svc_eep_field_id_t id, uint16_t *value);
dev_err_t svc_eep_write_u16(svc_eep_field_id_t id, uint16_t  value);

dev_err_t svc_eep_read_u32 (svc_eep_field_id_t id, uint32_t *value);
dev_err_t svc_eep_write_u32(svc_eep_field_id_t id, uint32_t  value);
```

These are thin wrappers — they pass `sizeof(type)` as the length. The field must be at least as large as the type.

### 11.4 Raw Mirror Access

| Function | Description |
|----------|-------------|
| `svc_eep_read(eep_id, addr, data, len)` | Copy from mirror at `addr` into `data`. No I2C traffic |
| `svc_eep_write(eep_id, addr, data, len)` | Copy `data` into mirror at `addr`. Marks pages dirty. No I2C traffic |

### 11.5 Flush

```c
dev_err_t svc_eep_flush(void);
```
Write all dirty pages to physical EEPROM via `dev_eep_write()`. Each dirty page is written individually. If a write fails, the dirty bit is preserved (not cleared) so a retry will re-write that page. Returns `DEV_OK` if nothing was dirty.

### 11.6 Dirty State

| Function | Description |
|----------|-------------|
| `svc_eep_is_dirty(eep_id)` | `true` if any page has unflushed changes |
| `svc_eep_mark_dirty(eep_id, addr, len)` | Manually mark pages dirty (normally automatic) |
| `svc_eep_clear_dirty(eep_id)` | Clear all dirty bits without writing to EEPROM (discard changes) |
| `svc_eep_get_dirty_page_count(eep_id)` | How many pages are dirty (for progress reporting) |

---

## 12. Full Application Example (STM32 + FreeRTOS)

```c
#include "svc_eep.h"
#include "dev_i2c.h"
#include "dev_common.h"
#include "cmsis_os.h"       // FreeRTOS via CubeMX
#include <stdio.h>
#include <string.h>

/* ── Real delay (FreeRTOS) ── */
void dev_delay_ms(uint32_t ms)
{
    osDelay(ms);
}

/* ── Application settings stored in EEPROM ── */
typedef struct {
    uint8_t  brightness;
    uint8_t  volume;
    uint16_t auto_off_min;
} app_settings_t;

_Static_assert(sizeof(app_settings_t) <= SVC_EEP_LAYOUT_MY_SETTINGS_SIZE,
               "Settings too large");

/* ── Boot task ── */
void boot_task(void *arg)
{
    (void)arg;
    dev_err_t  err;
    uint32_t   boot_count;
    char       name[SVC_EEP_LAYOUT_DEVICE_NAME_SIZE];

    /* 1. Init I2C bus */
    err = dev_i2c_init();
    configASSERT(err == DEV_OK);

    /* 2. Init EEPROM service */
    err = svc_eep_init();
    configASSERT(err == DEV_OK);

    /* 3. Boot count */
    svc_eep_read_u32(SVC_EEP_FIELD_BOOT_COUNT, &boot_count);
    boot_count++;
    svc_eep_write_u32(SVC_EEP_FIELD_BOOT_COUNT, boot_count);
    printf("[BOOT] #%lu\n", (unsigned long)boot_count);

    /* 4. Device name (set on first boot) */
    svc_eep_read_field(SVC_EEP_FIELD_DEVICE_NAME, name, sizeof(name));
    name[sizeof(name) - 1U] = '\0';
    if (name[0] == '\0') {
        svc_eep_write_field(SVC_EEP_FIELD_DEVICE_NAME,
                            "Controller-A",
                            (svc_eep_size_t)(strlen("Controller-A") + 1U));
        printf("[BOOT] First boot — name set to Controller-A\n");
    } else {
        printf("[BOOT] Device: %s\n", name);
    }

    /* 5. Flush persisted data now (so it survives a crash later) */
    svc_eep_flush();

    /* 6. Start main application */
    start_main_tasks();
    vTaskDelete(NULL);
}

/* ── Shutdown hook (called before reset/power-off) ── */
void shutdown_hook(void)
{
    printf("[SHUTDOWN] Saving EEPROM...\n");
    svc_eep_shutdown();   // updates CRC, flushes dirty pages, deinits
    printf("[SHUTDOWN] Done.\n");
}
```

---

## 13. Porting to a New Board

1. **Wire the EEPROM.** SDA, SCL, VCC, GND. Set A0/A1/A2 address pins. Add 4.7kΩ pull-ups on SDA/SCL if not already present.
2. **Configure I2C.** In `dev_i2c_cfg.h`, define `DEV_I2C_BUS_EEPROM` to map to the correct hardware I2C peripheral.
3. **Set EEPROM parameters.** In `dev_eep_cfg.h`: `DEV_EEP_MAIN_TOTAL_SIZE`, `DEV_EEP_MAIN_PAGE_SIZE`.
4. **Set I2C address and address width.** In `dev_eep.c` in the `s_configs[]` table.
5. **Provide `dev_delay_ms()`.** Real implementation (HAL tick, RTOS delay, or busy-wait).
6. **Build and test.** Run host tests first, then flash to hardware and verify `svc_eep_init()` returns `DEV_OK`.

---

## 14. Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `DEV_ERR_NOT_INITIALIZED` | Forgot `svc_eep_init()` or `dev_i2c_init()` | Call them in order: `dev_i2c_init()` → `svc_eep_init()` |
| `DEV_ERR_ALREADY_INITIALIZED` | Called `svc_eep_init()` twice | Call `svc_eep_shutdown()` before re-init |
| `DEV_ERR_CONFIG` on init | Field layout overlap or out-of-bounds | Check offsets/sizes in `svc_eep_layout.h` |
| `DEV_ERR_CRC` on init | EEPROM blank or data corrupt | Enable `LOAD_DEFAULTS_ON_INVALID_CRC` or handle manually |
| `DEV_ERR_TIMEOUT` on init/flush | EEPROM not responding | Check wiring, pull-ups, I2C address, bus speed |
| `DEV_ERR_NO_ACK` | Wrong I2C address or device not powered | Verify 7-bit address. Check VCC/GND. |
| Data lost after power cycle | `svc_eep_shutdown()` not called | Always call shutdown before power-off |
| EEPROM wears out quickly | Flushing too often | Batch writes, flush infrequently. Check `WRITE_ONLY_IF_CHANGED` is `DEV_ON` |
| `DEV_ERR_OUT_OF_RANGE` | Address + length exceeds EEPROM size | Check `DEV_EEP_MAIN_TOTAL_SIZE` matches chip |

## 15. Design Rationale

**Why a RAM mirror?** Eliminates all I2C traffic during normal operation. Reads are instant (memcpy). Trade-off: RAM usage (256 bytes for AT24C02 — negligible).

**Why not write-through?** EEPROM has limited write endurance (~1M cycles per byte). Writing immediately on every change would wear out frequently-updated fields. Batching writes extends EEPROM life dramatically.

**Why CRC-16 not CRC-32?** CRC-16 uses 2 bytes vs 4. For small EEPROMs, saving 2 bytes matters. CRC-16 catches all single-bit, double-bit, and burst errors up to 16 bits — adequate for EEPROM integrity.

**Why ACK polling (in dev_eep) not fixed delays?** The EEPROM ignores I2C traffic during its internal write cycle. Polling waits exactly as long as needed — no guesswork, no wasted time.

## 16. Source Files

| File | Purpose |
|------|---------|
| `services/svc_eep/include/svc_eep.h` | Public API |
| `services/svc_eep/include/svc_eep_types.h` | `svc_eep_device_t`, `svc_eep_field_t` |
| `services/svc_eep/include/svc_eep_cfg.h` | Service-level feature toggles |
| `services/svc_eep/include/svc_eep_layout.h` | Field IDs, offsets, sizes, magic/version constants |
| `services/svc_eep/src/svc_eep.c` | Full implementation (mirror, dirty tracking, CRC, field ops) |
| `services/svc_eep/src/svc_eep_layout.c` | Field descriptor table |
| `drivers/dev_eep/include/dev_eep.h` | Device-level EEPROM driver API |
| `drivers/dev_eep/include/dev_eep_cfg.h` | Device dimensions, I2C addr, page size, timing |
| `drivers/dev_eep/src/dev_eep.c` | Device-level implementation (I2C, page splitting, ACK polling) |
| `tests/dev_eep/test_eep.c` | 24 host-based unit tests |
