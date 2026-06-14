# dev_ringbuf — SPSC Byte Ring Buffer

## 1. Overview

Fixed-size single-producer/single-consumer byte ring buffer for embedded use. No dynamic allocation — caller provides the storage.

- **ISR-safe fast path**: `try_push()` / `try_pop()` return `bool`, no error detail
- **Non-ISR path**: `write()` / `pop()` return `dev_err_t` with specific error codes
- **Usable capacity**: `capacity - 1` (one slot reserved to distinguish full from empty)
- **Concurrency**: SPSC contract — one writer of `head`, one writer of `tail`

---

## 2. Quick Start

```c
#include "dev_ringbuf.h"

static uint8_t      rx_buf[64];
static dev_ringbuf_t rx_ring;

// Init
dev_ringbuf_init(&rx_ring, rx_buf, sizeof(rx_buf));

// ISR (producer): fast, bool return
void UART_IRQHandler(void) {
    uint8_t byte = UART->DR;
    dev_ringbuf_try_push(&rx_ring, byte);  // silently drops if full
}

// Main loop (consumer): dev_err_t return
void main_loop(void) {
    uint8_t byte;
    if (dev_ringbuf_pop(&rx_ring, &byte) == DEV_OK) {
        process(byte);
    }
}
```

---

## 3. API

| Function | ISR-safe | Return | Full/Empty behavior |
|----------|----------|--------|---------------------|
| `try_push(r, v)` | Yes | `bool` | `false` if full or invalid |
| `try_pop(r, *v)` | Yes | `bool` | `false` if empty or invalid |
| `write(r, v)` | No | `dev_err_t` | `DEV_ERR_BUSY` if full, `DEV_ERR_INVALID_ARG` if invalid |
| `pop(r, *v)` | No | `dev_err_t` | `DEV_ERR_BUSY` if empty, `DEV_ERR_NULL_PTR` if null out |
| `read(r, data, len, *n)` | No | `dev_err_t` | Reads up to `len` bytes, sets `*n` to actual count |
| `available(r)` | Yes | `size_t` | 0 if invalid or empty |
| `capacity(r)` | Yes | `size_t` | Usable capacity = `capacity - 1` |
| `free(r)` | Yes | `size_t` | Free space = `capacity - available` |
| `flush(r)` | No | `dev_err_t` | Drops all data. Caller must ensure no concurrent push/pop |
| `is_valid(r)` | Yes | `bool` | Validates pointer, capacity ≥ 2, head/tail < capacity |

---

## 4. Concurrency Contract

- One context writes `head` (producer: ISR or task)
- One context writes `tail` (consumer: ISR or task)
- `flush()` writes `tail = head` — disable the producer ISR before calling
- On platforms where `size_t` is wider than native word size, wrap in critical section

---

## 5. Build

```cmake
add_subdirectory(drivers/dev_ringbuf)
target_link_libraries(${PROJECT_NAME} dev_ringbuf)
```

14/14 tests pass.
