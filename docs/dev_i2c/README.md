# dev_i2c — Simplified I2C Wrapper

## 1. Overview

Hardware-independent I2C driver using logical bus IDs and unshifted 7-bit addresses.

- **Blocking** transfers with timeout
- **STM32 Cube-managed**: wraps Cube HAL handles (`hi2c1`, `hi2c2`...) — no GPIO/clock/NVIC/timing init
- **Address rule**: always use unshifted 7-bit address (`0x48`). Port shifts to HAL format (`0x90`) internally

## 2. Quick Start

```c
#include "dev_i2c.h"

#define SENSOR_ADDR  ((dev_i2c_addr_t)0x48U)

dev_i2c_init();
uint8_t data[2];
dev_i2c_mem_read(DEV_I2C_BUS_SENSOR, SENSOR_ADDR, 0x00U,
                 DEV_I2C_MEM_ADDR_SIZE_8BIT, data, 2U, DEV_I2C_TIMEOUT_DEFAULT_MS);
```

---

## 3. Adding a New I2C Bus — Complete Step-by-Step

This guide walks through adding a 3rd I2C bus (`DEV_I2C_BUS_POWER`) on I2C3 with PB3=SCL, PB4=SDA.

### Step 1: Enable I2C HAL in CubeMX

In `Core/Inc/stm32h7xx_hal_conf.h`, uncomment:

```c
#define HAL_I2C_MODULE_ENABLED
```

### Step 2: Configure I2C3 in CubeMX

In CubeMX, enable I2C3 with:
- Mode: I2C
- SCL: PB3, SDA: PB4
- Speed: Fast (400 kHz)
- NVIC: Enable I2C3 event interrupt (if needed)

CubeMX generates `hi2c3` handle in `Core/Src/i2c.c`.

### Step 3: Declare the Cube handle as extern

In `drivers/dev_i2c/port/stm32/dev_i2c_port_stm32.c`, add:

```c
extern I2C_HandleTypeDef hi2c3;   // <-- add this line
```

### Step 4: Add logical bus ID

In `drivers/dev_i2c/include/dev_i2c_cfg.h`:

```c
#define DEV_I2C_BUS_POWER                      ((dev_i2c_bus_t)2U)
```

Also bump `DEV_I2C_CFG_MAX_BUSES`:

```c
#define DEV_I2C_CFG_MAX_BUSES                  (3U)   // was 2U, now 3U
```

### Step 5: Add entry to I2C map

In `drivers/dev_i2c/port/stm32/dev_i2c_port_stm32.c`, inside `s_i2c_map[]`:

```c
static const dev_i2c_hw_bus_t s_i2c_map[DEV_I2C_CFG_MAX_BUSES] = {
    [DEV_I2C_BUS_SENSOR] = { DEV_I2C_BUS_SENSOR, &hi2c1, DEV_I2C_SPEED_FAST },
    [DEV_I2C_BUS_EEPROM] = { DEV_I2C_BUS_EEPROM, &hi2c2, DEV_I2C_SPEED_STANDARD },
    [DEV_I2C_BUS_POWER]  = { DEV_I2C_BUS_POWER,  &hi2c3, DEV_I2C_SPEED_FAST },  // <-- add
};
```

### Step 6: Verify CubeMX HAL source is compiled

In `cmake/stm32cubemx/CMakeLists.txt`, confirm `stm32h7xx_hal_i2c.c` is in the driver source list. Add if missing.

### Step 7: Rebuild and use

```c
dev_i2c_init();
dev_i2c_mem_read(DEV_I2C_BUS_POWER, 0x50U, 0x00U,
                 DEV_I2C_MEM_ADDR_SIZE_8BIT, data, 4U, DEV_I2C_TIMEOUT_DEFAULT_MS);
```

### Summary: Files to modify

| # | File | Change |
|---|------|--------|
| 1 | `Core/Inc/stm32h7xx_hal_conf.h` | Uncomment `HAL_I2C_MODULE_ENABLED` |
| 2 | CubeMX | Enable I2C3, generate code |
| 3 | `dev_i2c_port_stm32.c` | Add `extern I2C_HandleTypeDef hi2c3` |
| 4 | `dev_i2c_cfg.h` | Add `DEV_I2C_BUS_POWER` ID |
| 5 | `dev_i2c_cfg.h` | Bump `DEV_I2C_CFG_MAX_BUSES` to 3 |
| 6 | `dev_i2c_port_stm32.c` | Add entry to `s_i2c_map[]` |
| 7 | `cmake/stm32cubemx/CMakeLists.txt` | Verify `stm32h7xx_hal_i2c.c` is listed |

