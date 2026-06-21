# dev_eep — EEPROM Device Driver

## Purpose

`dev_eep` is the low-level EEPROM device driver. It handles all I2C communication with EEPROM chips and abstracts device-level details (page boundaries, address widths, write cycle waits) behind a clean hardware-independent API.

`dev_eep` uses `dev_i2c` for all I2C communication. It never calls vendor HALs or SDKs directly.

## Architecture Position

```
Application
   |
   v
svc_eep     (RAM mirror, dirty tracking, field layout, CRC)
   |
   v
dev_eep     (page boundaries, ACK polling, address width)  ← this component
   |
   v
dev_i2c     (I2C bus abstraction)
   |
   v
target I2C port
```

## Public API

| Function | Description |
|----------|-------------|
| `dev_eep_init(eep_id)` | Initialize device; probes I2C bus to verify presence |
| `dev_eep_deinit(eep_id)` | Deinitialize device; clears internal state |
| `dev_eep_read(eep_id, addr, data, len)` | Read raw bytes from EEPROM via I2C |
| `dev_eep_write(eep_id, addr, data, len)` | Write raw bytes to EEPROM; handles page splitting internally |
| `dev_eep_is_ready(eep_id)` | Check if device responds on I2C bus |
| `dev_eep_get_info(eep_id, info)` | Get device info (total size, page size, address width) |

### Write Behavior

`dev_eep_write()` automatically splits writes at page boundaries. Each page is written separately with a write-cycle wait (ACK polling or fixed delay) between pages. Callers do not need to worry about page alignment.

### Read Behavior

`dev_eep_read()` reads the full requested range in a single I2C transaction. Reads are not page-constrained.

### ACK Polling

After each page write, `dev_eep` polls the device via `dev_i2c_probe()`. The device ACKs only after its internal write cycle completes. This is more precise than fixed delays.

## Configuration

All configuration is in `drivers/dev_eep/include/dev_eep_cfg.h`.

### EEPROM Dimensions

```c
#define DEV_EEP_MAIN_TOTAL_SIZE    (256U)   // Total capacity in bytes
#define DEV_EEP_MAIN_PAGE_SIZE     (8U)     // Page write buffer size
#define DEV_EEP_MAIN_PAGE_COUNT    (32U)    // Auto-computed
```

### I2C Address

The 7-bit I2C address is set in the device table inside `dev_eep.c`:

```c
static const dev_eep_config_t s_configs[] = {
    {
        .eep_id        = DEV_EEP_MAIN,
        .i2c_bus       = DEV_I2C_BUS_EEPROM,
        .i2c_addr      = 0x50U,              // 7-bit address
        .total_size    = DEV_EEP_MAIN_TOTAL_SIZE,
        .page_size     = DEV_EEP_MAIN_PAGE_SIZE,
        .mem_addr_size = DEV_EEP_MEM_ADDR_SIZE_8BIT,
        .write_cycle_time_ms = 5U,
    },
};
```

### Feature Toggles

```c
#define DEV_EEP_CFG_ACK_POLLING_ENABLED    DEV_ON   // Use ACK polling after writes
#define DEV_EEP_CFG_PAGE_WRITE_ENABLED     DEV_ON   // Split writes at page boundaries
```

### Timing

```c
#define DEV_EEP_CFG_DEFAULT_WRITE_CYCLE_TIME_MS  (5U)    // Fallback when ACK polling is off
#define DEV_EEP_CFG_ACK_POLL_TIMEOUT_MS          (10U)   // Max time to wait for ACK
```

## Dependencies

| Dependency | Type | Purpose |
|-----------|------|---------|
| `dev_common` | Public | Types, errors, assert macros, delay |
| `dev_i2c` | Public | I2C bus communication |

`dev_eep` does NOT depend on:
- `svc_eep` (service layer)
- Any vendor HAL or SDK
- Any RTOS
- Application code

## Supported EEPROM Chips

| Chip | Size | Page | Address width | I2C addr |
|------|------|------|---------------|----------|
| AT24C01 | 128B | 8 | 8-bit | 0x50–0x57 |
| AT24C02 | 256B | 8 | 8-bit | 0x50–0x57 |
| AT24C04 | 512B | 16 | 8-bit | 0x50–0x53 |
| AT24C08 | 1KB | 16 | 8-bit | 0x50–0x51 |
| AT24C16 | 2KB | 16 | 8-bit | 0x50 |
| AT24C32 | 4KB | 32 | 16-bit | 0x50–0x57 |
| AT24C64 | 8KB | 32 | 16-bit | 0x50–0x57 |
| AT24C128 | 16KB | 64 | 16-bit | 0x50–0x57 |
| AT24C256 | 32KB | 64 | 16-bit | 0x50–0x57 |
| AT24C512 | 64KB | 128 | 16-bit | 0x50–0x57 |

## Safety Notes

- `dev_eep` is not reentrant. Do not call from interrupts or multiple RTOS tasks without external locking.
- `dev_delay_ms()` must be provided by the application layer. The weak default is a no-op.
- ACK polling timeout defaults to 10ms. Most EEPROMs complete writes in 5ms. Increase if using larger chips.

## Porting to New Hardware

`dev_eep` does not need a port layer. All hardware-specific I2C implementation lives in `dev_i2c`. To use `dev_eep` on a new MCU:

1. Implement `dev_i2c` port for the target MCU.
2. Configure EEPROM dimensions in `dev_eep_cfg.h`.
3. Set I2C bus and address in the device table.
4. Provide `dev_delay_ms()`.

## Source Files

| File | Purpose |
|------|---------|
| `drivers/dev_eep/include/dev_eep.h` | Public API |
| `drivers/dev_eep/include/dev_eep_types.h` | Type definitions |
| `drivers/dev_eep/include/dev_eep_cfg.h` | Compile-time configuration |
| `drivers/dev_eep/src/dev_eep.c` | Implementation |
| `drivers/dev_eep/CMakeLists.txt` | Build definition |
