#include "dev_ringbuf.h"
#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)
#define CHECK(cond, msg) do { if (!(cond)) { printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); g_failures++; return; } } while(false)
#define CHECK_EQ(a, b, msg) do { size_t _a=(size_t)(a),_b=(size_t)(b); if(_a!=_b){printf("  FAIL: %s (exp %zu got %zu) (%s:%d)\n",msg,_b,_a,__FILE__,__LINE__);g_failures++;return;} } while(false)
#define CHECK_ERR(e, exp, msg) do { int _e=(int)(e); if(_e!=(int)(exp)){printf("  FAIL: %s (exp %d got %d) (%s:%d)\n",msg,(int)(exp),_e,__FILE__,__LINE__);g_failures++;return;} } while(false)
#define RUN_TEST(name) do { printf("  test_%s...\n", #name); test_##name(); } while(false)

#define CAP (8U)
static uint8_t   g_buf[CAP];
static dev_ringbuf_t g_rb;

/* 1-3: init validation */
TEST(1_init_ok)      { CHECK_ERR(dev_ringbuf_init(&g_rb, g_buf, CAP), DEV_OK, "init"); CHECK(dev_ringbuf_is_valid(&g_rb), "valid"); printf("    PASS\n"); g_passes++; }
TEST(2_init_null_ring) { CHECK_ERR(dev_ringbuf_init(NULL, g_buf, CAP), DEV_ERR_NULL_PTR, "ring"); printf("    PASS\n"); g_passes++; }
TEST(3_init_null_storage) { CHECK_ERR(dev_ringbuf_init(&g_rb, NULL, CAP), DEV_ERR_NULL_PTR, "storage"); printf("    PASS\n"); g_passes++; }
TEST(4_init_cap_lt_2) { CHECK_ERR(dev_ringbuf_init(&g_rb, g_buf, 1U), DEV_ERR_INVALID_ARG, "cap<2"); printf("    PASS\n"); g_passes++; }

/* 5-6: empty pop */
TEST(5_pop_empty_try) { dev_ringbuf_init(&g_rb, g_buf, CAP); uint8_t v; CHECK(!dev_ringbuf_try_pop(&g_rb, &v), "empty"); printf("    PASS\n"); g_passes++; }
TEST(6_pop_empty_err) { dev_ringbuf_init(&g_rb, g_buf, CAP); uint8_t v; CHECK_ERR(dev_ringbuf_pop(&g_rb, &v), DEV_ERR_BUSY, "empty"); printf("    PASS\n"); g_passes++; }

/* 7-8: push/pop round trip */
TEST(7_push_pop_try) { dev_ringbuf_init(&g_rb, g_buf, CAP); CHECK(dev_ringbuf_try_push(&g_rb, 'A'), "push"); uint8_t v; CHECK(dev_ringbuf_try_pop(&g_rb, &v), "pop"); CHECK_EQ(v, 'A', "val"); printf("    PASS\n"); g_passes++; }
TEST(8_write_pop_err) { dev_ringbuf_init(&g_rb, g_buf, CAP); CHECK_ERR(dev_ringbuf_write(&g_rb, 'Z'), DEV_OK, "write"); uint8_t v; CHECK_ERR(dev_ringbuf_pop(&g_rb, &v), DEV_OK, "pop"); CHECK_EQ(v, 'Z', "val"); printf("    PASS\n"); g_passes++; }

/* 9: capacity-1 is max usable */
TEST(9_capacity_minus_one) {
    dev_ringbuf_init(&g_rb, g_buf, CAP);
    CHECK_EQ(dev_ringbuf_capacity(&g_rb), CAP - 1U, "usable cap");
    for (size_t i = 0U; i < CAP - 1U; i++) CHECK(dev_ringbuf_try_push(&g_rb, (uint8_t)i), "push");
    CHECK(!dev_ringbuf_try_push(&g_rb, 0xFFU), "full");
    CHECK_EQ(dev_ringbuf_available(&g_rb), CAP - 1U, "available");
    printf("    PASS\n"); g_passes++;
}

