# dev_i2c — Simplified I2C Wrapper

## 1. Overview

Hardware-independent I2C driver. Application code uses **logical bus IDs** and **unshifted 7-bit addresses** — no vendor HAL types exposed.

| Feature | Support |
|---------|---------|
| Write / Read | ✓ |
| Write-Read (repeated START) | ✓ |
| Memory Write/Read (8/16-bit reg addr) | ✓ |
| Probe (device present?) | ✓ |
| Bus recovery | ✓ (STM32), unsupported on ESP32/mock |
| 10-bit addressing | Optional (compile flag) |
| Ports | STM32H7, ESP32 stub, Mock (host test) |

---

## 2. Quick Start

```c
#include "dev_i2c.h"

#define SENSOR_ADDR  ((dev_i2c_addr_t)0x48U)   /* unshifted 7-bit */
#define TEMP_REG     ((uint16_t)0x00U)

int main(void)
{
    uint8_t data[2];
    dev_i2c_init();
    dev_i2c_mem_read(DEV_I2C_BUS_SENSOR, SENSOR_ADDR, TEMP_REG,
                     DEV_I2C_MEM_ADDR_SIZE_8BIT, data, 2U, 100U);
}
```

---

## 3. Types

```c
typedef uint8_t  dev_i2c_bus_t;          // logical bus ID
typedef uint16_t dev_i2c_addr_t;          // unshifted 7-bit address (0x00..0x7F)
typedef uint32_t dev_i2c_timeout_t;       // milliseconds

typedef enum { STANDARD=0, FAST, FAST_PLUS, HIGH } dev_i2c_speed_t;
typedef enum { MEM_ADDR_SIZE_8BIT=0, MEM_ADDR_SIZE_16BIT } dev_i2c_mem_addr_size_t;
```

---

## 4. Configuration — `dev_i2c_cfg.h`

```c
#define DEV_I2C_CFG_MAX_BUSES             (2U)
#define DEV_I2C_CFG_RUNTIME_CHECK_ENABLED (1U)
#define DEV_I2C_CFG_MEM_ACCESS_ENABLED    (1U)     // 0U = mem APIs return NOT_SUPPORTED
#define DEV_I2C_CFG_BUS_RECOVERY_ENABLED  (0U)     // 1U = enable recover_bus()
#define DEV_I2C_TIMEOUT_DEFAULT_MS        (100U)

#define DEV_I2C_BUS_SENSOR  ((dev_i2c_bus_t)0U)
#define DEV_I2C_BUS_EEPROM  ((dev_i2c_bus_t)1U)
```

---

## 5. Public API

```c
dev_err_t dev_i2c_init(void);
dev_err_t dev_i2c_deinit(void);
bool     dev_i2c_is_initialized(void);

dev_err_t dev_i2c_set_speed(bus, speed);
dev_err_t dev_i2c_write(bus, addr, data, len, timeout_ms);
dev_err_t dev_i2c_read(bus, addr, data, len, timeout_ms);
dev_err_t dev_i2c_write_read(bus, addr, wdata, wlen, rdata, rlen, timeout_ms);
dev_err_t dev_i2c_mem_write(bus, addr, mem_addr, addr_size, data, len, timeout_ms);
dev_err_t dev_i2c_mem_read(bus, addr, mem_addr, addr_size, data, len, timeout_ms);
dev_err_t dev_i2c_probe(bus, addr, timeout_ms);
dev_err_t dev_i2c_recover_bus(bus);
```

### Return values

| Condition | Error |
|-----------|-------|
| Success | `DEV_OK` |
| Not initialized | `DEV_ERR_NOT_INITIALIZED` |
| Invalid bus/addr/len/enum | `DEV_ERR_INVALID_ARG` |
| NULL pointer | `DEV_ERR_NULL_PTR` |
| Timeout | `DEV_ERR_TIMEOUT` |
| Slave NACK | `DEV_ERR_NO_ACK` |
| Bus stuck/arbitration | `DEV_ERR_BUS` |
| Unsupported feature | `DEV_ERR_NOT_SUPPORTED` |
| HAL failure | `DEV_ERR_HW_FAILURE` |

---

## 6. Address Rule

**Always use the unshifted 7-bit address.** The port layer handles left-shifting for vendor APIs like STM32 HAL.

```c
// ✓ Correct: unshifted 7-bit
dev_i2c_write(bus, 0x48U, data, len, 100U);

// ✗ Wrong: already-shifted (STM32-style)
dev_i2c_write(bus, 0x90U, data, len, 100U);  // rejected: > 0x7F
```

---

## 7. Usage Examples

### Sensor register read
```c
uint8_t buf[2];
dev_i2c_mem_read(DEV_I2C_BUS_SENSOR, 0x48U, 0x00U,
                 DEV_I2C_MEM_ADDR_SIZE_8BIT, buf, 2U, 100U);
```

### EEPROM write
```c
dev_i2c_mem_write(DEV_I2C_BUS_EEPROM, 0x50U, 0x0100U,
                  DEV_I2C_MEM_ADDR_SIZE_16BIT, data, 32U, 100U);
```

### Probe
```c
if (dev_i2c_probe(DEV_I2C_BUS_SENSOR, 0x48U, 50U) == DEV_OK) {
    /* device present */
}
```

---

## 8. Porting

Create `port/<vendor>/dev_i2c_port_<vendor>.c` implementing 11 functions from `dev_i2c_port.h`.

STM32 bus mapping example (`port/stm32/dev_i2c_port_stm32.c`):
```c
static const dev_i2c_hw_bus_t s_i2c_map[DEV_I2C_CFG_MAX_BUSES] = {
    [DEV_I2C_BUS_SENSOR] = { DEV_I2C_BUS_SENSOR, I2C1,
        GPIOB, GPIO_PIN_8, GPIOB, GPIO_PIN_9, GPIO_AF4_I2C1,
        DEV_I2C_SPEED_FAST, DEV_I2C_STM32_TIMING_400KHZ },
};
```

ESP32 bus mapping:
```c
static const dev_i2c_hw_bus_t s_i2c_map[DEV_I2C_CFG_MAX_BUSES] = {
    [DEV_I2C_BUS_SENSOR] = { DEV_I2C_BUS_SENSOR, 0, 21, 22, DEV_I2C_SPEED_FAST },
};
```

---

## 9. Build

```cmake
set(DEV_I2C_PORT "stm32" CACHE STRING "I2C port: mock, stm32, esp32")
add_subdirectory(drivers/dev_i2c)
target_link_libraries(${PROJECT_NAME} dev_i2c)
```

Host tests: `DEV_I2C_PORT=mock`.

---

## 10. Test Vectors

24 tests cover: init/deinit, validation (bus, addr, null, zero-len), write/read, write-read, mem read/write 8/16-bit, probe ok/nack, timeout, bus error, nack, recover, unsupported speed.

Run: `cd tests/dev_i2c/build && cmake .. && cmake --build . && ./i2c_test_host`
