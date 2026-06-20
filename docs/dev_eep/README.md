# dev_eep — I2C EEPROM Driver

## 1. What Is It?

`dev_eep` is a safe, wear-reducing service layer for external I2C EEPROM chips (24Cxx, 25Cxx, and similar). Instead of reading and writing the physical EEPROM on every access, it keeps a **RAM mirror** of the entire EEPROM content. Your application reads and writes the mirror; only when you explicitly flush (or shut down) are changed bytes written back to the physical chip.

This protects EEPROM lifetime — each byte in an EEPROM has a finite number of write cycles (typically 100,000–1,000,000). Writing only changed pages, and only when you say so, dramatically reduces wear.

### Key features

| Feature | What it does |
|---------|-------------|
| RAM mirror | All reads come from RAM; no I2C traffic after init |
| Dirty-page tracking | Only pages with changed data are written to EEPROM |
| Compare-before-write | Identical data is silently skipped — zero wear |
| CRC-16 integrity | Optional CRC check on init to detect corruption |
| Magic + version | Detects blank or corrupted EEPROM at boot |
| Field-based access | Read/write by logical name, not raw offset |
| Page-safe writes | Never writes across an EEPROM page boundary |
| ACK polling | Waits for EEPROM write cycle to complete before next write |
| Multi-size addressing | Supports 8-bit, 16-bit, 24-bit, and 32-bit EEPROM address widths |

---

## 2. How It Works

### 2.1 The Mirror Model

```
                  ┌─────────────┐
                  │  Application │
                  └──────┬──────┘
                         │ dev_eep_read / dev_eep_write (to/from RAM only!)
                  ┌──────▼──────┐
                  │  RAM mirror │  1024 bytes (or whatever your EEPROM size is)
                  │  + dirty map│  8 bytes (1 bit per page)
                  └──────┬──────┘
                         │ dev_eep_flush / dev_eep_shutdown (writes dirty pages)
                  ┌──────▼──────┐
                  │   dev_i2c   │
                  └──────┬──────┘
                         │ I2C bus
                  ┌──────▼──────┐
                  │ EEPROM chip │  Physical 24C08 / 24C16 / etc.
                  └─────────────┘
```

### 2.2 Lifecycle

```
POWER ON
   │
   ▼
dev_i2c_init()          ← initialize the I2C bus
   │
   ▼
dev_eep_init()          ← reads entire EEPROM → RAM mirror
   │                     ← validates magic, version, CRC
   │                     ← loads defaults if EEPROM is blank/corrupt
   │
   ▼
┌──────────────────────────────────────────┐
│  APPLICATION RUNS                        │
│                                          │
│  dev_eep_read_xxx()   ← reads from RAM   │
│  dev_eep_write_xxx()  ← writes to RAM    │
│                         marks pages dirty │
│                                          │
│  dev_eep_flush()      ← writes dirty     │
│                         pages to EEPROM   │
└──────────────────────────────────────────┘
   │
   ▼
dev_eep_shutdown()      ← updates CRC, flushes dirty pages, clears state
   │
   ▼
POWER OFF
```

### 2.3 Write Flow (Inside `dev_eep_write`)

```
Application calls dev_eep_write(field, data, len)
   │
   ▼
Is driver initialized? ──NO──→ DEV_ERR_NOT_INITIALIZED
   │YES
   ▼
Is address valid? ──NO──→ DEV_ERR_OUT_OF_RANGE
   │YES
   ▼
Is new data == existing mirror data? ──YES──→ DEV_OK (no-op, no wear)
   │NO
   ▼
memcpy(data → mirror)
   │
   ▼
Mark affected pages dirty in dirty_map
   │
   ▼
DEV_OK  (physical EEPROM NOT written yet)
```

### 2.4 Flush Flow (Inside `dev_eep_flush`)