/* 10: fill then drain */
TEST(10_fill_drain) {
    dev_ringbuf_init(&g_rb, g_buf, CAP);
    for (size_t i = 0U; i < CAP - 1U; i++) dev_ringbuf_try_push(&g_rb, (uint8_t)i);
    CHECK_EQ(dev_ringbuf_available(&g_rb), CAP - 1U, "avail");
    for (size_t i = 0U; i < CAP - 1U; i++) { uint8_t v; dev_ringbuf_try_pop(&g_rb, &v); CHECK_EQ(v, (uint8_t)i, "order"); }
    CHECK_EQ(dev_ringbuf_available(&g_rb), 0U, "empty"); CHECK_EQ(dev_ringbuf_free(&g_rb), CAP - 1U, "free");
    printf("    PASS\n"); g_passes++;
}

/* 11: wraparound */
TEST(11_wraparound) {
    dev_ringbuf_init(&g_rb, g_buf, CAP);
    for (size_t i = 0U; i < CAP - 1U; i++) dev_ringbuf_try_push(&g_rb, (uint8_t)(i + 10U));
    for (size_t i = 0U; i < 3U; i++) { uint8_t v; dev_ringbuf_try_pop(&g_rb, &v); }
    for (size_t i = 0U; i < 3U; i++) dev_ringbuf_try_push(&g_rb, (uint8_t)(i + 100U));
    uint8_t vals[7]; size_t n;
    CHECK_ERR(dev_ringbuf_read(&g_rb, vals, 7U, &n), DEV_OK, "read"); CHECK_EQ(n, 7U, "count");
    CHECK_EQ(vals[0], 13U, "v0"); CHECK_EQ(vals[1], 14U, "v1"); CHECK_EQ(vals[2], 15U, "v2");
    CHECK_EQ(vals[3], 16U, "v3");
    printf("    PASS\n"); g_passes++;
}

/* 12: flush */
TEST(12_flush) {
    dev_ringbuf_init(&g_rb, g_buf, CAP);
    for (size_t i = 0U; i < CAP - 1U; i++) dev_ringbuf_try_push(&g_rb, (uint8_t)i);
    CHECK_ERR(dev_ringbuf_flush(&g_rb), DEV_OK, "flush");
    CHECK_EQ(dev_ringbuf_available(&g_rb), 0U, "empty after flush");
    uint8_t v; CHECK(!dev_ringbuf_try_pop(&g_rb, &v), "pop empty");
    printf("    PASS\n"); g_passes++;
}

/* 13: read partial */
TEST(13_read_partial) {
    dev_ringbuf_init(&g_rb, g_buf, CAP);
    dev_ringbuf_try_push(&g_rb, 'X'); dev_ringbuf_try_push(&g_rb, 'Y');
    uint8_t out[4]; size_t n;
    CHECK_ERR(dev_ringbuf_read(&g_rb, out, 4U, &n), DEV_OK, "read");
    CHECK_EQ(n, 2U, "partial read"); CHECK_EQ(out[0], 'X', "X"); CHECK_EQ(out[1], 'Y', "Y");
    printf("    PASS\n"); g_passes++;
}

/* 14: invalid context */
TEST(14_invalid_ctx) {
    dev_ringbuf_t bad = {NULL, 0U, 0U, 0U};
    CHECK(!dev_ringbuf_is_valid(&bad), "!valid");
    uint8_t v;
    CHECK(!dev_ringbuf_try_push(&bad, 0U), "push"); CHECK(!dev_ringbuf_try_pop(&bad, &v), "pop");
    CHECK_EQ(dev_ringbuf_available(&bad), 0U, "avail"); CHECK_EQ(dev_ringbuf_capacity(&bad), 0U, "cap"); CHECK_EQ(dev_ringbuf_free(&bad), 0U, "free");
    printf("    PASS\n"); g_passes++;
}

int main(void)
{
    printf("=== dev_ringbuf Test Suite ===\n\n");
    RUN_TEST(1_init_ok); RUN_TEST(2_init_null_ring); RUN_TEST(3_init_null_storage);
    RUN_TEST(4_init_cap_lt_2); RUN_TEST(5_pop_empty_try); RUN_TEST(6_pop_empty_err);
    RUN_TEST(7_push_pop_try); RUN_TEST(8_write_pop_err); RUN_TEST(9_capacity_minus_one);
    RUN_TEST(10_fill_drain); RUN_TEST(11_wraparound); RUN_TEST(12_flush);
    RUN_TEST(13_read_partial); RUN_TEST(14_invalid_ctx);
    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
