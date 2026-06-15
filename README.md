# GachDriver — Embedded Driver Abstraction

Hardware-independent driver layer for STM32, ESP32, nRF52, and future MCUs.

## Architecture

```
Application → dev_* public API → common implementation → port interface → vendor HAL
```

- Application code never includes vendor HAL headers
- Logical IDs abstract away hardware pins/peripherals
- Port layer isolates all vendor-specific code
- No dynamic memory allocation, static configuration

## Components

| Component | Description | Documentation |
|-----------|-------------|---------------|
| `dev_common` | Types, errors, assert, compiler helpers | [docs/dev_common/README.md](docs/dev_common/README.md) |
| `dev_gpio` | GPIO wrapper (X-Macro pin definition) | [docs/dev_gpio/README.md](docs/dev_gpio/README.md) |
| `dev_i2c` | I2C wrapper (logical bus, 7-bit addr) | [docs/dev_i2c/README.md](docs/dev_i2c/README.md) |
| `dev_crc` | CRC-8/16/32 computation | [docs/dev_crc/README.md](docs/dev_crc/README.md) |
| `dev_list` | Fixed-capacity singly-linked list | [docs/dev_list/README.md](docs/dev_list/README.md) |
| `dev_ringbuf` | SPSC byte ring buffer | [docs/dev_ringbuf/README.md](docs/dev_ringbuf/README.md) |
| `dev_uart` | UART wrapper (logical ID, RX buffering) | [docs/dev_uart/README.md](docs/dev_uart/README.md) |
| `dev_shell` | UART command shell | [docs/dev_shell/README.md](docs/dev_shell/README.md) |
| `dev_adc` | ADC wrapper (logical channel, raw/mV, average) | [docs/dev_adc/README.md](docs/dev_adc/README.md) |

## Quick Start

```c
#include "dev_gpio.h"
#include "dev_i2c.h"

int main(void) {
    dev_gpio_init();                                    // init GPIO
    dev_gpio_output(DEV_GPIO_LED_GREEN);                 // configure LED
    dev_gpio_high(DEV_GPIO_LED_GREEN);                   // turn on

    dev_i2c_init();                                     // init I2C
    uint8_t data[2];
    dev_i2c_mem_read(DEV_I2C_BUS_SENSOR, 0x48U, 0x00U,  // read sensor
                     DEV_I2C_MEM_ADDR_SIZE_8BIT, data, 2U, 100U);
}
```

## Building

```cmake
set(DEV_GPIO_PORT "stm32"  CACHE STRING "GPIO port: mock, stm32")
set(DEV_I2C_PORT  "stm32"  CACHE STRING "I2C port: mock, stm32, esp32")
add_subdirectory(drivers/dev_common)
add_subdirectory(drivers/dev_gpio)
add_subdirectory(drivers/dev_i2c)
# ... etc
target_link_libraries(${PROJECT_NAME} dev_common dev_gpio dev_i2c ...)
```

## Folder Structure

```
drivers/
  dev_common/     Foundation (types, errors, assert)
  dev_gpio/       GPIO driver
  dev_i2c/        I2C driver
  dev_crc/        CRC computation
  dev_list/       Linked list container
  dev_ringbuf/    SPSC byte ring buffer
tests/
  dev_gpio/       34 host tests
  dev_i2c/        24 host tests
  dev_crc/        13 host tests
  dev_list/       27 host tests
docs/
  dev_common/     Library reference
  dev_gpio/       Library reference + porting guide
  dev_i2c/        Library reference + porting guide
  dev_crc/        Library reference
  dev_list/       Library reference
```

## Porting

Each driver has a `port/<target>/` directory. Select the port via CMake:

```cmake
set(DEV_GPIO_PORT "stm32")   # or "mock" for host testing
set(DEV_I2C_PORT  "stm32")   # or "mock", "esp32"
```

Host tests always use `mock` port — no hardware required.
