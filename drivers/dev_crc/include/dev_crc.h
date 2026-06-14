#ifndef DEV_CRC_H
#define DEV_CRC_H

#include "dev_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CRC-8 calculation context.
 */
typedef struct
{
    uint8_t value;
} dev_crc8_ctx_t;

/**
 * @brief CRC-16 calculation context.
 */
typedef struct
{
    uint16_t value;
} dev_crc16_ctx_t;

/**
 * @brief CRC-32 calculation context.
 */
typedef struct
{
    uint32_t value;
} dev_crc32_ctx_t;

/**
 * @brief Initialize CRC-8 context (poly 0x07, init 0x00, xor_out 0x00).
 *
 * @param ctx CRC context.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if ctx is NULL.
 */
dev_err_t dev_crc8_init(dev_crc8_ctx_t *ctx);

/**
 * @brief Append bytes into CRC-8 context.
 *
 * @param ctx CRC context.
 * @param data Input buffer.
 * @param len Input buffer length in bytes.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc8_update(dev_crc8_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief Finalize CRC-8 and return result.
 *
 * @param ctx CRC context.
 * @param out_crc Output CRC value.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc8_final(const dev_crc8_ctx_t *ctx, uint8_t *out_crc);

/**
 * @brief Compute CRC-8 (poly 0x07, init 0x00, xor_out 0x00).
 *
 * @param data Input buffer.
 * @param len Input buffer length in bytes.
 * @param out_crc Output CRC value.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc8_compute(const uint8_t *data, size_t len, uint8_t *out_crc);

/**
 * @brief Initialize CRC-16 context (poly 0xA001 reflected, init 0xFFFF, xor_out 0x0000).
 *
 * @param ctx CRC context.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc16_init(dev_crc16_ctx_t *ctx);

/**
 * @brief Append bytes into CRC-16 context.
 *
 * @param ctx CRC context.
 * @param data Input buffer.
 * @param len Input buffer length in bytes.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc16_update(dev_crc16_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief Finalize CRC-16 and return result.
 *
 * @param ctx CRC context.
 * @param out_crc Output CRC value.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc16_final(const dev_crc16_ctx_t *ctx, uint16_t *out_crc);

/**
 * @brief Compute CRC-16/MODBUS (poly 0xA001 reflected, init 0xFFFF, xor_out 0x0000).
 *
 * @param data Input buffer.
 * @param len Input buffer length in bytes.
 * @param out_crc Output CRC value.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc16_compute(const uint8_t *data, size_t len, uint16_t *out_crc);

/**
 * @brief Initialize CRC-32 context (poly 0xEDB88320 reflected, init 0xFFFFFFFF, xor_out 0xFFFFFFFF).
 *
 * @param ctx CRC context.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc32_init(dev_crc32_ctx_t *ctx);

/**
 * @brief Append bytes into CRC-32 context.
 *
 * @param ctx CRC context.
 * @param data Input buffer.
 * @param len Input buffer length in bytes.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc32_update(dev_crc32_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief Finalize CRC-32 and return result.
 *
 * @param ctx CRC context.
 * @param out_crc Output CRC value.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc32_final(const dev_crc32_ctx_t *ctx, uint32_t *out_crc);

/**
 * @brief Compute CRC-32/IEEE 802.3 (poly 0xEDB88320 reflected, init 0xFFFFFFFF, xor_out 0xFFFFFFFF).
 *
 * @param data Input buffer.
 * @param len Input buffer length in bytes.
 * @param out_crc Output CRC value.
 * @return DEV_OK on success, or DEV_ERR_NULL_PTR if a required pointer is NULL.
 */
dev_err_t dev_crc32_compute(const uint8_t *data, size_t len, uint32_t *out_crc);

#ifdef __cplusplus
}
#endif

#endif /* DEV_CRC_H */