```
dev_eep_flush()
   │
   ▼
For each page (0 .. page_count-1):
   │
   ▼
Is page dirty? ──NO──→ skip to next page
   │YES
   ▼
Write page to physical EEPROM via I2C
   │
   ▼
Wait for EEPROM write cycle (ACK polling or delay)
   │
   ▼
Write succeeded? ──NO──→ return error (dirty bit stays set)
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

## 3. EEPROM Memory Layout

### 3.1 Visual Map

The default layout uses the first 44 bytes of EEPROM for metadata; the remaining 980 bytes are available for your application fields.

```
Offset  Size  Field              Description
──────  ────  ─────────────────  ───────────────────────────────────────
0x0000   4    MAGIC              0x44564550 ("DEVP") — proves EEPROM is valid
0x0004   2    VERSION            Layout version (currently 1)
0x0006   2    CRC                CRC-16 over bytes 0x0000–0x0005
0x0008   4    BOOT_COUNT         Incremented on each boot (your app manages this)
0x000C  32    DEVICE_NAME        Null-terminated device name string
──────  ────  ─────────────────  ───────────────────────────────────────
0x002C 980    (free space)       Add your own fields here
──────  ────  ─────────────────  ───────────────────────────────────────
```

### 3.2 Built-in Fields

| Field ID macro | C type | Size | Purpose |
|---------------|--------|------|---------|
| `DEV_EEP_FIELD_MAGIC` | `uint32_t` | 4 | Magic number `0x44564550` — proves EEPROM was programmed by this firmware |
| `DEV_EEP_FIELD_VERSION` | `uint16_t` | 2 | Layout version — change when you reorganize fields |
| `DEV_EEP_FIELD_CRC` | `uint16_t` | 2 | CRC-16 over magic + version (excludes CRC field itself) |
| `DEV_EEP_FIELD_BOOT_COUNT` | `uint32_t` | 4 | Boot counter — application increments on each power-on |
| `DEV_EEP_FIELD_DEVICE_NAME` | `char[32]` | 32 | Human-readable name (e.g., "Controller-A") |

### 3.3 CRC Coverage

```
CRC covers:  bytes 0x0000 through 0x0005  (magic + version)
CRC stored:  bytes 0x0006 through 0x0007  (excluded from calculation)
```

The CRC is recalculated and written just before each flush/shutdown.

---

## 4. Configuration Guide

All configuration is in `drivers/dev_eep/include/dev_eep_cfg.h`. You MUST edit this file before compiling — the defaults are for a 24C08 (1024-byte, 16-byte page) EEPROM.

### 4.1 EEPROM Dimensions (REQUIRED — match your chip's datasheet)

```c
// Total EEPROM size in bytes. Check your chip: 24C01=128, 24C02=256,
// 24C04=512, 24C08=1024, 24C16=2048, 24C32=4096, 24C64=8192, etc.
#define DEV_EEP_MAIN_TOTAL_SIZE    (1024U)

// Page size in bytes. This is CRITICAL — get it wrong and writes will fail.
// Common values: 24C01–24C16 = 16, 24C32–24C64 = 32, 24C128–24C512 = 64 or 128
#define DEV_EEP_MAIN_PAGE_SIZE     (16U)
```

### 4.2 I2C Address (match your hardware wiring)

The I2C address is set in the device table inside `dev_eep.c`:

```c
// In dev_eep.c, the static device table:
static const dev_eep_device_t s_devices[] = {
    {
        .eep_id        = DEV_EEP_MAIN,
        .i2c_bus       = DEV_I2C_BUS_EEPROM,   // defined in dev_i2c_cfg.h
        .i2c_addr      = 0x50U,                 // 7-bit address (unshifted)
        .total_size    = DEV_EEP_MAIN_TOTAL_SIZE,
        .page_size     = DEV_EEP_MAIN_PAGE_SIZE,
        .mem_addr_size = DEV_EEP_MEM_ADDR_SIZE_16BIT,
        // ...
    },
};
```

**How to find your EEPROM's I2C address:**
- 24Cxx series: base address is `0x50` (7-bit). The A0/A1/A2 pins select the upper bits.
  - All address pins to GND → `0x50`
  - A0 to VCC → `0x51`, A1 to VCC → `0x52`, etc.
- Check your schematic. The address in the code is the **7-bit** address, NOT the 8-bit shifted address. If your datasheet says `0xA0`, the 7-bit address is `0x50`.

### 4.3 Memory Address Size (match your chip)

```c
// Which EEPROMs use which address size:
//   8-bit:  24C01–24C16  (up to 2048 bytes — single address byte)
//  16-bit:  24C32–24C512 (4096–65536 bytes — two address bytes)
//  24-bit:  Rare, very large EEPROMs (>64KB)
//  32-bit:  Extremely rare
//
// Set in the device table, not in cfg.h:
//   .mem_addr_size = DEV_EEP_MEM_ADDR_SIZE_16BIT,
```

### 4.4 Feature Toggles

```c
// Maximum number of EEPROM chips on the board.
// Increase if you have multiple EEPROMs.
#define DEV_EEP_CFG_MAX_DEVICES    (1U)

