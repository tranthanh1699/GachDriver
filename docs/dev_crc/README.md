# dev_crc — CRC-8/16/32 Computation

## 1. Overview

Hardware-independent CRC computation with streaming API (init→update→final) and one-shot `compute()`.

| Algorithm | Poly | Init | XorOut | Check ("123456789") |
|-----------|------|------|--------|---------------------|
| CRC-8 | 0x07 | 0x00 | 0x00 | `0xF4` |
| CRC-16/MODBUS | 0x8005 (refl 0xA001) | 0xFFFF | 0x0000 | `0x4B37` |
| CRC-32/IEEE 802.3 | 0x04C11DB7 (refl 0xEDB88320) | 0xFFFFFFFF | 0xFFFFFFFF | `0xCBF43926` |

---

## 2. Quick Start — How to Use

```c
#include "dev_crc.h"

/* One-shot */
uint16_t crc16;
dev_crc16_compute(data, len, &crc16);

uint32_t crc32;
dev_crc32_compute(data, len, &crc32);

uint8_t crc8;
dev_crc8_compute(data, len, &crc8);
```

### Streaming (data arrives in chunks)

```c
dev_crc32_ctx_t ctx;
dev_crc32_init(&ctx);

while (more_data()) {
    dev_crc32_update(&ctx, chunk, chunk_len);
}

uint32_t crc;
dev_crc32_final(&ctx, &crc);
```

---

## 3. Configuration

No compile-time configuration needed. The polynomial, init value, and xor-out are fixed per algorithm:

| Setting | CRC-8 | CRC-16 | CRC-32 |
|---------|-------|--------|--------|
| Polynomial | `0x07` | `0xA001` (refl) | `0xEDB88320` (refl) |
| Init value | `0x00` | `0xFFFF` | `0xFFFFFFFF` |
| XorOut | `0x00` | `0x0000` | `0xFFFFFFFF` |
| Zero-length result | `0x00` | `0xFFFF` | `0x00000000` |

These are defined as `DEV_CRC8_POLY`, `DEV_CRC16_POLY`, `DEV_CRC32_POLY` in `dev_crc.c`. To change the polynomial (e.g., CRC-16/CCITT instead of MODBUS), edit those constants and update the init/final functions.

---

## 4. API Reference

```c
/* Streaming */
dev_err_t dev_crc8_init(dev_crc8_ctx_t *ctx);
dev_err_t dev_crc8_update(dev_crc8_ctx_t *ctx, const uint8_t *data, size_t len);
dev_err_t dev_crc8_final(const dev_crc8_ctx_t *ctx, uint8_t *out_crc);

dev_err_t dev_crc16_init(dev_crc16_ctx_t *ctx);
dev_err_t dev_crc16_update(dev_crc16_ctx_t *ctx, const uint8_t *data, size_t len);
dev_err_t dev_crc16_final(const dev_crc16_ctx_t *ctx, uint16_t *out_crc);

dev_err_t dev_crc32_init(dev_crc32_ctx_t *ctx);
dev_err_t dev_crc32_update(dev_crc32_ctx_t *ctx, const uint8_t *data, size_t len);
dev_err_t dev_crc32_final(const dev_crc32_ctx_t *ctx, uint32_t *out_crc);

/* One-shot */
dev_err_t dev_crc8_compute(const uint8_t *data, size_t len, uint8_t *out_crc);
dev_err_t dev_crc16_compute(const uint8_t *data, size_t len, uint16_t *out_crc);
dev_err_t dev_crc32_compute(const uint8_t *data, size_t len, uint32_t *out_crc);
```

| Condition | Error |
|-----------|-------|
| Success | `DEV_OK` |
| NULL ctx, NULL data, NULL out_crc | `DEV_ERR_NULL_PTR` |

---

## 5. Porting — No Changes Needed

`dev_crc` is pure C software with no hardware dependency. No port layer. Works on any platform.

---

## 6. Test Vectors

| Input | CRC-8 | CRC-16 | CRC-32 |
|-------|-------|--------|--------|
| `"123456789"` (9B) | `0xF4` | `0x4B37` | `0xCBF43926` |
| `""` (0B) | `0x00` | `0xFFFF` | `0x00000000` |
| Streaming (split) | Same | Same | Same |

---

## 7. Build

```cmake
add_subdirectory(drivers/dev_crc)
target_link_libraries(${PROJECT_NAME} dev_crc)
```

Depends on `dev_common` for `dev_err_t`, `DEV_RETURN_ON_FALSE`, `DEV_CHECK_RET`.
