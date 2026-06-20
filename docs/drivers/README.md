# Drivers Layer

## Overview

Drivers are low-level hardware wrappers. They abstract vendor HALs and SDKs behind `dev_*` APIs so application and service code never includes vendor headers directly.

Drivers shall not depend on services or application code.

## Layer Rules

- Drivers expose `dev_*` APIs.
- Drivers may use target ports such as STM32, ESP32, nRF52.
- Drivers shall not depend on service modules (`svc_*`).
- Drivers shall not depend on application code.
- Drivers shall not call `svc_*` APIs.
- Vendor HAL / SDK includes are allowed only in port layer files.

## Components

| Component | Description | Documentation |
|-----------|-------------|---------------|
| `dev_common` | Foundation: types, errors, assert, compiler helpers | [../dev_common/README.md](../dev_common/README.md) |
| `dev_gpio` | GPIO wrapper | [../dev_gpio/README.md](../dev_gpio/README.md) |
| `dev_i2c` | I2C wrapper | [../dev_i2c/README.md](../dev_i2c/README.md) |
| `dev_uart` | UART wrapper | [../dev_uart/README.md](../dev_uart/README.md) |
| `dev_adc` | ADC wrapper | [../dev_adc/README.md](../dev_adc/README.md) |
| `dev_crc` | CRC-8/16/32 computation | [../dev_crc/README.md](../dev_crc/README.md) |
| `dev_list` | Fixed-capacity singly-linked list | [../dev_list/README.md](../dev_list/README.md) |
| `dev_ringbuf` | SPSC byte ring buffer | [../dev_ringbuf/README.md](../dev_ringbuf/README.md) |
| `dev_log` | UART logging with colored macros | [../dev_log/README.md](../dev_log/README.md) |

## Port Layer

Each driver has a `port/<target>/` directory. The port layer isolates all vendor-specific code.

Supported targets:
- `stm32` — STM32 HAL / LL
- `esp32` — ESP-IDF
- `nrf52` — nRF5 SDK / nrfx
- `mock` — host-based testing (no hardware required)

## Adding a New Driver

1. Create `drivers/dev_<name>/include/` and `drivers/dev_<name>/src/`.
2. Use the `dev_` prefix for all public symbols.
3. Create `drivers/dev_<name>/port/<target>/` for each hardware target.
4. Create `drivers/dev_<name>/CMakeLists.txt`.
5. Create `docs/dev_<name>/README.md`.
6. Update this file and the root `README.md`.
