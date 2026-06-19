# dev_log — Lightweight UART Logging

## 1. Overview

Formatted logging over UART with ANSI color support. Uses `dev_uart` for output — no direct HAL dependency.

- **3 log functions**: `dev_log()` (printf-style), `dev_log_hex()` (hex dump)
- **Colored macros**: `DEV_LOG_ERR` (red), `DEV_LOG_WRN` (yellow), `DEV_LOG_INF` (green)
- **Per-module tag**: `CONFIG_LOG_TAG` adds module name + line number
- **Compile-out**: Define `CONFIG_LOG_DEFAULT_LEVEL_NONE` to strip all logging

## 2. Quick Start

```c
#include "dev_log.h"

int main(void)
{
    dev_log_init(DEV_UART_CONSOLE);  // picks UART, auto-inits dev_uart

    dev_log("System started at %lu MHz\r\n", SystemCoreClock / 1000000UL);

    uint8_t buf[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                       0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    dev_log_hex(buf, sizeof(buf));
}
```

## 3. API

| Function | Purpose |
|----------|---------|
| `dev_log_init(uart_id)` | Init logger on a UART (auto-inits `dev_uart` if needed) |
| `dev_log(fmt, ...)` | `printf`-style formatted output to UART |
| `dev_log_hex(data, len)` | Hex dump to UART |

### dev_log_init

```c
void dev_log_init(uint8_t uart_id);
```

Stores the UART ID and calls `dev_uart_init()` if not already initialized. Invalid UART IDs are silently ignored.

### dev_log

```c
void dev_log(const char *str, ...);
```

Formats args into a 512-byte internal buffer using `vsnprintf()`, then sends to UART. Blocking TX with default timeout.

### dev_log_hex

```c
void dev_log_hex(const uint8_t *data, uint32_t length);
```

Prints each byte as two hex characters followed by `\r\n`.

## 4. Colored Log Macros

Define `CONFIG_LOG_TAG` at the top of each module to enable per-module tagged logging:

```c
#include "dev_log.h"

CONFIG_LOG_TAG(SENSOR, true);  // tag = "SENSOR", enabled

void sensor_init(void)
{
    DEV_LOG_INF("Initializing...");   // [SENSOR-42]: Initializing...
    DEV_LOG_ERR("Calibration failed (%d)", code);
    DEV_LOG_WRN("Retry count: %d", n);
    DEV_LOG_RAW("raw message");
}
```

| Macro | Color | Format |
|-------|-------|--------|
| `DEV_LOG_ERR(msg, ...)` | Red | `[TAG-line]: msg` |
| `DEV_LOG_WRN(msg, ...)` | Yellow | `[TAG-line]: msg` |
| `DEV_LOG_INF(msg, ...)` | Green | `[TAG-line]: msg` |
| `DEV_LOG_RAW(msg, ...)` | None | `msg` (no prefix) |

ANSI colors:
- ERR: `\033[0;31m` (red)
- WRN: `\033[0;33m` (yellow)
- INF: `\033[0;32m` (green)
- Reset: `\033[0m` after each message

### Disabling logging

Define `CONFIG_LOG_DEFAULT_LEVEL_NONE` before including `dev_log.h` to compile out all logging:

```c
#define CONFIG_LOG_DEFAULT_LEVEL_NONE
#include "dev_log.h"
// All DEV_LOG_* macros become no-ops
```

### Tag naming

Each module can define its own tag:

```c
CONFIG_LOG_TAG(GPIO, true);     // GPIO module — enabled
CONFIG_LOG_TAG(I2C, false);     // I2C module — disabled
CONFIG_LOG_TAG(ADC, true);      // ADC module — enabled
```

## 5. Configuration

No separate cfg header. All configuration is through the API and macros:

| Setting | Where | Effect |
|---------|-------|--------|
| UART channel | `dev_log_init(uart_id)` | Which UART to use |
| Tag name | `CONFIG_LOG_TAG(name, enable)` | Module prefix in log output |
| Tag enabled | Second arg to `CONFIG_LOG_TAG` | `true` = log, `false` = silent |
| Global disable | `#define CONFIG_LOG_DEFAULT_LEVEL_NONE` | Strip all logging at compile time |
| Internal buffer | Hardcoded `512` | `vsnprintf` buffer size |

## 6. Build

```cmake
add_subdirectory(drivers/dev_log)
target_link_libraries(${PROJECT_NAME} dev_log)
```

Depends on: `dev_common`, `dev_uart`.

## 7. Design Notes

- Internal 512-byte buffer for `vsnprintf` — messages longer than 511 chars are truncated by `vsnprintf`
- TX is blocking with `DEV_UART_TIMEOUT_DEFAULT_MS` (100ms)
- No dynamic allocation
- No vendor headers
- ANSI color codes may not render on all terminals