// CRC-16 integrity check. Strongly recommended.
// Disable only if you need the 2 bytes for something else.
#define DEV_EEP_CFG_CRC_ENABLED    DEV_ON   // or DEV_OFF

// Auto-read all EEPROM data into RAM mirror during init.
// Almost always want this ON.
#define DEV_EEP_CFG_AUTO_READ_ALL_ON_INIT       DEV_ON

// Auto-flush dirty pages during shutdown.
// Turn OFF if you want manual control over when flushing happens.
#define DEV_EEP_CFG_AUTO_FLUSH_ON_SHUTDOWN      DEV_ON

// Skip writes when new data == existing mirror data.
// This is the core wear-reduction feature. Keep ON.
#define DEV_EEP_CFG_WRITE_ONLY_IF_CHANGED       DEV_ON

// When CRC/magic check fails at init, load default values into the mirror
// (instead of returning DEV_ERR_CRC). Useful for first-boot with blank EEPROM.
#define DEV_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC DEV_ON

// ACK polling: after a write, poll the EEPROM until it ACKs (write done).
// More reliable than fixed delays. Keep ON unless you have a specific reason.
#define DEV_EEP_CFG_ACK_POLLING_ENABLED          DEV_ON
```

### 4.5 Timing

```c
// EEPROM write cycle time in milliseconds (from datasheet). Typically 5ms.
// Used only as fallback when ACK polling is disabled.
#define DEV_EEP_CFG_DEFAULT_WRITE_CYCLE_TIME_MS  (5U)

// How long to poll for ACK after a write before giving up.
// Default 10ms is enough for most EEPROMs (max write cycle is 5–10ms).
#define DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS          (10U)
```

### 4.6 Providing a Real `dev_delay_ms`

The driver needs a millisecond delay for write-cycle timing. Add this to your board file:

```c
// In your board's main.c or a board-support file:
#include "dev_common.h"
#include "stm32h7xx_hal.h"   // vendor HAL — allowed in application/board layer

void dev_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);   // STM32
    // or: vTaskDelay(pdMS_TO_TICKS(ms));  // FreeRTOS
    // or: esp_rom_delay_us(ms * 1000);    // ESP32
}
```

Without this, the weak default does nothing, and ACK-polling-disabled mode will not wait for write completion (causing data corruption).

---

## 5. How to Add Your Own Fields

### Step 1: Add field ID macros in `dev_eep_layout.h`

```c
// Add your field IDs after the built-in ones:
#define DEV_EEP_FIELD_MY_SETTINGS          ((dev_eep_field_id_t)5U)
#define DEV_EEP_FIELD_CALIBRATION_DATA      ((dev_eep_field_id_t)6U)
#define DEV_EEP_FIELD_SERIAL_NUMBER         ((dev_eep_field_id_t)7U)
```

### Step 2: Define offset and size

Pick offsets that don't overlap with existing fields. The first free byte is at `0x002C`:

```c
#define DEV_EEP_LAYOUT_MY_SETTINGS_OFFSET       ((dev_eep_addr_t)0x002CU)
#define DEV_EEP_LAYOUT_MY_SETTINGS_SIZE         ((dev_eep_size_t)64U)

#define DEV_EEP_LAYOUT_CALIBRATION_OFFSET       ((dev_eep_addr_t)0x006CU)
#define DEV_EEP_LAYOUT_CALIBRATION_SIZE         ((dev_eep_size_t)128U)

