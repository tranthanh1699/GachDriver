# dev_uart — Simplified UART Wrapper

## 1. Overview

Hardware-independent UART driver using logical UART IDs. Application code never includes vendor HAL headers.

- **TX**: blocking write with timeout
- **RX**: non-blocking buffered read (reads from `dev_ringbuf` on STM32, ESP-IDF internal buffer on ESP32)
- **Timeout semantics**: TX blocks until complete or timeout. RX reads from buffer immediately; `DEV_UART_TIMEOUT_NO_WAIT` returns `DEV_ERR_EMPTY` if no data, `DEV_UART_TIMEOUT_FOREVER` is reserved for future async use.

## 2. Quick Start

```c
#include "dev_uart.h"

dev_uart_init();
dev_uart_write_string(DEV_UART_CONSOLE, "Hello\r\n", DEV_UART_TIMEOUT_DEFAULT_MS);

uint8_t byte;
if (dev_uart_read_byte(DEV_UART_CONSOLE, &byte, DEV_UART_TIMEOUT_NO_WAIT) == DEV_OK) {
    /* got a byte */
}
```

## 3. API

| Function | Blocking | Returns |
|----------|----------|---------|
| `init()` / `deinit()` | — | `dev_err_t` |
| `config(uart, baud, data, stop, parity, flow)` | — | `dev_err_t` |
| `write(uart, data, len, timeout)` | Yes | `dev_err_t` |
| `write_byte(uart, byte, timeout)` | Yes | `dev_err_t` |
| `write_string(uart, str, timeout)` | Yes | `dev_err_t` (truncates at `DEV_UART_CFG_MAX_STRING_LENGTH`) |
| `read(uart, data, len, *read_len, timeout)` | No | `dev_err_t`, `DEV_ERR_EMPTY` if no data with `NO_WAIT` |
| `read_byte(uart, *byte, timeout)` | No | `dev_err_t` |
| `rx_available(uart)` | No | `uint16_t` byte count |
| `flush_rx(uart)` / `flush_tx(uart)` | — | `dev_err_t` |
| `rx_start(uart)` / `rx_stop(uart)` | — | `dev_err_t` |

## 4. RX Model

RX is **non-blocking buffered**: UART ISR pushes bytes into a ring buffer; `dev_uart_read()` pulls from the buffer without blocking. Use `dev_uart_rx_available()` to check before reading, or pass `DEV_UART_TIMEOUT_NO_WAIT` to avoid busy-wait.

## 5. STM32 Enable Steps

1. Uncomment `#define HAL_UART_MODULE_ENABLED` in `Core/Inc/stm32h7xx_hal_conf.h`
2. Add `stm32h7xx_hal_uart.c` to the STM32_Drivers library in `cmake/stm32cubemx/CMakeLists.txt`
3. Rebuild

## 6. Build

```cmake
set(DEV_UART_PORT "stm32" CACHE STRING "UART port: mock, stm32, esp32, nrf52")
add_subdirectory(drivers/dev_uart)
target_link_libraries(${PROJECT_NAME} dev_uart)
```

## 7. Porting

Each port implements 11 functions from `dev_uart_port.h`. STM32 uses `dev_ringbuf` for RX. ESP32 stub returns `DEV_ERR_NOT_SUPPORTED` — complete the stub with ESP-IDF UART driver calls.