No changes needed in `dev_i2c.c`, `dev_i2c_port.h`, or `dev_i2c.h`.

---

## 4. Configuration (`dev_i2c_cfg.h`)

```c
#define DEV_I2C_CFG_MAX_BUSES                  (2U)     // max I2C buses
#define DEV_I2C_CFG_RUNTIME_CHECK_ENABLED      (1U)
#define DEV_I2C_CFG_MEM_ACCESS_ENABLED         (1U)     // 0U = mem APIs return NOT_SUPPORTED
#define DEV_I2C_CFG_BUS_RECOVERY_ENABLED       (0U)     // unsupported in Cube mode
#define DEV_I2C_STM32_CUBE_MANAGED_HW_INIT     (1U)
#define DEV_I2C_TIMEOUT_DEFAULT_MS             (100U)
#define DEV_I2C_ADDR_7BIT_MAX                  (0x7FU)

#define DEV_I2C_BUS_SENSOR                     ((dev_i2c_bus_t)0U)
#define DEV_I2C_BUS_EEPROM                     ((dev_i2c_bus_t)1U)
```

---

## 5. API

| Function | Blocking | Key Returns |
|----------|----------|-------------|
| `init()` / `deinit()` | — | `DEV_OK`, `DEV_ERR_ALREADY_INITIALIZED` |
| `set_speed(bus, speed)` | — | `DEV_ERR_NOT_SUPPORTED` in Cube mode |
| `write(bus, addr, data, len, to)` | Yes | `DEV_OK`, `DEV_ERR_TIMEOUT`, `DEV_ERR_NO_ACK` |
| `read(bus, addr, data, len, to)` | Yes | `DEV_OK`, `DEV_ERR_TIMEOUT`, `DEV_ERR_NO_ACK` |
| `write_read(bus, addr, wd, wl, rd, rl, to)` | Yes | `DEV_OK`, sequential transmit+receive |
| `mem_write(bus, addr, reg, size, data, len, to)` | Yes | `DEV_OK`, uses `HAL_I2C_Mem_Write` |
| `mem_read(bus, addr, reg, size, data, len, to)` | Yes | `DEV_OK`, uses `HAL_I2C_Mem_Read` |
| `probe(bus, addr, to)` | Yes | `DEV_OK`, `DEV_ERR_NO_ACK`, `DEV_ERR_TIMEOUT` |
| `recover_bus(bus)` | — | `DEV_ERR_NOT_SUPPORTED` in Cube mode |

---

## 6. Address Rule

**Always unshifted 7-bit.** The STM32 port shifts internally:

```c
dev_i2c_write(bus, 0x48U, data, len, 100U);  // ✓ correct
dev_i2c_write(bus, 0x90U, data, len, 100U);  // ✗ wrong (shifted HAL address)
```

---

## 7. Error Reference

| Condition | Error |
|-----------|-------|
| Success | `DEV_OK` |
| NACK (no device) | `DEV_ERR_NO_ACK` |
| Bus error / arbitration | `DEV_ERR_BUS` |
| Timeout | `DEV_ERR_TIMEOUT` |
| Busy | `DEV_ERR_BUSY` |
| Invalid bus/addr/len/enum | `DEV_ERR_INVALID_ARG` |
| NULL pointer | `DEV_ERR_NULL_PTR` |
| Not initialized | `DEV_ERR_NOT_INITIALIZED` |
| Unsupported | `DEV_ERR_NOT_SUPPORTED` |
| HAL failure | `DEV_ERR_HW_FAILURE` |

---

## 8. Build

```cmake
set(DEV_I2C_PORT "stm32" CACHE STRING "I2C port: mock, stm32, esp32")
add_subdirectory(drivers/dev_i2c)
target_link_libraries(${PROJECT_NAME} dev_i2c)
```

Host tests: `DEV_I2C_PORT=mock`. 24 tests pass.