#define DEV_EEP_LAYOUT_SERIAL_NUMBER_OFFSET     ((dev_eep_addr_t)0x00ECU)
#define DEV_EEP_LAYOUT_SERIAL_NUMBER_SIZE       ((dev_eep_size_t)16U)
```

Check: `0x002C + 64 = 0x006C` ✓, `0x006C + 128 = 0x00EC` ✓, `0x00EC + 16 = 0x00FC` ✓ (all under 1024).

### Step 3: Add entries to the field table in `dev_eep_layout.c`

```c
const dev_eep_field_t g_dev_eep_fields[] = {
    // ... existing fields ...
    {
        DEV_EEP_FIELD_MY_SETTINGS,
        DEV_EEP_LAYOUT_MY_SETTINGS_OFFSET,
        DEV_EEP_LAYOUT_MY_SETTINGS_SIZE,
        "my_settings"
    },
    {
        DEV_EEP_FIELD_CALIBRATION_DATA,
        DEV_EEP_LAYOUT_CALIBRATION_OFFSET,
        DEV_EEP_LAYOUT_CALIBRATION_SIZE,
        "calibration"
    },
    {
        DEV_EEP_FIELD_SERIAL_NUMBER,
        DEV_EEP_LAYOUT_SERIAL_NUMBER_OFFSET,
        DEV_EEP_LAYOUT_SERIAL_NUMBER_SIZE,
        "serial_number"
    },
};
```

The `name` field is for debugging only — it appears in error logs.

### Step 4: Validate

The `dev_eep_init()` function automatically validates the field table at init — it will return `DEV_ERR_CONFIG` if any fields overlap or go out of bounds. No manual checking needed.

---

## 6. Usage Examples

### 6.1 Minimal Boot Counter

The simplest real-world use: count how many times the device has powered on.

```c
#include "dev_eep.h"
#include "dev_i2c.h"

int main(void)
{
    dev_err_t  err;
    uint32_t   boot_count;

    // 1. Initialize I2C, then EEPROM
    dev_i2c_init();
    err = dev_eep_init();
    if (err != DEV_OK) {
        // On first boot with blank EEPROM, defaults are loaded
        // (if DEV_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC is ON)
        // boot_count starts at 0
    }

    // 2. Read current boot count
    err = dev_eep_read_u32(DEV_EEP_FIELD_BOOT_COUNT, &boot_count);
    if (err != DEV_OK) {
        // handle error
    }

    // 3. Increment and save
    boot_count++;
    err = dev_eep_write_u32(DEV_EEP_FIELD_BOOT_COUNT, boot_count);
    if (err != DEV_OK) {
        // handle error
    }

    // 4. Shutdown — writes dirty pages to EEPROM
    dev_eep_shutdown();

    printf("Boot count: %lu\n", (unsigned long)boot_count);
}
```

### 6.2 Device Name (String Field)

Store a human-readable device identifier.

```c
void set_device_name(const char *name)
{
    size_t len = strlen(name);

    if (len >= DEV_EEP_LAYOUT_DEVICE_NAME_SIZE) {
        // Name too long — truncation would lose the null terminator
        return;
    }

    // Write the string + null terminator. Only writes up to the field size.
    dev_eep_write_field(DEV_EEP_FIELD_DEVICE_NAME,
                        name,
                        (dev_eep_size_t)(len + 1U));
}

void get_device_name(char *buf, size_t buf_size)
{
    dev_eep_read_field(DEV_EEP_FIELD_DEVICE_NAME,
                       buf,
                       (dev_eep_size_t)buf_size);
    // Ensure null termination even if EEPROM data is corrupt
    buf[buf_size - 1U] = '\0';
}
```

### 6.3 Settings Struct (Multi-Byte Field)

Store a configuration struct as a single EEPROM field.

```c
// Define your settings struct (in your application code)
typedef struct {
    uint8_t  brightness;    // 0–100
    uint8_t  volume;        // 0–100
    uint16_t auto_off_min;  // auto power-off after N minutes
    uint32_t baud_rate;     // UART baud rate
} device_settings_t;

// Compile-time check that it fits in your EEPROM field
_Static_assert(sizeof(device_settings_t) <= DEV_EEP_LAYOUT_MY_SETTINGS_SIZE,
               "Settings struct too large for EEPROM field");

dev_err_t load_settings(device_settings_t *settings)
{
    return dev_eep_read_field(DEV_EEP_FIELD_MY_SETTINGS,
                              settings,
                              sizeof(device_settings_t));
}

dev_err_t save_settings(const device_settings_t *settings)
{
    return dev_eep_write_field(DEV_EEP_FIELD_MY_SETTINGS,
                               settings,
                               sizeof(device_settings_t));
}

// Usage:
device_settings_t settings;
if (load_settings(&settings) == DEV_OK) {
    // Use settings...
}
```

### 6.4 Manual Flush (Write-Intensive Applications)

If your application writes frequently, don't flush on every write. Batch changes and flush periodically or at safe points.

```c
void update_sensor_log(uint32_t value)
{
    // Write to mirror only — fast, no I2C
    dev_eep_write_u32(DEV_EEP_FIELD_SENSOR_LATEST, value);

    // Only flush every 100 updates to reduce EEPROM wear
    static uint32_t update_count = 0;
    update_count++;

    if ((update_count % 100U) == 0U) {
        dev_eep_flush(DEV_EEP_MAIN);
    }
}

