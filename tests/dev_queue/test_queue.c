#include "dev_queue.h"
#include <stdio.h>

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)
#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        g_failures++; return; \
    } \
} while(false)

#define CHECK_EQ(a, b, msg) do { \
    int _a = (int)(a); int _b = (int)(b); \
    if (_a != _b) { \
        printf("  FAIL: %s (exp %d got %d) (%s:%d)\n", msg, _b, _a, __FILE__, __LINE__); \
        g_failures++; return; \
    } \
} while(false)

#define CHECK_ERR(e, exp, msg) do { \
    int _e = (int)(e); \
    if (_e != (int)(exp)) { \
        printf("  FAIL: %s (exp %d got %d) (%s:%d)\n", msg, (int)(exp), _e, __FILE__, __LINE__); \
        g_failures++; return; \
    } \
} while(false)

#define RUN_TEST(name) do { \
    printf("  test_%s...\n", #name); \
    test_##name(); \
} while(false)

/* ── Shared test storage ── */

#define Q_CAP (8U)
static uint8_t    g_storage[Q_CAP * sizeof(uint32_t)];
static dev_queue_t g_q;

/* ── 1-4: init validation ── */

TEST(1_init_ok)
{
    CHECK_ERR(dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP), DEV_OK, "init");
    CHECK(g_q.initialized, "initialized flag");
    CHECK_EQ(g_q.count, 0, "count zero");
    CHECK_EQ(g_q.capacity, Q_CAP, "capacity");
    printf("    PASS\n"); g_passes++;
}

TEST(2_init_null_queue)
{
    CHECK_ERR(dev_queue_init(NULL, g_storage, sizeof(uint32_t), Q_CAP), DEV_ERR_NULL_PTR, "null queue");
    printf("    PASS\n"); g_passes++;
}

TEST(3_init_null_buffer)
{
    CHECK_ERR(dev_queue_init(&g_q, NULL, sizeof(uint32_t), Q_CAP), DEV_ERR_NULL_PTR, "null buffer");
    printf("    PASS\n"); g_passes++;
}

TEST(4_init_zero_item_size)
{
    CHECK_ERR(dev_queue_init(&g_q, g_storage, 0U, Q_CAP), DEV_ERR_INVALID_ARG, "item_size=0");
    printf("    PASS\n"); g_passes++;
}

TEST(5_init_zero_capacity)
{
    CHECK_ERR(dev_queue_init(&g_q, g_storage, sizeof(uint32_t), 0U), DEV_ERR_INVALID_ARG, "capacity=0");
    printf("    PASS\n"); g_passes++;
}

TEST(6_double_init)
{
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK_ERR(dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP), DEV_ERR_ALREADY_INITIALIZED, "double init");
    printf("    PASS\n"); g_passes++;
}

/* ── 7-8: deinit ── */

TEST(7_deinit)
{
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK_ERR(dev_queue_deinit(&g_q), DEV_OK, "deinit");
    CHECK(!g_q.initialized, "!initialized after deinit");
    printf("    PASS\n"); g_passes++;
}

TEST(8_deinit_before_init)
{
    dev_queue_t q = {NULL, 0U, 0U, 0U, 0U, 0U, false};
    CHECK_ERR(dev_queue_deinit(&q), DEV_ERR_NOT_INITIALIZED, "deinit before init");
    printf("    PASS\n"); g_passes++;
}

/* ── 9-11: push/pop before init ── */

TEST(9_push_before_init)
{
    dev_queue_t q = {NULL, 0U, 0U, 0U, 0U, 0U, false};
    uint32_t v = 42U;
    CHECK_ERR(dev_queue_push(&q, &v), DEV_ERR_NOT_INITIALIZED, "push before init");
    printf("    PASS\n"); g_passes++;
}

TEST(10_pop_before_init)
{
    dev_queue_t q = {NULL, 0U, 0U, 0U, 0U, 0U, false};
    uint32_t v;
    CHECK_ERR(dev_queue_pop(&q, &v), DEV_ERR_NOT_INITIALIZED, "pop before init");
    printf("    PASS\n"); g_passes++;
}

