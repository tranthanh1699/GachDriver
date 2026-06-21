# dev_eep — EEPROM Device Driver

## 1. Purpose

`dev_eep` is the low-level EEPROM device driver. It handles all I2C communication with EEPROM chips and abstracts device-level details behind a clean, hardware-independent API:

- **I2C device address** — which chip on which bus
- **Page boundary handling** — splits writes that cross page boundaries
- **Address width** — 8-bit, 16-bit, 24-bit, or 32-bit memory addressing
- **Write cycle waits** — ACK polling or fixed delay after each page write

`dev_eep` uses `dev_i2c` for all I2C bus operations. It never calls vendor HALs or SDKs directly.

## 2. Where It Sits

```
Application
   |
   v
svc_eep         ← service layer (mirror, dirty tracking, fields, CRC)
   |
   v
dev_eep         ← THIS component (page splitting, ACK polling, I2C addr)
   |
   v
dev_i2c         ← I2C bus abstraction
   |
   v
target I2C port ← STM32 / ESP32 / nRF / mock
```

`dev_eep` can also be used directly by application code when you don't need the service-layer features (RAM mirror, dirty tracking, field layout, CRC).

## 3. Public API

| Function | What it does |
|----------|-------------|
| `dev_eep_init(eep_id)` | Probe device on I2C; mark ready for use |
| `dev_eep_deinit(eep_id)` | Clear internal state (no I2C traffic) |
| `dev_eep_read(eep_id, addr, data, len)` | Read raw bytes from EEPROM via I2C |
| `dev_eep_write(eep_id, addr, data, len)` | Write raw bytes — page-splitting is automatic |
| `dev_eep_is_ready(eep_id)` | Check if device ACKs on the I2C bus |
| `dev_eep_get_info(eep_id, info)` | Query total size, page size, address width |

### 3.1 `dev_eep_init`

```c
dev_err_t dev_eep_init(dev_eep_id_t eep_id);
```

- Probes the device on the I2C bus by calling `dev_i2c_probe()`.
- If the device ACKs, the driver marks it initialized and returns `DEV_OK`.
- If the device does not respond, returns `DEV_ERR_TIMEOUT` (or `DEV_ERR_NO_ACK` from the port layer).
- If already initialized, returns `DEV_ERR_ALREADY_INITIALIZED`.
- If `eep_id` is not in the config table, returns `DEV_ERR_INVALID_ARG`.

**Must be called after `dev_i2c_init()`.** `dev_eep` does not initialize the I2C bus — that is the application's responsibility.

### 3.2 `dev_eep_read`

```c
dev_err_t dev_eep_read(dev_eep_id_t eep_id,
                       uint32_t address,
                       uint8_t *data,
                       uint32_t length);
```

- Reads `length` bytes starting at `address` from the physical EEPROM.
- **Reads are not page-constrained.** The entire requested range is read in a single I2C transaction.
- For 8-bit and 16-bit address widths, uses `dev_i2c_mem_read()` (efficient combined "write address + read data" transaction).
- For 24-bit and 32-bit address widths, uses `dev_i2c_write_read()` (manually constructs the address bytes).
- Returns `DEV_ERR_OUT_OF_RANGE` if `address + length` exceeds the device size.

### 3.3 `dev_eep_write`

```c
dev_err_t dev_eep_write(dev_eep_id_t eep_id,
                        uint32_t address,
                        const uint8_t *data,
                        uint32_t length);
```

- Writes `length` bytes starting at `address` to the physical EEPROM.
- **Automatically splits the write at page boundaries.** The caller does NOT need to align data or worry about page size.
- For each page chunk:
  1. Sends the data via `dev_i2c_mem_write()` (or `dev_i2c_write()` for 24/32-bit addressing).
  2. Waits for the EEPROM write cycle to complete (ACK polling or fixed delay).
  3. Advances to the next chunk.
- If any page write or wait fails, returns immediately with the error. Already-written pages are NOT rolled back.
- Returns `DEV_ERR_OUT_OF_RANGE` if `address + length` exceeds the device size.

### 3.4 `dev_eep_is_ready`

```c
dev_err_t dev_eep_is_ready(dev_eep_id_t eep_id);
```

- Probes the device via `dev_i2c_probe()`. Returns `DEV_OK` if the device ACKs.
- Useful as a health check before a write, or to verify the device is still present.

### 3.5 `dev_eep_get_info`

```c
dev_err_t dev_eep_get_info(dev_eep_id_t eep_id,
                           dev_eep_info_t *info);
```