// Always flush remaining changes before shutdown:
void on_shutdown(void)
{
    if (dev_eep_is_dirty(DEV_EEP_MAIN)) {
        uint16_t dirty = dev_eep_get_dirty_page_count(DEV_EEP_MAIN);
        printf("Flushing %u dirty pages...\n", dirty);
        dev_eep_flush(DEV_EEP_MAIN);
    }
    dev_eep_shutdown();
}
```

### 6.5 Error Handling Pattern

Every `dev_eep` API returns `dev_err_t`. Always check.

```c
dev_err_t err;

err = dev_eep_write_u32(DEV_EEP_FIELD_BOOT_COUNT, new_count);
switch (err) {
case DEV_OK:
    break;  // success
case DEV_ERR_NOT_INITIALIZED:
    // Forgot to call dev_eep_init()
    break;
case DEV_ERR_INVALID_ARG:
    // Invalid field ID — check your DEV_EEP_FIELD_* constant
    break;
case DEV_ERR_OUT_OF_RANGE:
    // Data larger than the field — check your field size definitions
    break;
case DEV_ERR_CRC:
    // EEPROM data is corrupt — consider loading defaults
    break;
case DEV_ERR_TIMEOUT:
    // EEPROM not responding — check I2C wiring and address
    break;
default:
    // Unexpected error — log and investigate
    break;
}
```

### 6.6 Full Application Example

```c
#include "dev_eep.h"
#include "dev_i2c.h"
#include "dev_common.h"
#include <stdio.h>
#include <string.h>

// ── Provide real delay (STM32 example) ──
void dev_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

// ── Application ──
int main(void)
{
    dev_err_t  err;
    uint32_t   boot_count;
    char       name[DEV_EEP_LAYOUT_DEVICE_NAME_SIZE];

    // ── Hardware init ──
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();   // STM32 CubeMX-generated I2C init

    // ── Driver init ──
    err = dev_i2c_init();
    if (err != DEV_OK) {
        printf("I2C init failed: %d\n", err);
        return 1;
    }

    err = dev_eep_init();
    if (err != DEV_OK && err != DEV_ERR_CRC) {
        // DEV_ERR_CRC is OK on first boot (blank EEPROM → defaults loaded)
        printf("EEPROM init failed: %d\n", err);
        return 1;
    }

    // ── Boot count ──
    dev_eep_read_u32(DEV_EEP_FIELD_BOOT_COUNT, &boot_count);
    boot_count++;
    dev_eep_write_u32(DEV_EEP_FIELD_BOOT_COUNT, boot_count);
    printf("Boot #%lu\n", (unsigned long)boot_count);

    // ── Set device name on first boot ──
    dev_eep_read_field(DEV_EEP_FIELD_DEVICE_NAME, name, sizeof(name));
    name[sizeof(name) - 1U] = '\0';

    if (name[0] == '\0') {
        // First boot — set a default name
        dev_eep_write_field(DEV_EEP_FIELD_DEVICE_NAME,
                            "MyDevice",
                            (dev_eep_size_t)(strlen("MyDevice") + 1U));
        printf("Device name set to: MyDevice\n");
    } else {
        printf("Device name: %s\n", name);
    }

    // ── Flush changes to EEPROM now (optional — shutdown does it too) ──
    if (dev_eep_is_dirty(DEV_EEP_MAIN)) {
        printf("Flushing %u dirty pages...\n",
               dev_eep_get_dirty_page_count(DEV_EEP_MAIN));
        dev_eep_flush(DEV_EEP_MAIN);
    }

    // ── Application main loop ──
    while (1) {
        // Your application logic here...
    }

    // ── Clean shutdown (reached on reset/power-down) ──
    dev_eep_shutdown();
    return 0;
}
```

---

## 7. API Reference

### 7.1 Lifecycle

```c
dev_err_t dev_eep_init(void);
```
Initialize the driver. Reads all EEPROM data into the RAM mirror, validates the magic number and CRC (if enabled), and loads defaults if the EEPROM is blank or corrupt. Returns `DEV_ERR_ALREADY_INITIALIZED` if called twice without a `dev_eep_shutdown()` or `dev_eep_deinit()` in between.

```c
dev_err_t dev_eep_shutdown(void);
```
Update CRC, flush all dirty pages to physical EEPROM, and clear the initialized state. Returns `DEV_ERR_NOT_INITIALIZED` if the driver wasn't initialized. If any flush fails, the state is NOT cleared (so you can retry). Safe to call even if nothing is dirty — it's a no-op flush.

```c
dev_err_t dev_eep_deinit(void);
```
Clear the RAM mirror and dirty map without writing anything to EEPROM. Use this if you want to abandon changes and reset to a clean state.

```c
bool dev_eep_is_initialized(void);
```
Returns `true` if `dev_eep_init()` has been called (and not yet shut down).

### 7.2 Raw Read/Write (operate on raw EEPROM addresses)

```c
dev_err_t dev_eep_read(dev_eep_id_t eep_id, dev_eep_addr_t addr,
                       uint8_t *data, dev_eep_size_t length);