TEST(11_null_item_push)
{
    dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK_ERR(dev_queue_push(&g_q, NULL), DEV_ERR_NULL_PTR, "null item push");
    printf("    PASS\n"); g_passes++;
}

/* ── 12-13: push/pop round trip ── */

TEST(12_push_pop_one)
{
    uint32_t in = 0xDEADBEEFU;
    uint32_t out = 0U;
    dev_queue_deinit(&g_q); dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK_ERR(dev_queue_push(&g_q, &in), DEV_OK, "push");
    CHECK_EQ(dev_queue_get_count(&g_q), 1, "count=1");
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop");
    CHECK_EQ(out, 0xDEADBEEFU, "value preserved");
    CHECK_EQ(dev_queue_get_count(&g_q), 0, "count=0");
    printf("    PASS\n"); g_passes++;
}

TEST(13_fifo_order)
{
    uint32_t in, out;
    dev_queue_deinit(&g_q); dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    in = 100U; dev_queue_push(&g_q, &in);
    in = 200U; dev_queue_push(&g_q, &in);
    in = 300U; dev_queue_push(&g_q, &in);

    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop1");
    CHECK_EQ(out, 100U, "first in");
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop2");
    CHECK_EQ(out, 200U, "second in");
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop3");
    CHECK_EQ(out, 300U, "third in");
    printf("    PASS\n"); g_passes++;
}

/* ── 14-15: full/empty edge cases ── */

TEST(14_push_when_full)
{
    uint32_t v;
    dev_queue_deinit(&g_q); dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    for (v = 0U; v < Q_CAP; v++) { dev_queue_push(&g_q, &v); }
    CHECK(dev_queue_is_full(&g_q), "is_full");
    v = 99U;
    CHECK_ERR(dev_queue_push(&g_q, &v), DEV_ERR_OVERFLOW, "push full");
    printf("    PASS\n"); g_passes++;
}

TEST(15_pop_when_empty)
{
    uint32_t v;
    dev_queue_deinit(&g_q); dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK(dev_queue_is_empty(&g_q), "is_empty");
    CHECK_ERR(dev_queue_pop(&g_q, &v), DEV_ERR_EMPTY, "pop empty");
    printf("    PASS\n"); g_passes++;
}

/* ── 16: peek ── */

TEST(16_peek)
{
    uint32_t in = 0xCAFEU;
    uint32_t out;
    dev_queue_deinit(&g_q); dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    /* Peek empty */
    CHECK_ERR(dev_queue_peek(&g_q, &out), DEV_ERR_EMPTY, "peek empty");

    /* Peek does not remove */
    dev_queue_push(&g_q, &in);
    CHECK_ERR(dev_queue_peek(&g_q, &out), DEV_OK, "peek");
    CHECK_EQ(out, 0xCAFEU, "peek value");
    CHECK_EQ(dev_queue_get_count(&g_q), 1, "count still 1 after peek");

    /* Pop confirms item still there */
    out = 0U;
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop after peek");
    CHECK_EQ(out, 0xCAFEU, "pop value matches peek");
    printf("    PASS\n"); g_passes++;
}

/* ── 17: reset ── */

TEST(17_reset)
{
    uint32_t v;
    dev_queue_deinit(&g_q); dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    v = 1U; dev_queue_push(&g_q, &v);
    v = 2U; dev_queue_push(&g_q, &v);
    CHECK_EQ(dev_queue_get_count(&g_q), 2, "count before reset");

    CHECK_ERR(dev_queue_reset(&g_q), DEV_OK, "reset");
    CHECK_EQ(dev_queue_get_count(&g_q), 0, "count after reset");
    CHECK(dev_queue_is_empty(&g_q), "empty after reset");

    /* Push/pop works after reset */
    v = 42U;
    CHECK_ERR(dev_queue_push(&g_q, &v), DEV_OK, "push after reset");
    CHECK_ERR(dev_queue_pop(&g_q, &v), DEV_OK, "pop after reset");
    CHECK_EQ(v, 42U, "value after reset");
    printf("    PASS\n"); g_passes++;
}

