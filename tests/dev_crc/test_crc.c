#include "dev_crc.h"
#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)

#define CHECK(cond, msg) do {                              \
    if (!(cond)) {                                         \
        printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        g_failures++; return;                               \
    }                                                       \
} while (false)

#define CHECK_EQ(a, b, msg) do {                           \
    if ((a) != (b)) {                                      \
        printf("  FAIL: %s (expected 0x%x, got 0x%x) (%s:%d)\n", \
               msg, (unsigned)(b), (unsigned)(a), __FILE__, __LINE__); \
        g_failures++; return;                               \
    }                                                       \
} while (false)

#define CHECK_ERR(e, expected, msg) do {                   \
    if ((e) != (expected)) {                               \
        printf("  FAIL: %s (expected %d, got %d) (%s:%d)\n", \
               msg, (int)(expected), (int)(e), __FILE__, __LINE__); \
        g_failures++; return;                               \
    }                                                       \
} while (false)

#define RUN_TEST(name) do { printf("  test_%s...\n", #name); test_##name(); } while (false)

/* Known test vector for all CRCs: "123456789" (9 bytes) */
static const uint8_t k_check_data[] = "123456789";
#define K_CHECK_LEN  9U

/* ── CRC-8 known vectors ── */

TEST(1_crc8_check_value)
{
    uint8_t out;
    CHECK_ERR(dev_crc8_compute(k_check_data, K_CHECK_LEN, &out), DEV_OK, "compute");
    CHECK_EQ(out, 0xF4U, "CRC-8(123456789) == 0xF4");
    printf("    PASS\n"); g_passes++;
}

TEST(2_crc8_streaming)
{
    dev_crc8_ctx_t ctx;
    uint8_t out;
    CHECK_ERR(dev_crc8_init(&ctx), DEV_OK, "init");
    CHECK_ERR(dev_crc8_update(&ctx, k_check_data, 5U), DEV_OK, "update 5");
    CHECK_ERR(dev_crc8_update(&ctx, k_check_data + 5U, 4U), DEV_OK, "update 4");
    CHECK_ERR(dev_crc8_final(&ctx, &out), DEV_OK, "final");
    CHECK_EQ(out, 0xF4U, "streaming == one-shot");
    printf("    PASS\n"); g_passes++;
}

TEST(3_crc8_zero_len)
{
    uint8_t out;
    CHECK_ERR(dev_crc8_compute(k_check_data, 0U, &out), DEV_OK, "zero len");
    CHECK_EQ(out, 0x00U, "CRC-8() == init 0x00");
    printf("    PASS\n"); g_passes++;
}

TEST(4_crc8_null_ctx)
{
    CHECK_ERR(dev_crc8_init(NULL), DEV_ERR_NULL_PTR, "null ctx");
    CHECK_ERR(dev_crc8_update(NULL, k_check_data, 1U), DEV_ERR_NULL_PTR, "null ctx update");
    CHECK_ERR(dev_crc8_final(NULL, NULL), DEV_ERR_NULL_PTR, "null ctx final");
    printf("    PASS\n"); g_passes++;
}

TEST(5_crc8_null_out)
{
    dev_crc8_ctx_t ctx;
    dev_crc8_init(&ctx);
    CHECK_ERR(dev_crc8_final(&ctx, NULL), DEV_ERR_NULL_PTR, "null out_crc");
    CHECK_ERR(dev_crc8_compute(k_check_data, K_CHECK_LEN, NULL), DEV_ERR_NULL_PTR, "null out compute");
    printf("    PASS\n"); g_passes++;
}

/* ── CRC-16 known vectors ── */

TEST(6_crc16_check_value)
{
    uint16_t out;
    CHECK_ERR(dev_crc16_compute(k_check_data, K_CHECK_LEN, &out), DEV_OK, "compute");
    CHECK_EQ(out, 0x4B37U, "CRC-16/MODBUS(123456789) == 0x4B37");
    printf("    PASS\n"); g_passes++;
}