- Fills a `dev_eep_info_t` struct with:
  - `total_size` — total EEPROM capacity in bytes
  - `page_size` — write page buffer size in bytes
  - `mem_addr_size` — address width enum

## 4. Type Reference

### 4.1 `dev_eep_id_t`

```c
typedef uint8_t dev_eep_id_t;
```

Logical ID for an EEPROM device instance. Values come from the enum in `dev_eep_cfg.h`:
```c
enum { DEV_EEP_MAIN = 0, DEV_EEP_CFG_MAX_DEVICES };
```

### 4.2 `dev_eep_config_t`

```c
typedef struct
{
    dev_eep_id_t            eep_id;              // Logical ID (matches enum)
    dev_i2c_bus_t           i2c_bus;             // Which I2C bus (from dev_i2c_cfg.h)
    dev_i2c_addr_t          i2c_addr;            // 7-bit I2C address (unshifted)
    dev_eep_size_t          total_size;           // Total capacity in bytes
    dev_eep_size_t          page_size;            // Write page buffer size
    dev_eep_mem_addr_size_t mem_addr_size;        // 8/16/24/32-bit addressing
    uint32_t                write_cycle_time_ms;  // Fallback delay (ms)
} dev_eep_config_t;
```

| Field | Type | Meaning |
|-------|------|---------|
| `eep_id` | `uint8_t` | Must match the enum value in `dev_eep_cfg.h` |
| `i2c_bus` | `dev_i2c_bus_t` | Logical I2C bus ID, e.g. `DEV_I2C_BUS_EEPROM` |
| `i2c_addr` | `dev_i2c_addr_t` | 7-bit I2C address. 24Cxx base is `0x50` |
| `total_size` | `uint32_t` | Total EEPROM capacity. AT24C02 = 256, AT24C08 = 1024 |
| `page_size` | `uint32_t` | Write page buffer. **Critical** — get this wrong and writes silently fail |
| `mem_addr_size` | enum | `DEV_EEP_MEM_ADDR_SIZE_8BIT` for ≤2KB chips, `_16BIT` for ≥4KB |
| `write_cycle_time_ms` | `uint32_t` | Used only when ACK polling is disabled. Datasheet value, typically 5ms |

### 4.3 `dev_eep_info_t`

```c
typedef struct
{
    dev_eep_size_t          total_size;
    dev_eep_size_t          page_size;
    dev_eep_mem_addr_size_t mem_addr_size;
} dev_eep_info_t;
```

Runtime-readable subset of the config. Use `dev_eep_get_info()` when code needs to know EEPROM geometry without hardcoding it.

### 4.4 `dev_eep_mem_addr_size_t`

```c
typedef enum
{
    DEV_EEP_MEM_ADDR_SIZE_8BIT  = 0,  // 24C01–24C16  (≤2KB)
    DEV_EEP_MEM_ADDR_SIZE_16BIT,      // 24C32–24C512 (4KB–64KB)
    DEV_EEP_MEM_ADDR_SIZE_24BIT,      // Rare, very large (>64KB)
    DEV_EEP_MEM_ADDR_SIZE_32BIT       // Extremely rare
} dev_eep_mem_addr_size_t;
```

## 5. Configuration — Every `#define` Explained

All device-level configuration lives in `drivers/dev_eep/include/dev_eep_cfg.h`.

### 5.1 Feature Toggles

```c
#define DEV_EEP_CFG_RUNTIME_CHECK_ENABLED  DEV_ON
```
When `DEV_ON`: all public API functions validate parameters (null pointers, range checks, init state). When `DEV_OFF`: parameter checks are skipped for speed. **Keep ON except in extreme flash/RAM-constrained situations.**

```c
#define DEV_EEP_CFG_ACK_POLLING_ENABLED    DEV_ON
```
When `DEV_ON`: after each page write, the driver actively polls the EEPROM via `dev_i2c_probe()` until the device ACKs (meaning its internal write cycle is done). This is more precise than a fixed delay — you wait exactly as long as the chip needs.

When `DEV_OFF`: the driver uses a fixed delay (`write_cycle_time_ms`, typically 5ms) after each page write. This is simpler but can be either too short (data corruption) or too long (wasted time).

```c
#define DEV_EEP_CFG_PAGE_WRITE_ENABLED     DEV_ON
```
When `DEV_ON`: `dev_eep_write()` splits data at page boundaries and writes one page at a time. **Keep ON.** When `DEV_OFF`: writes are not split (useful only for single-page writes or debugging).

### 5.2 Timing