/* ── 18: wrap-around ── */

TEST(18_wraparound)
{
    uint32_t in, out;
    dev_queue_deinit(&g_q); dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    /* Fill queue */
    for (in = 0U; in < Q_CAP; in++) { dev_queue_push(&g_q, &in); }

    /* Pop half */
    for (in = 0U; in < (Q_CAP / 2U); in++) { dev_queue_pop(&g_q, &out); }

    /* Push new values to cause wrap */
    for (in = 100U; in < 100U + (Q_CAP / 2U); in++) { dev_queue_push(&g_q, &in); }

    /* Verify FIFO order across wrap boundary */
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop after wrap");
    CHECK_EQ(out, (Q_CAP / 2U), "wrapped first val");

    for (in = 1U; in < (Q_CAP / 2U); in++) { dev_queue_pop(&g_q, &out); }

    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop new val");
    CHECK_EQ(out, 100U, "new first val");
    printf("    PASS\n"); g_passes++;
}

/* ── 19: query APIs ── */

TEST(19_query_apis)
{
    dev_queue_deinit(&g_q); dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);
    CHECK(dev_queue_is_initialized(&g_q), "is_initialized");
    CHECK_EQ(dev_queue_get_capacity(&g_q), Q_CAP, "capacity");
    CHECK_EQ(dev_queue_get_item_size(&g_q), sizeof(uint32_t), "item_size");
    CHECK_EQ(dev_queue_get_free_count(&g_q), Q_CAP, "free=capacity");

    uint32_t v = 5U;
    dev_queue_push(&g_q, &v);
    CHECK_EQ(dev_queue_get_count(&g_q), 1, "count=1");
    CHECK_EQ(dev_queue_get_free_count(&g_q), Q_CAP - 1U, "free=cap-1");
    printf("    PASS\n"); g_passes++;
}

/* ── 20: queue of struct ── */

TEST(20_struct_queue)
{
    typedef struct { uint8_t id; uint32_t ts; } event_t;
    static event_t    evt_storage[4];
    static dev_queue_t evt_q;

    dev_queue_init(&evt_q, evt_storage, sizeof(event_t), 4U);

    event_t in  = { .id = 7U, .ts = 12345U };
    event_t out = { .id = 0U, .ts = 0U };

    CHECK_ERR(dev_queue_push(&evt_q, &in), DEV_OK, "struct push");
    CHECK_ERR(dev_queue_pop(&evt_q, &out), DEV_OK, "struct pop");
    CHECK_EQ(out.id, 7U, "struct id");
    CHECK_EQ(out.ts, 12345U, "struct ts");
    printf("    PASS\n"); g_passes++;
}

/* ── 21: queue of pointers ── */

TEST(21_pointer_queue)
{
    static void *ptr_storage[4];
    static dev_queue_t ptr_q;

    dev_queue_init(&ptr_q, ptr_storage, sizeof(void *), 4U);

    int a = 1, b = 2;
    void *in  = &a;
    void *out = NULL;

    CHECK_ERR(dev_queue_push(&ptr_q, &in), DEV_OK, "ptr push");
    CHECK_ERR(dev_queue_pop(&ptr_q, &out), DEV_OK, "ptr pop");
    CHECK_EQ((int)(out == &a), 1, "ptr matches");

    /* Push second, verify FIFO for pointers */
    in = &b;
    dev_queue_push(&ptr_q, &in);
    dev_queue_pop(&ptr_q, &out);
    CHECK_EQ((int)(out == &b), 1, "ptr2 matches");
    printf("    PASS\n"); g_passes++;
}

/* ── 22: push_overwrite ── */

