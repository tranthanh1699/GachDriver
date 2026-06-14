# dev_uart — Simplified UART Wrapper

## 1. Overview

Hardware-independent UART driver using logical UART IDs. Application code never includes vendor HAL headers.

- **TX**: blocking write with timeout via HAL
- **RX**: non-blocking buffered read via `dev_ringbuf`
- **STM32**: wraps existing Cube-generated HAL handles (`huart1`, `huart2`...) — no GPIO/clock/NVIC init performed by the driver
- **Timeout**: TX blocks until complete or timeout. RX reads from buffer immediately; `DEV_UART_TIMEOUT_NO_WAIT` returns `DEV_ERR_EMPTY`

---

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

---

## 3. Adding a New UART — Complete Step-by-Step

This guide walks through adding a 4th UART (`DEV_UART_DEBUG`) on USART6 with TX=PC6, RX=PC7.

### Step 1: Enable UART HAL in CubeMX

In `Core/Inc/stm32h7xx_hal_conf.h`, uncomment:

```c
#define HAL_UART_MODULE_ENABLED
```

CubeMX must generate `stm32h7xx_hal_uart.c` into the `Drivers/STM32H7xx_HAL_Driver/Src/` folder. If it's missing, regenerate code from CubeMX with USART6 enabled.

### Step 2: Configure USART6 in CubeMX

In CubeMX, enable USART6 with:
- Mode: Asynchronous
- TX: PC6, RX: PC7
- Baud Rate: 115200
- Word Length: 8 bits
- Parity: None
- Stop Bits: 1
- NVIC: Enable USART6 global interrupt

CubeMX generates `huart6` handle in `Core/Src/usart.c` or `Core/Src/main.c`.

### Step 3: Declare the Cube handle as extern

In `drivers/dev_uart/port/stm32/dev_uart_port_stm32.c`, add:

```c
extern UART_HandleTypeDef huart6;   // <-- add this line
```

### Step 4: Add logical UART ID and buffer

In `drivers/dev_uart/include/dev_uart_cfg.h`:

```c
#define DEV_UART_DEBUG                        ((dev_uart_id_t)3U)
#define DEV_UART_DEBUG_RX_BUFFER_SIZE         (512U)
```

Also increase `DEV_UART_CFG_MAX_INSTANCES` if needed:

```c
#define DEV_UART_CFG_MAX_INSTANCES             (4U)   // was 3U, now 4U
```

### Step 5: Add static RX buffer

In `drivers/dev_uart/port/stm32/dev_uart_port_stm32.c`:

```c
static uint8_t s_debug_rx_buf[DEV_UART_DEBUG_RX_BUFFER_SIZE];  // <-- add
```

### Step 6: Add entry to UART map

In the same file, inside `s_uart_map[]`:

```c
static const dev_uart_hw_t s_uart_map[DEV_UART_CFG_MAX_INSTANCES] = {
    [DEV_UART_CONSOLE] = { DEV_UART_CONSOLE, &huart1,
        DEV_UART_BAUDRATE_115200, s_console_rx_buf, DEV_UART_CONSOLE_RX_BUFFER_SIZE },
    [DEV_UART_GNSS]    = { DEV_UART_GNSS, &huart2,
        DEV_UART_BAUDRATE_9600, s_gnss_rx_buf, DEV_UART_GNSS_RX_BUFFER_SIZE },
    [DEV_UART_MODEM]   = { DEV_UART_MODEM, &huart3,
        DEV_UART_BAUDRATE_115200, s_modem_rx_buf, DEV_UART_MODEM_RX_BUFFER_SIZE },
    [DEV_UART_DEBUG]   = { DEV_UART_DEBUG, &huart6,                     // <-- add
        DEV_UART_BAUDRATE_115200, s_debug_rx_buf, DEV_UART_DEBUG_RX_BUFFER_SIZE },
};
```

### Step 7: Wire ISR callback