```c
#define DEV_EEP_CFG_DEFAULT_WRITE_CYCLE_TIME_MS  (5U)
```
Fallback delay in milliseconds. Used **only when ACK polling is disabled** (`DEV_EEP_CFG_ACK_POLLING_ENABLED == DEV_OFF`). Set to the maximum write cycle time from your EEPROM datasheet. For most 24Cxx chips this is 5ms.

```c
#define DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS          (10U)
```
Maximum time the driver will poll for an ACK before giving up. After each page write, the driver probes the device repeatedly. If the device doesn't ACK within this timeout, `dev_eep_write()` returns `DEV_ERR_TIMEOUT`. 10ms covers the worst-case write cycle of most EEPROMs. Increase if using very large chips (some 24C512 take up to 10ms).

```c
#define DEV_EEP_CFG_ACK_POLL_INTERVAL_US         (100U)
```
Delay between ACK polling attempts, in microseconds. Currently not used in the implementation (the driver polls with 1ms intervals via `dev_delay_ms(1)`). Reserved for future finer-grained polling.

### 5.3 Device Count

```c
enum
{
    DEV_EEP_MAIN = 0,
    DEV_EEP_CFG_MAX_DEVICES    // ← auto-computed; add new IDs before this
};
```

To add a second EEPROM:
```c
enum
{
    DEV_EEP_MAIN = 0,
    DEV_EEP_AUX,                // ← new
    DEV_EEP_CFG_MAX_DEVICES
};
```

Then add a matching entry in the `s_configs[]` table inside `dev_eep.c`.

### 5.4 EEPROM Dimensions

```c
#define DEV_EEP_MAIN_TOTAL_SIZE    (256U)   // Total capacity in bytes
#define DEV_EEP_MAIN_PAGE_SIZE     (8U)     // Write page buffer size
#define DEV_EEP_MAIN_PAGE_COUNT    (DEV_EEP_MAIN_TOTAL_SIZE / DEV_EEP_MAIN_PAGE_SIZE)
```

| Chip | `TOTAL_SIZE` | `PAGE_SIZE` | `PAGE_COUNT` | Address width |
|------|-------------|------------|-------------|---------------|
| AT24C01 | 128 | 8 | 16 | 8-bit |
| AT24C02 | 256 | 8 | 32 | 8-bit |
| AT24C04 | 512 | 16 | 32 | 8-bit |
| AT24C08 | 1024 | 16 | 64 | 8-bit |
| AT24C16 | 2048 | 16 | 128 | 8-bit |
| AT24C32 | 4096 | 32 | 128 | 16-bit |
| AT24C64 | 8192 | 32 | 256 | 16-bit |
| AT24C128 | 16384 | 64 | 256 | 16-bit |
| AT24C256 | 32768 | 64 | 512 | 16-bit |
| AT24C512 | 65536 | 128 | 512 | 16-bit |

## 6. How Page Splitting Works

When you call `dev_eep_write(eep_id, addr, data, length)`, the driver:

```
Input: address=10, length=25, page_size=8

  Page 1 (bytes 8-15):   [--******]   ← address 10, write 6 bytes (to fill page)
  Page 2 (bytes 16-23):  [********]   ← write 8 bytes (full page)
  Page 3 (bytes 24-31):  [********]   ← write 8 bytes (full page)
  Page 4 (bytes 32-39):  [***-----]   ← write 3 bytes (remaining)

  Total: 4 separate I2C write transactions, each ≤ page_size
         Each followed by an ACK-polling wait.
```

The driver computes each chunk as:
```c
chunk = page_size - (addr % page_size);   // bytes until next page boundary
if (chunk > remaining) chunk = remaining; // don't overshoot the end
```

This means you can write any size at any address — the driver handles alignment automatically.

## 7. Write Cycle Wait — ACK Polling vs Fixed Delay

### ACK Polling (default, `DEV_EEP_CFG_ACK_POLLING_ENABLED == DEV_ON`)

```
After sending a page write:
   │
   ▼
dev_i2c_probe(device_addr)
   │
   ├── DEV_OK ──→ Device ACKed → write cycle complete → proceed
   │
   └── error ──→ dev_delay_ms(1) → increment elapsed → probe again
                    │
                    ├── elapsed < timeout → loop
                    └── elapsed ≥ timeout → DEV_ERR_TIMEOUT
```

The EEPROM ignores all I2C traffic while its internal write is in progress — it won't ACK its address. As soon as it ACKs, the write is done. No guesswork, no wasted time.

### Fixed Delay (fallback, `DEV_EEP_CFG_ACK_POLLING_ENABLED == DEV_OFF`)