TEST(7_crc16_streaming)
{
    dev_crc16_ctx_t ctx;
    uint16_t out;
    CHECK_ERR(dev_crc16_init(&ctx), DEV_OK, "init");
    CHECK_ERR(dev_crc16_update(&ctx, k_check_data, 3U), DEV_OK, "update 3");
    CHECK_ERR(dev_crc16_update(&ctx, k_check_data + 3U, 3U), DEV_OK, "update 3");
    CHECK_ERR(dev_crc16_update(&ctx, k_check_data + 6U, 3U), DEV_OK, "update 3");
    CHECK_ERR(dev_crc16_final(&ctx, &out), DEV_OK, "final");
    CHECK_EQ(out, 0x4B37U, "streaming == one-shot");
    printf("    PASS\n"); g_passes++;
}

TEST(8_crc16_zero_len)
{
    uint16_t out;
    CHECK_ERR(dev_crc16_compute(k_check_data, 0U, &out), DEV_OK, "zero len");
    CHECK_EQ(out, 0xFFFFU, "CRC-16() == init 0xFFFF");
    printf("    PASS\n"); g_passes++;
}

TEST(9_crc16_null_ptr)
{
    CHECK_ERR(dev_crc16_init(NULL), DEV_ERR_NULL_PTR, "null ctx");
    CHECK_ERR(dev_crc16_compute(k_check_data, K_CHECK_LEN, NULL), DEV_ERR_NULL_PTR, "null out");
    printf("    PASS\n"); g_passes++;
}

/* ── CRC-32 known vectors ── */

TEST(10_crc32_check_value)
{
    uint32_t out;
    CHECK_ERR(dev_crc32_compute(k_check_data, K_CHECK_LEN, &out), DEV_OK, "compute");
    CHECK_EQ(out, 0xCBF43926UL, "CRC-32/IEEE(123456789) == 0xCBF43926");
    printf("    PASS\n"); g_passes++;
}

TEST(11_crc32_streaming)
{
    dev_crc32_ctx_t ctx;
    uint32_t out;
    CHECK_ERR(dev_crc32_init(&ctx), DEV_OK, "init");
    CHECK_ERR(dev_crc32_update(&ctx, k_check_data, 3U), DEV_OK, "update 3");
    CHECK_ERR(dev_crc32_update(&ctx, k_check_data + 3U, 3U), DEV_OK, "update 3");
    CHECK_ERR(dev_crc32_update(&ctx, k_check_data + 6U, 3U), DEV_OK, "update 3");
    CHECK_ERR(dev_crc32_final(&ctx, &out), DEV_OK, "final");
    CHECK_EQ(out, 0xCBF43926UL, "streaming == one-shot");
    printf("    PASS\n"); g_passes++;
}

TEST(12_crc32_zero_len)
{
    uint32_t out;
    CHECK_ERR(dev_crc32_compute(k_check_data, 0U, &out), DEV_OK, "zero len");
    CHECK_EQ(out, 0x00000000UL, "CRC-32() == init^xor_out = 0");
    printf("    PASS\n"); g_passes++;
}

TEST(13_crc32_null_ptr)
{
    CHECK_ERR(dev_crc32_init(NULL), DEV_ERR_NULL_PTR, "null ctx");
    CHECK_ERR(dev_crc32_compute(k_check_data, K_CHECK_LEN, NULL), DEV_ERR_NULL_PTR, "null out");
    printf("    PASS\n"); g_passes++;
}

/* ── Main ── */

int main(void)
{
    printf("=== dev_crc Test Suite ===\n\n");

    RUN_TEST(1_crc8_check_value);
    RUN_TEST(2_crc8_streaming);
    RUN_TEST(3_crc8_zero_len);
    RUN_TEST(4_crc8_null_ctx);
    RUN_TEST(5_crc8_null_out);
    RUN_TEST(6_crc16_check_value);
    RUN_TEST(7_crc16_streaming);
    RUN_TEST(8_crc16_zero_len);
    RUN_TEST(9_crc16_null_ptr);
    RUN_TEST(10_crc32_check_value);
    RUN_TEST(11_crc32_streaming);
    RUN_TEST(12_crc32_zero_len);
    RUN_TEST(13_crc32_null_ptr);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