```
Copy `length` bytes from the RAM mirror starting at `addr` into `data`. Reads from RAM, not from the physical EEPROM.

```c
dev_err_t dev_eep_write(dev_eep_id_t eep_id, dev_eep_addr_t addr,
                        const uint8_t *data, dev_eep_size_t length);
```
Copy `data` into the RAM mirror at `addr`. Marks affected pages dirty. Does NOT write to physical EEPROM. If the new data is identical to what's already in the mirror, this is a no-op (returns `DEV_OK` without marking anything dirty).

```c
dev_err_t dev_eep_read_all(dev_eep_id_t eep_id);
```
Re-read the entire EEPROM into the RAM mirror. Clears the dirty map. Use this to discard pending changes and sync with the physical EEPROM.

```c
dev_err_t dev_eep_write_all(dev_eep_id_t eep_id);
```
Write the ENTIRE mirror to physical EEPROM, page by page, regardless of dirty state. This is a full rewrite — use sparingly. For normal use, prefer `dev_eep_flush()`.

```c
dev_err_t dev_eep_flush(dev_eep_id_t eep_id);
```
Write only dirty pages to physical EEPROM. Each page is written individually with write-cycle waits. If a page write fails, the dirty bit is preserved (not cleared) so it can be retried. Returns the first error encountered.

### 7.3 Field-Based Access (operate by field ID)

```c
dev_err_t dev_eep_read_field(dev_eep_field_id_t field_id,
                             void *data, dev_eep_size_t length);
```
Read from a named field. `length` can be less than or equal to the field size (for partial reads). Returns `DEV_ERR_INVALID_ARG` if the field ID doesn't exist or if `length` exceeds the field size.

```c
dev_err_t dev_eep_write_field(dev_eep_field_id_t field_id,
                              const void *data, dev_eep_size_t length);
```
Write to a named field. `length` can be less than or equal to the field size (for partial writes). Delegates to `dev_eep_write()` — compare-before-write and dirty tracking apply.

```c
dev_err_t dev_eep_get_field_info(dev_eep_field_id_t field_id,
                                 const dev_eep_field_t **field);
```
Get a pointer to the field descriptor (offset, size, name). Useful for debugging or generic field iteration.

### 7.4 Typed Access (convenience wrappers)

```c
dev_err_t dev_eep_read_u8 (dev_eep_field_id_t field_id, uint8_t  *value);
dev_err_t dev_eep_write_u8(dev_eep_field_id_t field_id, uint8_t   value);

dev_err_t dev_eep_read_u16 (dev_eep_field_id_t field_id, uint16_t *value);
dev_err_t dev_eep_write_u16(dev_eep_field_id_t field_id, uint16_t  value);

dev_err_t dev_eep_read_u32 (dev_eep_field_id_t field_id, uint32_t *value);
dev_err_t dev_eep_write_u32(dev_eep_field_id_t field_id, uint32_t  value);
```
These are thin wrappers around `dev_eep_read_field` / `dev_eep_write_field`. They pass `sizeof(uintX_t)` as the length. The field must be at least as large as the type (e.g., don't call `read_u32` on a 2-byte field).

### 7.5 Dirty State

```c
bool dev_eep_is_dirty(dev_eep_id_t eep_id);
```
Returns `true` if any page in the dirty map is set (i.e., there are unflushed changes).

```c
dev_err_t dev_eep_mark_dirty(dev_eep_id_t eep_id,
                             dev_eep_addr_t addr, dev_eep_size_t length);