```
After sending a page write:
   │
   ▼
dev_delay_ms(write_cycle_time_ms)   ← always wait 5ms (or whatever you set)
   │
   ▼
proceed
```

Simpler but risky — if your delay is shorter than the actual write time, the next write corrupts data.

## 8. Usage Examples

### 8.1 Minimal Bare-Metal Read/Write (without svc_eep)

```c
#include "dev_i2c.h"
#include "dev_eep.h"
#include "dev_common.h"
#include <stdio.h>

/* ── Provide real delay (STM32 example) ── */
void dev_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

int main(void)
{
    dev_err_t  err;
    uint8_t    buf[4];

    /* 1. Hardware init (vendor layer) */
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();              // CubeMX-generated I2C pin/clock config

    /* 2. Initialize the I2C abstraction */
    err = dev_i2c_init();
    if (err != DEV_OK) {
        printf("I2C init failed: %d\n", err);
        return 1;
    }

    /* 3. Initialize the EEPROM device */
    err = dev_eep_init(DEV_EEP_MAIN);
    if (err != DEV_OK) {
        printf("EEPROM not found at 0x50: %d\n", err);
        return 1;
    }

    /* 4. Write 4 bytes at address 0 */
    buf[0] = 0xDE; buf[1] = 0xAD;
    buf[2] = 0xBE; buf[3] = 0xEF;
    err = dev_eep_write(DEV_EEP_MAIN, 0U, buf, 4U);
    if (err != DEV_OK) {
        printf("Write failed: %d\n", err);
        return 1;
    }

    /* 5. Read them back */
    buf[0] = buf[1] = buf[2] = buf[3] = 0U;
    err = dev_eep_read(DEV_EEP_MAIN, 0U, buf, 4U);
    if (err != DEV_OK) {
        printf("Read failed: %d\n", err);
        return 1;
    }

    printf("Data: 0x%02X 0x%02X 0x%02X 0x%02X\n",
           buf[0], buf[1], buf[2], buf[3]);
    /* Prints: Data: 0xDE 0xAD 0xBE 0xEF */

    /* 6. Cleanup */
    dev_eep_deinit(DEV_EEP_MAIN);
    dev_i2c_deinit();
    return 0;
}
```

### 8.2 Checking Device Readiness

```c
dev_err_t err = dev_eep_is_ready(DEV_EEP_MAIN);
if (err == DEV_OK) {
    /* Device is present and responding */
} else if (err == DEV_ERR_NOT_INITIALIZED) {
    /* Forgot to call dev_eep_init() */
} else {
    /* Device not responding — check wiring, power, pull-ups */
}
```

### 8.3 Querying Device Info at Runtime

```c
dev_eep_info_t info;
dev_err_t err = dev_eep_get_info(DEV_EEP_MAIN, &info);
if (err == DEV_OK) {
    printf("EEPROM: %lu bytes, %lu-byte pages, addr_width=%d\n",
           (unsigned long)info.total_size,
           (unsigned long)info.page_size,
           (int)info.mem_addr_size);
}
```

### 8.4 Writing Across Page Boundaries (Automatic)

```c
/* Write 20 bytes starting at address 5 on an 8-byte-page EEPROM.
 * The driver automatically splits this into:
 *   page 0: bytes 5-7   (3 bytes, fills page 0)
 *   page 1: bytes 8-15  (8 bytes, full page)
 *   page 2: bytes 16-23 (8 bytes, full page)
 *   page 3: byte  24    (1 byte)
 * You don't need to do any alignment math. */
uint8_t data[20];
memset(data, 0x42, sizeof(data));
dev_err_t err = dev_eep_write(DEV_EEP_MAIN, 5U, data, 20U);
```

### 8.5 Error Handling for Every API

```c
dev_err_t err;

err = dev_eep_write(DEV_EEP_MAIN, 100U, my_data, 16U);
switch (err) {
case DEV_OK:
    break;  /* success */
case DEV_ERR_NOT_INITIALIZED:
    /* Forgot to call dev_eep_init() — or dev_i2c_init() before it */
    break;
case DEV_ERR_INVALID_ARG:
    /* Invalid eep_id, or length == 0 */
    break;
case DEV_ERR_NULL_PTR:
    /* data pointer is NULL */
    break;
case DEV_ERR_OUT_OF_RANGE:
    /* address + length exceeds DEV_EEP_MAIN_TOTAL_SIZE */
    break;
case DEV_ERR_TIMEOUT:
    /* Device didn't ACK within DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS.
     * Check: is the EEPROM powered? Are pull-ups present?
     * Is the I2C address correct? Is the bus speed too high? */
    break;
case DEV_ERR_NO_ACK:
    /* I2C NACK — device not present at all, or wrong address */
    break;
default:
    /* Unexpected error — log and investigate */
    break;
}
```

