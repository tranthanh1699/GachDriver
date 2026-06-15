# dev_ringbuf — SPSC Byte Ring Buffer

## 1. Overview

Fixed-size single-producer/single-consumer byte ring buffer. Caller provides static storage. Used by `dev_uart` for RX buffering (ISR pushes, main loop pops).

- **ISR-safe fast path**: `try_push()` / `try_pop()` return `bool`
- **Non-ISR path**: `write()` / `pop()` return `dev_err_t`
- **Usable capacity**: `capacity - 1` (one slot reserved)

---

## 2. Quick Start — How to Use

```c
#include "dev_ringbuf.h"

#define CAP  64U
static uint8_t      buf[CAP];
static dev_ringbuf_t rb;

/* ISR (producer) — fast, bool return */
void UART_ISR(void) {
    uint8_t byte = UART->DR;
    dev_ringbuf_try_push(&rb, byte);  /* silently drops if full */
}

/* Main loop (consumer) — dev_err_t return */
void main_loop(void) {
    uint8_t byte;
    if (dev_ringbuf_pop(&rb, &byte) == DEV_OK) {
        process(byte);
    }
}

int main(void) {
    dev_ringbuf_init(&rb, buf, CAP);
    /* ... */
}
```

---

## 3. Configuration

| Parameter | Meaning | Typical |
|-----------|---------|---------|
| `storage` | Static byte array | `uint8_t buf[64]` |
| `capacity` | Total array size (must be >= 2) | `64U` |

**Capacity note**: Usable space is `capacity - 1`. One slot is reserved to distinguish full from empty. A 64-byte buffer stores up to 63 bytes.

---

## 4. API Reference

| Function | ISR-safe | Return | Full/Empty |
|----------|----------|--------|------------|
| `init(rb, buf, cap)` | No | `dev_err_t` | — |
| `try_push(rb, byte)` | **Yes** | `bool` | `false` |
| `try_pop(rb, *out)` | **Yes** | `bool` | `false` |
| `write(rb, byte)` | No | `dev_err_t` | `DEV_ERR_BUSY` |
| `pop(rb, *out)` | No | `dev_err_t` | `DEV_ERR_BUSY` |
| `read(rb, data, len, *read_len)` | No | `dev_err_t` | Reads up to `len` |
| `available(rb)` | **Yes** | `size_t` | 0 if empty |
| `capacity(rb)` | **Yes** | `size_t` | Usable = cap - 1 |
| `free(rb)` | **Yes** | `size_t` | Free space |
| `flush(rb)` | No | `dev_err_t` | Clears all data |
| `is_valid(rb)` | **Yes** | `bool` | Validates context |

### Concurrency Contract

- **One** context writes `head` (producer: ISR or task)
- **One** context writes `tail` (consumer: another task)
- `flush()` writes `tail = head` — **disable the producer ISR** before calling
- On platforms where `size_t` > native word size, wrap in critical section

---

## 5. Porting — No Changes Needed

`dev_ringbuf` is pure C software with no hardware dependency. Works on any platform.

---

## 6. Build

```cmake
add_subdirectory(drivers/dev_ringbuf)
target_link_libraries(${PROJECT_NAME} dev_ringbuf)
```

Depends on `dev_common` for `dev_err_t` and `DEV_RETURN_ON_FALSE`.
