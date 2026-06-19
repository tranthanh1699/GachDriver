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