## 9. Adding a Second EEPROM Device

### Step 1 — Add a device ID in `dev_eep_cfg.h`

```c
enum
{
    DEV_EEP_MAIN = 0,
    DEV_EEP_AUX,              // ← new ID
    DEV_EEP_CFG_MAX_DEVICES
};
```

### Step 2 — Add dimension macros

```c
#define DEV_EEP_AUX_TOTAL_SIZE    (1024U)
#define DEV_EEP_AUX_PAGE_SIZE     (16U)
#define DEV_EEP_AUX_PAGE_COUNT    (DEV_EEP_AUX_TOTAL_SIZE / DEV_EEP_AUX_PAGE_SIZE)
```

### Step 3 — Add config entry in `dev_eep.c`

```c
static const dev_eep_config_t s_configs[DEV_EEP_CFG_MAX_DEVICES] =
{
    {
        DEV_EEP_MAIN,
        DEV_I2C_BUS_EEPROM,
        ((dev_i2c_addr_t)0x50U),
        DEV_EEP_MAIN_TOTAL_SIZE,
        DEV_EEP_MAIN_PAGE_SIZE,
        DEV_EEP_MEM_ADDR_SIZE_8BIT,
        5U
    },
    {
        DEV_EEP_AUX,                                     // ← new entry
        DEV_I2C_BUS_EEPROM,                              // same bus
        ((dev_i2c_addr_t)0x51U),                         // different address
        DEV_EEP_AUX_TOTAL_SIZE,
        DEV_EEP_AUX_PAGE_SIZE,
        DEV_EEP_MEM_ADDR_SIZE_16BIT,                     // 24C32 needs 16-bit
        5U
    },
};
```

### Step 4 — Use it

```c
dev_eep_init(DEV_EEP_AUX);
dev_eep_write(DEV_EEP_AUX, 0U, my_data, my_len);
```

## 10. Dependency Rules

| Allowed | Forbidden |
|---------|-----------|
| `dev_i2c` — all I2C bus operations | `svc_eep` — service layer must not be called from dev_eep |
| `dev_common` — types, errors, assert, delay | Any vendor HAL / SDK |
| `<string.h>` — `memcpy` only | Any RTOS API |
| | Application code |

## 11. Safety Notes

- **Not reentrant.** A per-device init flag is tracked, but concurrent I2C access is not mutex-protected. Use external locking if calling from multiple RTOS tasks.
- **`dev_delay_ms()` must be real.** The weak default is a no-op. Without a real delay, ACK polling loops spin at CPU speed and fixed-delay fallback never waits — causing data corruption.
- **Page size is critical.** If `DEV_EEP_MAIN_PAGE_SIZE` doesn't match the actual chip, writes that cross the real page boundary will wrap around and corrupt data silently. Always verify against the datasheet.
- **No rollback on partial write.** If a multi-page `dev_eep_write()` fails on page 3 of 5, pages 1–2 are already written. The caller must handle this (re-read and verify, or use `svc_eep` which provides CRC protection).

## 12. Porting to New Hardware

`dev_eep` has no port layer. All hardware-specific work is in `dev_i2c`. To bring up `dev_eep` on a new MCU:

1. Implement the `dev_i2c` port for the target (e.g., `dev_i2c_port_stm32.c`).
2. Configure `dev_i2c_cfg.h` — define `DEV_I2C_BUS_EEPROM`, set bus count.
3. Configure `dev_eep_cfg.h` — set `TOTAL_SIZE`, `PAGE_SIZE`.
4. Set the I2C address and address width in the `s_configs[]` table.
5. Provide a real `dev_delay_ms()` implementation.
6. Build, flash, and call `dev_eep_init(DEV_EEP_MAIN)`. If it returns `DEV_OK`, the EEPROM is alive.

## 13. Source Files

| File | Purpose |
|------|---------|
| `drivers/dev_eep/include/dev_eep.h` | Public API declarations |
| `drivers/dev_eep/include/dev_eep_types.h` | `dev_eep_config_t`, `dev_eep_info_t`, enums |
| `drivers/dev_eep/include/dev_eep_cfg.h` | Compile-time configuration (dimensions, timing, toggles) |
| `drivers/dev_eep/src/dev_eep.c` | Full implementation (~400 lines) |
| `drivers/dev_eep/CMakeLists.txt` | Build — links `dev_common` + `dev_i2c` |