TEST(22_push_overwrite)
{
    uint32_t in, out;
    dev_queue_deinit(&g_q); dev_queue_init(&g_q, g_storage, sizeof(uint32_t), Q_CAP);

    /* Fill queue */
    for (in = 10U; in < 10U + Q_CAP; in++) { dev_queue_push(&g_q, &in); }
    CHECK(dev_queue_is_full(&g_q), "full");

    /* Overwrite: oldest (10) dropped, 99 inserted */
    in = 99U;
    CHECK_ERR(dev_queue_push_overwrite(&g_q, &in), DEV_ERR_OVERFLOW, "overwrite signaled");

    /* Oldest should now be 11 (10 was dropped) */
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop after overwrite");
    CHECK_EQ(out, 11U, "oldest is now 11");

    /* Drain remaining, last should be 99 */
    while (dev_queue_get_count(&g_q) > 1U) { dev_queue_pop(&g_q, &out); }
    CHECK_ERR(dev_queue_pop(&g_q, &out), DEV_OK, "pop last");
    CHECK_EQ(out, 99U, "new item is at tail");
    printf("    PASS\n"); g_passes++;
}

/* ── 23: static macros ── */

TEST(23_static_macros)
{
    typedef struct { uint8_t a; uint16_t b; } item_t;

    DEV_QUEUE_DEFINE(macro_q, item_t, 4U);
    CHECK_ERR(DEV_QUEUE_INIT(macro_q, item_t, 4U), DEV_OK, "macro init");

    item_t in  = { .a = 3U, .b = 500U };
    item_t out = { .a = 0U, .b = 0U };

    CHECK_ERR(dev_queue_push(&macro_q, &in), DEV_OK, "macro push");
    CHECK_ERR(dev_queue_pop(&macro_q, &out), DEV_OK, "macro pop");
    CHECK_EQ(out.a, 3U, "macro a");
    CHECK_EQ(out.b, 500U, "macro b");
    printf("    PASS\n"); g_passes++;
}

/* ── 24: reset not initialized ── */

TEST(24_reset_not_init)
{
    dev_queue_t q = {NULL, 0U, 0U, 0U, 0U, 0U, false};
    CHECK_ERR(dev_queue_reset(&q), DEV_ERR_NOT_INITIALIZED, "reset not init");
    printf("    PASS\n"); g_passes++;
}

/* ── 25: null pointer safety ── */

TEST(25_null_safety)
{
    CHECK(!dev_queue_is_initialized(NULL), "is_init(NULL)");
    CHECK(dev_queue_is_empty(NULL), "is_empty(NULL)");
    CHECK(!dev_queue_is_full(NULL), "is_full(NULL)");
    CHECK_EQ(dev_queue_get_count(NULL), 0, "count(NULL)");
    CHECK_EQ(dev_queue_get_capacity(NULL), 0, "capacity(NULL)");
    CHECK_EQ(dev_queue_get_free_count(NULL), 0, "free(NULL)");
    CHECK_EQ(dev_queue_get_item_size(NULL), 0, "item_size(NULL)");
    printf("    PASS\n"); g_passes++;
}

/* ── Main ── */

int main(void)
{
    printf("=== dev_queue Test Suite ===\n\n");

    RUN_TEST(1_init_ok);
    RUN_TEST(2_init_null_queue);
    RUN_TEST(3_init_null_buffer);
    RUN_TEST(4_init_zero_item_size);
    RUN_TEST(5_init_zero_capacity);
    RUN_TEST(6_double_init);
    RUN_TEST(7_deinit);
    RUN_TEST(8_deinit_before_init);
    RUN_TEST(9_push_before_init);
    RUN_TEST(10_pop_before_init);
    RUN_TEST(11_null_item_push);
    RUN_TEST(12_push_pop_one);
    RUN_TEST(13_fifo_order);
    RUN_TEST(14_push_when_full);
    RUN_TEST(15_pop_when_empty);
    RUN_TEST(16_peek);
    RUN_TEST(17_reset);
    RUN_TEST(18_wraparound);
    RUN_TEST(19_query_apis);
    RUN_TEST(20_struct_queue);
    RUN_TEST(21_pointer_queue);
    RUN_TEST(22_push_overwrite);
    RUN_TEST(23_static_macros);
    RUN_TEST(24_reset_not_init);
    RUN_TEST(25_null_safety);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
