# dev_crc — CRC-8/16/32 Computation

## 1. Overview

Hardware-independent CRC computation with init/update/final streaming API and one-shot `compute()` convenience.

| Algorithm | Poly | Init | XorOut | Check ("123456789") |
|-----------|------|------|--------|---------------------|
| CRC-8 | 0x07 | 0x00 | 0x00 | `0xF4` |
| CRC-16/MODBUS | 0x8005 (refl 0xA001) | 0xFFFF | 0x0000 | `0x4B37` |
| CRC-32/IEEE 802.3 | 0x04C11DB7 (refl 0xEDB88320) | 0xFFFFFFFF | 0xFFFFFFFF | `0xCBF43926` |

---

## 2. Quick Start

```c
#include "dev_crc.h"

uint8_t crc;
dev_crc8_compute(data, len, &crc);     // one-shot CRC-8

uint16_t crc16;
dev_crc16_compute(data, len, &crc16);  // one-shot CRC-16/MODBUS

uint32_t crc32;
dev_crc32_compute(data, len, &crc32);  // one-shot CRC-32/IEEE
```

---

## 3. Types

```c
typedef struct { uint8_t  value; } dev_crc8_ctx_t;
typedef struct { uint16_t value; } dev_crc16_ctx_t;
typedef struct { uint32_t value; } dev_crc32_ctx_t;
```

---

## 4. API Reference

### 4.1 Streaming API (init → update → final)

```c
dev_err_t dev_crc8_init(dev_crc8_ctx_t *ctx);
dev_err_t dev_crc8_update(dev_crc8_ctx_t *ctx, const uint8_t *data, size_t len);
dev_err_t dev_crc8_final(const dev_crc8_ctx_t *ctx, uint8_t *out_crc);
```

Same pattern for `dev_crc16_*` and `dev_crc32_*`.

### 4.2 One-shot API

```c
dev_err_t dev_crc8_compute(const uint8_t *data, size_t len, uint8_t *out_crc);
dev_err_t dev_crc16_compute(const uint8_t *data, size_t len, uint16_t *out_crc);
dev_err_t dev_crc32_compute(const uint8_t *data, size_t len, uint32_t *out_crc);
```

### 4.3 Return values

| Condition | Error |
|-----------|-------|
| Success | `DEV_OK` |
| NULL ctx, NULL data, or NULL out_crc | `DEV_ERR_NULL_PTR` |

Zero-length input is valid — returns the initial value (CRC-8: `0x00`, CRC-16: `0xFFFF`, CRC-32: `0x00000000`).

---

## 5. Usage Patterns

### 5.1 Streaming (data arrives in chunks)

```c
dev_crc32_ctx_t ctx;
dev_crc32_init(&ctx);

while (more_data()) {
    dev_crc32_update(&ctx, chunk, chunk_len);
}

uint32_t crc;
dev_crc32_final(&ctx, &crc);
```

### 5.2 One-shot

```c
uint16_t crc;
dev_err_t err = dev_crc16_compute(packet, packet_len, &crc);
if (err == DEV_OK) { /* crc holds the result */ }
```

---

## 6. Build

```cmake
add_subdirectory(drivers/dev_common)
add_subdirectory(drivers/dev_crc)
target_link_libraries(${PROJECT_NAME} dev_crc)
```

Depends on `dev_common` for `dev_err_t` and `DEV_CHECK_RET`.

---

## 7. Test Vectors

| Test | Input | Result |
|------|-------|--------|
| CRC-8 | `"123456789"` (9 bytes) | `0xF4` |
| CRC-16/MODBUS | `"123456789"` (9 bytes) | `0x4B37` |
| CRC-32/IEEE 802.3 | `"123456789"` (9 bytes) | `0xCBF43926` |
| CRC-8 (zero len) | `""` (0 bytes) | `0x00` |
| CRC-16 (zero len) | `""` (0 bytes) | `0xFFFF` |
| CRC-32 (zero len) | `""` (0 bytes) | `0x00000000` |

Streaming update (split into 3 chunks) produces the same result as one-shot.
