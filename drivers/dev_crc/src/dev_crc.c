#include "dev_crc.h"

#define DEV_CRC8_POLY  (0x07U)
#define DEV_CRC16_POLY (0xA001U)
#define DEV_CRC32_POLY (0xEDB88320UL)

static dev_err_t dev_crc_validate_input(const uint8_t *data, size_t len)
{
    if ((len > 0U) && (data == NULL))
    {
        return DEV_ERR_PARAM;
    }

    return DEV_OK;
}

dev_err_t dev_crc8_init(dev_crc8_ctx_t *ctx)
{
    DEV_RETURN_ON_FALSE(ctx != NULL, DEV_ERR_PARAM);

    ctx->value = 0x00U;
    return DEV_OK;
}

dev_err_t dev_crc8_update(dev_crc8_ctx_t *ctx, const uint8_t *data, size_t len)
{
    uint8_t crc;
    size_t i;
    uint8_t bit;

    DEV_RETURN_ON_FALSE(ctx != NULL, DEV_ERR_PARAM);
    DEV_RETURN_ON_FALSE(dev_crc_validate_input(data, len) == DEV_OK, DEV_ERR_PARAM);

    crc = ctx->value;

    for (i = 0U; i < len; i++)
    {
        crc ^= data[i];

        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x80U) != 0U)
            {
                crc = (uint8_t)((crc << 1U) ^ DEV_CRC8_POLY);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    ctx->value = crc;
    return DEV_OK;
}

dev_err_t dev_crc8_final(const dev_crc8_ctx_t *ctx, uint8_t *out_crc)
{
    DEV_RETURN_ON_FALSE(ctx != NULL, DEV_ERR_PARAM);
    DEV_RETURN_ON_FALSE(out_crc != NULL, DEV_ERR_PARAM);

    *out_crc = ctx->value;
    return DEV_OK;
}

dev_err_t dev_crc8_compute(const uint8_t *data, size_t len, uint8_t *out_crc)
{
    dev_crc8_ctx_t ctx;
    dev_err_t err;

    err = dev_crc8_init(&ctx);
    DEV_RETURN_ON_FALSE(err == DEV_OK, err);

    err = dev_crc8_update(&ctx, data, len);
    DEV_RETURN_ON_FALSE(err == DEV_OK, err);

    err = dev_crc8_final(&ctx, out_crc);
    DEV_RETURN_ON_FALSE(err == DEV_OK, err);

    return DEV_OK;
}

dev_err_t dev_crc16_init(dev_crc16_ctx_t *ctx)
{
    DEV_RETURN_ON_FALSE(ctx != NULL, DEV_ERR_PARAM);

    ctx->value = 0xFFFFU;
    return DEV_OK;
}

dev_err_t dev_crc16_update(dev_crc16_ctx_t *ctx, const uint8_t *data, size_t len)
{
    uint16_t crc;
    size_t i;
    uint8_t bit;

    DEV_RETURN_ON_FALSE(ctx != NULL, DEV_ERR_PARAM);
    DEV_RETURN_ON_FALSE(dev_crc_validate_input(data, len) == DEV_OK, DEV_ERR_PARAM);

    crc = ctx->value;

    for (i = 0U; i < len; i++)
    {
        crc ^= (uint16_t)data[i];

        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (crc >> 1U) ^ DEV_CRC16_POLY;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    ctx->value = crc;
    return DEV_OK;
}

dev_err_t dev_crc16_final(const dev_crc16_ctx_t *ctx, uint16_t *out_crc)
{
    DEV_RETURN_ON_FALSE(ctx != NULL, DEV_ERR_PARAM);
    DEV_RETURN_ON_FALSE(out_crc != NULL, DEV_ERR_PARAM);

    *out_crc = ctx->value;
    return DEV_OK;
}

dev_err_t dev_crc16_compute(const uint8_t *data, size_t len, uint16_t *out_crc)
{
    dev_crc16_ctx_t ctx;
    dev_err_t err;

    err = dev_crc16_init(&ctx);
    DEV_RETURN_ON_FALSE(err == DEV_OK, err);

    err = dev_crc16_update(&ctx, data, len);
    DEV_RETURN_ON_FALSE(err == DEV_OK, err);

    err = dev_crc16_final(&ctx, out_crc);
    DEV_RETURN_ON_FALSE(err == DEV_OK, err);

    return DEV_OK;
}

dev_err_t dev_crc32_init(dev_crc32_ctx_t *ctx)
{
    DEV_RETURN_ON_FALSE(ctx != NULL, DEV_ERR_PARAM);

    ctx->value = 0xFFFFFFFFUL;
    return DEV_OK;
}

dev_err_t dev_crc32_update(dev_crc32_ctx_t *ctx, const uint8_t *data, size_t len)
{
    uint32_t crc;
    size_t i;
    uint8_t bit;

    DEV_RETURN_ON_FALSE(ctx != NULL, DEV_ERR_PARAM);
    DEV_RETURN_ON_FALSE(dev_crc_validate_input(data, len) == DEV_OK, DEV_ERR_PARAM);

    crc = ctx->value;

    for (i = 0U; i < len; i++)
    {
        crc ^= (uint32_t)data[i];

        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x00000001UL) != 0UL)
            {
                crc = (crc >> 1U) ^ DEV_CRC32_POLY;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    ctx->value = crc;
    return DEV_OK;
}

dev_err_t dev_crc32_final(const dev_crc32_ctx_t *ctx, uint32_t *out_crc)
{
    DEV_RETURN_ON_FALSE(ctx != NULL, DEV_ERR_PARAM);
    DEV_RETURN_ON_FALSE(out_crc != NULL, DEV_ERR_PARAM);

    *out_crc = ctx->value ^ 0xFFFFFFFFUL;
    return DEV_OK;
}

dev_err_t dev_crc32_compute(const uint8_t *data, size_t len, uint32_t *out_crc)
{
    dev_crc32_ctx_t ctx;
    dev_err_t err;

    err = dev_crc32_init(&ctx);
    DEV_RETURN_ON_FALSE(err == DEV_OK, err);

    err = dev_crc32_update(&ctx, data, len);
    DEV_RETURN_ON_FALSE(err == DEV_OK, err);

    err = dev_crc32_final(&ctx, out_crc);
    DEV_RETURN_ON_FALSE(err == DEV_OK, err);

    return DEV_OK;
}