In `Core/Src/stm32h7xx_it.c`, the existing callback already dispatches to all UARTs:

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    dev_uart_port_stm32_rx_cplt_callback(huart);
}
```

No changes needed here — `dev_uart_port_stm32_rx_cplt_callback()` loops through all entries in `s_uart_map[]` and matches the handle automatically.

### Step 8: Verify stm32cubemx CMake includes the UART source

In `cmake/stm32cubemx/CMakeLists.txt`, confirm `stm32h7xx_hal_uart.c` is in the `STM32_Drivers` library source list. If not, add:

```cmake
${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
```

### Step 9: Rebuild

```bash
# VS Code: Ctrl+Shift+B, or terminal:
cube-cmake --build build/Debug
```

### Step 10: Use in application

```c
#include "dev_uart.h"

dev_uart_init();
dev_uart_write_string(DEV_UART_DEBUG, "Debug UART active\r\n", DEV_UART_TIMEOUT_DEFAULT_MS);

uint8_t byte;
if (dev_uart_read_byte(DEV_UART_DEBUG, &byte, DEV_UART_TIMEOUT_NO_WAIT) == DEV_OK) {
    /* process debug input */
}
```

### Summary: Files to modify

| # | File | Change |
|---|------|--------|
| 1 | `Core/Inc/stm32h7xx_hal_conf.h` | Uncomment `HAL_UART_MODULE_ENABLED` |
| 2 | CubeMX | Enable USART6, generate code |
| 3 | `dev_uart_port_stm32.c` | Add `extern UART_HandleTypeDef huart6` |
| 4 | `dev_uart_cfg.h` | Add `DEV_UART_DEBUG` ID + buffer size macro |
| 5 | `dev_uart_cfg.h` | Bump `DEV_UART_CFG_MAX_INSTANCES` to 4 |
| 6 | `dev_uart_port_stm32.c` | Add `static uint8_t s_debug_rx_buf[...]` |
| 7 | `dev_uart_port_stm32.c` | Add entry `[DEV_UART_DEBUG] = {...}` to `s_uart_map[]` |
| 8 | `cmake/stm32cubemx/CMakeLists.txt` | Verify `stm32h7xx_hal_uart.c` is listed |

No changes needed in:
- `stm32h7xx_it.c` — the callback already dispatches automatically
- `dev_uart.c` — the common wrapper validates via `DEV_UART_CFG_MAX_INSTANCES`
- `dev_uart_port.h` — port interface stays the same

---

## 4. Removing a UART

1. Remove the entry from `s_uart_map[]` in `dev_uart_port_stm32.c`
2. Remove the `#define DEV_UART_XXXX` from `dev_uart_cfg.h`
3. Optionally reduce `DEV_UART_CFG_MAX_INSTANCES`

---

## 5. Configuration Reference (`dev_uart_cfg.h`)

```c
#define DEV_UART_CFG_MAX_INSTANCES             (3U)     // max UARTs, bump when adding
#define DEV_UART_CFG_RX_BUFFER_ENABLED         (1U)     // 1U = auto-start RX on init
#define DEV_UART_TIMEOUT_DEFAULT_MS            (100U)
#define DEV_UART_TIMEOUT_NO_WAIT               (0U)
#define DEV_UART_CFG_MAX_STRING_LENGTH         (256U)   // write_string() truncation limit

#define DEV_UART_CONSOLE                ((dev_uart_id_t)0U)
#define DEV_UART_GNSS                   ((dev_uart_id_t)1U)
#define DEV_UART_MODEM                  ((dev_uart_id_t)2U)
```

---

## 6. API Reference

| Function | Blocking | Key Returns |
|----------|----------|-------------|
| `init()` / `deinit()` | — | `DEV_OK`, `DEV_ERR_ALREADY_INITIALIZED`, `DEV_ERR_NOT_INITIALIZED` |
| `config(uart, baud, data, stop, parity, flow)` | — | `DEV_OK`, `DEV_ERR_NOT_SUPPORTED` (flow ctl) |
| `set_baudrate(uart, baud)` | — | `DEV_OK` |
| `write(uart, data, len, timeout)` | Yes | `DEV_OK`, `DEV_ERR_TIMEOUT`, `DEV_ERR_HW_FAILURE` |
| `write_byte(uart, byte, timeout)` | Yes | Same as write |
| `write_string(uart, str, timeout)` | Yes | Truncates at `MAX_STRING_LENGTH` (256) |
| `read(uart, data, len, *read_len, timeout)` | No | `DEV_OK`, `DEV_ERR_EMPTY` |
| `read_byte(uart, *byte, timeout)` | No | `DEV_OK`, `DEV_ERR_EMPTY` |
| `rx_available(uart)` | No | Byte count in ring buffer |
| `rx_start(uart)` / `rx_stop(uart)` | — | Start/stop HAL UART RX interrupt |
| `flush_rx(uart)` / `flush_tx(uart)` | — | Clear ring buffer |

---

## 7. RX Buffering Model

```
HAL_UART_RxCpltCallback (in stm32h7xx_it.c)
  → dev_uart_port_stm32_rx_cplt_callback(huart)
    → dev_ringbuf_write(rx_ring, byte)     // ISR pushes byte
    → HAL_UART_Receive_IT(re-arm)          // keep listening

dev_uart_read() → dev_ringbuf_pop(rx_ring, &byte) → return to app
```

On `dev_uart_init()` with `DEV_UART_CFG_RX_BUFFER_ENABLED == 1U`, RX interrupt starts automatically for all UARTs in the map.

---

## 8. Build

```cmake
set(DEV_UART_PORT "stm32" CACHE STRING "UART port: mock, stm32, esp32, nrf52")
add_subdirectory(drivers/dev_uart)
target_link_libraries(${PROJECT_NAME} dev_uart)
```

Host tests: `DEV_UART_PORT=mock`. 23 tests pass.
