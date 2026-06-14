# dev_uart — Simplified UART Wrapper

## 1. Overview

Hardware-independent UART driver using logical UART IDs. Application code never includes vendor HAL headers.

- **TX**: blocking write with timeout via HAL
- **RX**: non-blocking buffered read via `dev_ringbuf`
- **STM32**: wraps existing Cube-generated HAL handles (`huart1`, `huart2`...) — no GPIO/clock/NVIC init
- **Timeout**: TX blocks until complete or timeout. RX reads from buffer immediately; `DEV_UART_TIMEOUT_NO_WAIT` returns `DEV_ERR_EMPTY`

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

## 3. STM32 Cube-Managed Mode

The STM32 port assumes CubeMX/CubeIDE has already configured:
- GPIO alternate function for TX/RX
- UART peripheral clock
- NVIC priority
- HAL UART init (`huart1`, `huart2`...)

The port only wraps handles and manages RX buffering via `dev_ringbuf`.

### Enable Steps
1. Uncomment `#define HAL_UART_MODULE_ENABLED` in `Core/Inc/stm32h7xx_hal_conf.h`
2. Add `stm32h7xx_hal_uart.c` to `cmake/stm32cubemx/CMakeLists.txt`
3. In `Core/Src/stm32h7xx_it.c`, add:
```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    dev_uart_port_stm32_rx_cplt_callback(huart);
}
```
4. Ensure `huart1`, `huart2` are declared `extern` in your project

### STM32 UART Map (`dev_uart_port_stm32.c`)
```c
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

static const dev_uart_hw_t s_uart_map[] = {
    [DEV_UART_CONSOLE] = { DEV_UART_CONSOLE, &huart1, 115200, rx_buf, 256 },
    [DEV_UART_GNSS]    = { DEV_UART_GNSS,    &huart2, 9600,   rx_buf2, 512 },
};
```

## 4. RX Buffering Model

```
HAL_UART_RxCpltCallback → dev_uart_port_stm32_rx_cplt_callback
                        → dev_ringbuf_write(rx_ring, byte)
                        → HAL_UART_Receive_IT(re-arm)
dev_uart_read()         → dev_ringbuf_pop(rx_ring, &byte) → return to app
```

## 5. API

| Function | Blocking | Key Returns |
|----------|----------|-------------|
| `init()` / `deinit()` | — | `DEV_OK`, `DEV_ERR_ALREADY_INITIALIZED`, `DEV_ERR_NOT_INITIALIZED` |
| `config(uart, baud, data, stop, parity, flow)` | — | `DEV_OK`, `DEV_ERR_NOT_SUPPORTED` (flow ctl) |
| `write(uart, data, len, timeout)` | Yes | `DEV_OK`, `DEV_ERR_TIMEOUT` |
| `read(uart, data, len, *read_len, timeout)` | No | `DEV_OK`, `DEV_ERR_EMPTY` |
| `read_byte(uart, *byte, timeout)` | No | `DEV_OK`, `DEV_ERR_EMPTY` |
| `write_string(uart, str, timeout)` | Yes | `DEV_OK`, truncates at `DEV_UART_CFG_MAX_STRING_LENGTH` (256) |
| `rx_available(uart)` | No | byte count from ringbuf |
| `rx_start(uart)` / `rx_stop(uart)` | — | start/stop HAL UART RX interrupt |
| `flush_rx(uart)` / `flush_tx(uart)` | — | clear ringbuf |

## 6. Build

```cmake
set(DEV_UART_PORT "stm32" CACHE STRING "UART port: mock, stm32, esp32, nrf52")
add_subdirectory(drivers/dev_uart)
target_link_libraries(${PROJECT_NAME} dev_uart)
```

## 7. Porting

Each port implements 11 functions from `dev_uart_port.h`. STM32 uses Cube handles + `dev_ringbuf`. Mock uses `dev_ringbuf`. ESP32/nRF52 are stubs returning `DEV_ERR_NOT_SUPPORTED`.