```
Manually mark pages covering `[addr, addr+length)` as dirty. Normally you don't need this — `dev_eep_write()` calls it automatically. Use when you modify the mirror through some other mechanism.

```c
dev_err_t dev_eep_clear_dirty(dev_eep_id_t eep_id);
```
Clear all dirty bits without writing to EEPROM. Use with caution — this discards the knowledge of what needs flushing.

```c
uint16_t dev_eep_get_dirty_page_count(dev_eep_id_t eep_id);
```
Return how many pages are currently dirty. Useful for progress reporting during flush.

---

## 8. Porting to a New Board

### Checklist

1. **Wire the EEPROM.** Connect SDA, SCL, VCC, GND. Set A0/A1/A2 address pins as needed. Add pull-up resistors (typically 4.7kΩ) on SDA and SCL if not already present.

2. **Configure the I2C bus.** In `dev_i2c_cfg.h`, ensure `DEV_I2C_BUS_EEPROM` is mapped to the correct hardware I2C peripheral. If not already defined, define it:
   ```c
   #define DEV_I2C_BUS_EEPROM   ((dev_i2c_bus_t)0U)   // or whichever bus it's on
   ```

3. **Set EEPROM parameters.** In `dev_eep_cfg.h`:
   - `DEV_EEP_MAIN_TOTAL_SIZE` — match your chip
   - `DEV_EEP_MAIN_PAGE_SIZE` — match your chip's datasheet
   - `DEV_EEP_MAIN_PAGE_COUNT` — auto-computed
   - `DEV_EEP_MAIN_DIRTY_MAP_SIZE` — auto-computed

4. **Set I2C address.** In `dev_eep.c`, in the `s_devices[]` table, set `.i2c_addr` to match your hardware wiring (7-bit address).

5. **Set memory address size.** In `dev_eep.c`, in `s_devices[]`, set `.mem_addr_size`:
   - `DEV_EEP_MEM_ADDR_SIZE_8BIT` for 24C01–24C16
   - `DEV_EEP_MEM_ADDR_SIZE_16BIT` for 24C32 and larger

6. **Provide `dev_delay_ms()`.** Implement the real delay function (see §4.6).

7. **Build and test.** Run the host tests first (`tests/dev_eep/`), then flash to hardware and verify init succeeds.

### EEPROM Compatibility Table

| Chip | Size | Page | Address bytes | I2C addr range | Notes |
|------|------|------|---------------|----------------|-------|
| 24C01 | 128B | 8 | 1 (8-bit) | 0x50–0x57 | |
| 24C02 | 256B | 8 | 1 (8-bit) | 0x50–0x57 | |
| 24C04 | 512B | 16 | 1 (8-bit) | 0x50–0x53 | Uses address pin for upper address bit |
| 24C08 | 1024B | 16 | 1 (8-bit) | 0x50–0x51 | Uses address pin for upper address bits |
| 24C16 | 2048B | 16 | 1 (8-bit) | 0x50 only | All address pins used for addressing |
| 24C32 | 4096B | 32 | 2 (16-bit) | 0x50–0x57 | |
| 24C64 | 8192B | 32 | 2 (16-bit) | 0x50–0x57 | |
| 24C128 | 16KB | 64 | 2 (16-bit) | 0x50–0x57 | |
| 24C256 | 32KB | 64 | 2 (16-bit) | 0x50–0x57 | |
| 24C512 | 64KB | 128 | 2 (16-bit) | 0x50–0x57 | |

---

## 9. Safety Notes

### EEPROM Wear
- Each EEPROM byte is rated for 100,000–1,000,000 write cycles.
- The driver writes only dirty pages, only on explicit flush. If you flush on every write, you defeat the wear reduction. Batch writes and flush infrequently.
- Identical data is never rewritten — the compare-before-write check prevents pointless wear.

### Power Loss During Write
- If power is lost during a page write, that page may be partially written or corrupted. The CRC check on next boot will detect this.
- With `DEV_EEP_CFG_LOAD_DEFAULTS_ON_INVALID_CRC` enabled, a corrupt EEPROM causes defaults to load. Your application should handle this gracefully.
- The dirty bit is only cleared after a successful page write — if a flush is interrupted, retrying will re-write the failed pages.

### Endianness
- Multi-byte values (magic, version, CRC, boot count) are stored in little-endian byte order (native for ARM Cortex-M). If the EEPROM is shared with a big-endian system, byte swapping is needed.

### Stack Usage
- The I2C write path uses a stack buffer of `DEV_EEP_MAIN_PAGE_SIZE + 4` bytes (default: 20 bytes). Large page sizes increase stack usage.

### Reentrancy
- The driver is **not** reentrant. Do not call dev_eep functions from interrupts or multiple RTOS tasks without external locking.

---

## 10. Bring-Up Checklist

- [ ] EEPROM wired correctly (SDA, SCL, VCC, GND, address pins, pull-ups)
- [ ] `DEV_I2C_BUS_EEPROM` defined in `dev_i2c_cfg.h`
- [ ] EEPROM total size and page size match the datasheet
- [ ] I2C address (7-bit) correct in the device table
- [ ] Memory address size (8/16/24/32-bit) correct
- [ ] `dev_delay_ms()` implemented (not relying on weak no-op)
- [ ] `dev_i2c_init()` succeeds (I2C bus works)
- [ ] `dev_eep_init()` succeeds (or returns DEV_ERR_CRC on first blank boot)
- [ ] Read a known field and verify the value
- [ ] Write a field, flush, power cycle, read back — value persists
- [ ] Host tests pass: `cd tests/dev_eep/build && cmake .. && make && ./eep_test_host`

---

## 11. Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `DEV_ERR_NOT_INITIALIZED` | Forgot to call `dev_eep_init()` | Call `dev_eep_init()` after `dev_i2c_init()` |
| `DEV_ERR_ALREADY_INITIALIZED` | Called `dev_eep_init()` twice | Call `dev_eep_shutdown()` before re-initializing |
| `DEV_ERR_CONFIG` on init | Field layout overlap or out-of-bounds | Check your field offsets and sizes in `dev_eep_layout.h` |
| `DEV_ERR_CRC` on init | EEPROM is blank or data is corrupt | Normal on first boot if `LOAD_DEFAULTS_ON_INVALID_CRC` is OFF. Enable it for first-boot handling. |
| `DEV_ERR_TIMEOUT` on flush | EEPROM not responding (ACK polling timed out) | Check I2C wiring, address, pull-up resistors. Try increasing `ACK_POLL_TIMEOUT_MS`. |
| `DEV_ERR_NO_ACK` | I2C device not found at the configured address | Verify the 7-bit address matches your hardware. Check SDA/SCL connections. |
| Data not persisting after power cycle | `dev_eep_shutdown()` not called, or flush failed silently | Always check return values. Verify dirty pages are cleared after flush. |
| EEPROM wears out quickly | Flushing too frequently | Batch writes. Call `dev_eep_flush()` infrequently, not after every `dev_eep_write()`. |
| `DEV_ERR_OUT_OF_RANGE` | Address + length exceeds EEPROM size | Check your `DEV_EEP_MAIN_TOTAL_SIZE` and field offset/size definitions. |

---

## 12. Design Decisions & Rationale

**Why a RAM mirror instead of reading EEPROM on every access?**
EEPROM reads are fast, but reads through I2C add latency and bus contention. Keeping a mirror eliminates all I2C traffic during normal operation. The trade-off is RAM usage (1KB for a 24C08), which is negligible on modern MCUs.

**Why not write-through (write to EEPROM immediately)?**
EEPROM has limited write endurance. Writing immediately on every change would wear out frequently-updated fields (like boot counters). Batching writes and flushing infrequently dramatically extends EEPROM life.

**Why CRC-16 instead of CRC-32?**
CRC-16 uses 2 bytes versus CRC-32's 4 bytes. For small EEPROMs (128–2048 bytes), saving 2 bytes matters. CRC-16 catches all single-bit, double-bit, and burst errors up to 16 bits — adequate for EEPROM data integrity.

**Why ACK polling instead of fixed delays?**
Fixed delays are pessimistic (you always wait the worst-case time) or risky (you might not wait long enough). ACK polling is exact — you wait exactly as long as the EEPROM needs, and no longer.

---

## 13. Source Files

| File | Purpose |
|------|---------|
| `drivers/dev_eep/include/dev_eep.h` | Public API declarations |
| `drivers/dev_eep/include/dev_eep_types.h` | Type definitions |
| `drivers/dev_eep/include/dev_eep_cfg.h` | Compile-time configuration |
| `drivers/dev_eep/include/dev_eep_layout.h` | Field IDs, offsets, sizes |
| `drivers/dev_eep/src/dev_eep.c` | Full implementation |
| `drivers/dev_eep/src/dev_eep_layout.c` | Field descriptor table |
| `tests/dev_eep/test_eep.c` | 24 host-based unit tests |
