#include "dev_list.h"
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
    size_t _a = (size_t)(a); size_t _b = (size_t)(b);       \
    if (_a != _b) {                                        \
        printf("  FAIL: %s (expected %zu, got %zu) (%s:%d)\n", \
               msg, _b, _a, __FILE__, __LINE__);            \
        g_failures++; return;                               \
    }                                                       \
} while (false)

#define RUN_TEST(name) do {                                \
    printf("  test_%s...\n", #name);                       \
    test_##name();                                         \
} while (false)

/* ── Test helpers ── */

#define CAP   (4U)
#define ISZ   (8U)

static dev_list_node_t g_nodes[CAP];
static uint8_t         g_data[CAP * ISZ];
static dev_list_t      g_list;

/* ── 1: init valid ── */
TEST(1_init_valid)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK_EQ(dev_list_size(&g_list), 0U, "size 0");
    printf("    PASS\n"); g_passes++;
}

/* ── 2: init NULL list ── */
TEST(2_init_null_list)
{
    CHECK(!dev_list_init(NULL, g_nodes, g_data, CAP, ISZ), "null list fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 3: init NULL node_pool ── */
TEST(3_init_null_nodes)
{
    CHECK(!dev_list_init(&g_list, NULL, g_data, CAP, ISZ), "null nodes fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 4: init NULL data_pool ── */
TEST(4_init_null_data)
{
    CHECK(!dev_list_init(&g_list, g_nodes, NULL, CAP, ISZ), "null data fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 5: init zero capacity ── */
TEST(5_init_zero_cap)
{
    CHECK(!dev_list_init(&g_list, g_nodes, g_data, 0U, ISZ), "zero cap fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 6: init zero item_size ── */
TEST(6_init_zero_item)
{
    CHECK(!dev_list_init(&g_list, g_nodes, g_data, CAP, 0U), "zero item fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 7: init overflow guard ── */
TEST(7_init_overflow)
{
    CHECK(!dev_list_init(&g_list, g_nodes, g_data, SIZE_MAX, 2U), "overflow fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 8: push_head then pop_head round-trip ── */
TEST(8_push_head_pop_head)
{
    uint8_t buf[ISZ];
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_head(&g_list, "ABCD", 4U), "push 4");
    CHECK_EQ(dev_list_size(&g_list), 1U, "size 1");
    CHECK(dev_list_pop_head(&g_list, buf, ISZ), "pop");
    CHECK_EQ(memcmp(buf, "ABCD", 4U), 0U, "data match");
    CHECK_EQ(dev_list_size(&g_list), 0U, "size 0");
    printf("    PASS\n"); g_passes++;
}

/* ── 9: push_tail then pop_tail ── */
TEST(9_push_tail_pop_tail)
{
    uint8_t buf[ISZ];
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_tail(&g_list, "HELLO", 5U), "push 5");
    CHECK_EQ(dev_list_size(&g_list), 1U, "size 1");
    CHECK(dev_list_pop_tail(&g_list, buf, ISZ), "pop");
    CHECK_EQ(memcmp(buf, "HELLO", 5U), 0U, "data match");
    printf("    PASS\n"); g_passes++;
}

/* ── 10: push_head + push_tail order check ── */
TEST(10_order_head_tail)
{
    uint8_t buf[ISZ];
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_tail(&g_list, "FIRST", 5U), "tail FIRST");
    CHECK(dev_list_push_tail(&g_list, "LAST", 4U), "tail LAST");

    CHECK(dev_list_pop_head(&g_list, buf, ISZ), "pop head");
    CHECK_EQ(memcmp(buf, "FIRST", 5U), 0U, "head is FIRST");

    CHECK(dev_list_pop_head(&g_list, buf, ISZ), "pop head");
    CHECK_EQ(memcmp(buf, "LAST", 4U), 0U, "next is LAST");
    printf("    PASS\n"); g_passes++;
}

/* ── 11: push_head + push_tail → pop_tail reverse order ── */
TEST(11_order_tail_reverse)
{
    uint8_t buf[ISZ];
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_head(&g_list, "B", 1U), "head B");
    CHECK(dev_list_push_head(&g_list, "A", 1U), "head A");

    CHECK(dev_list_pop_tail(&g_list, buf, ISZ), "pop tail");
    CHECK(buf[0] == 'B', "tail is B");
    printf("    PASS\n"); g_passes++;
}

/* ── 12: fill to capacity then push fails ── */
TEST(12_capacity_exhaustion)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_head(&g_list, "1", 1U), "push 1");
    CHECK(dev_list_push_head(&g_list, "2", 1U), "push 2");
    CHECK(dev_list_push_head(&g_list, "3", 1U), "push 3");
    CHECK(dev_list_push_head(&g_list, "4", 1U), "push 4");
    CHECK(!dev_list_push_head(&g_list, "5", 1U), "push 5 fails");
    CHECK_EQ(dev_list_size(&g_list), CAP, "size CAP");
    printf("    PASS\n"); g_passes++;
}

/* ── 13: pop restores free node ── */
TEST(13_pop_restores_capacity)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_head(&g_list, "1", 1U), "push 1");
    CHECK(dev_list_push_head(&g_list, "2", 1U), "push 2");
    CHECK(dev_list_push_head(&g_list, "3", 1U), "push 3");
    CHECK(dev_list_push_head(&g_list, "4", 1U), "push 4");
    CHECK_EQ(dev_list_size(&g_list), CAP, "full");

    dev_list_remove_head(&g_list);   /* free one slot */
    CHECK_EQ(dev_list_size(&g_list), 3U, "size 3 after remove");

    CHECK(dev_list_push_tail(&g_list, "NEW", 3U), "push after remove");
    CHECK_EQ(dev_list_size(&g_list), 4U, "size 4 again");
    printf("    PASS\n"); g_passes++;
}

/* ── 14: remove_head on empty ── */
TEST(14_remove_head_empty)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    dev_list_remove_head(&g_list); /* no-op */
    CHECK_EQ(dev_list_size(&g_list), 0U, "still 0");
    printf("    PASS\n"); g_passes++;
}

/* ── 15: remove_tail on empty ── */
TEST(15_remove_tail_empty)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    dev_list_remove_tail(&g_list); /* no-op */
    CHECK_EQ(dev_list_size(&g_list), 0U, "still 0");
    printf("    PASS\n"); g_passes++;
}

/* ── 16: pop_head on empty ── */
TEST(16_pop_head_empty)
{
    uint8_t buf[ISZ];
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(!dev_list_pop_head(&g_list, buf, ISZ), "pop empty fails");
    printf("    PASS\n"); g_passes++;
}

/* ── 17: pop_tail on empty ── */
TEST(17_pop_tail_empty)
{
    uint8_t buf[ISZ];
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(!dev_list_pop_tail(&g_list, buf, ISZ), "pop empty fails");
    printf("    PASS\n"); g_passes++;
}

/* ── 18: push NULL data ── */
TEST(18_push_null_data)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(!dev_list_push_head(&g_list, NULL, 1U), "null data fail");
    CHECK(!dev_list_push_tail(&g_list, NULL, 1U), "null data fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 19: push zero data_size ── */
TEST(19_push_zero_size)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(!dev_list_push_head(&g_list, "X", 0U), "zero size fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 20: push data_size > item_size ── */
TEST(20_push_oversize)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(!dev_list_push_head(&g_list, "TOO_BIG!", ISZ + 1U), "oversize fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 21: pop with out_size too small ── */
TEST(21_pop_out_too_small)
{
    uint8_t buf[2];
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_tail(&g_list, "HELLO", 5U), "push 5 bytes");
    CHECK(!dev_list_pop_head(&g_list, buf, 2U), "pop 2 < 5 fails");
    printf("    PASS\n"); g_passes++;
}

/* ── 22: pop with NULL out_data ── */
TEST(22_pop_null_out)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_head(&g_list, "X", 1U), "push");
    CHECK(!dev_list_pop_head(&g_list, NULL, ISZ), "null out fail");
    printf("    PASS\n"); g_passes++;
}

/* ── 23: variable-size items round-trip ── */
TEST(23_variable_size)
{
    uint8_t buf[ISZ];
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_head(&g_list, "AB", 2U), "push 2");
    CHECK(dev_list_push_head(&g_list, "CDEFGH", 6U), "push 6");

    CHECK(dev_list_pop_head(&g_list, buf, ISZ), "pop 6");
    CHECK_EQ(memcmp(buf, "CDEFGH", 6U), 0U, "6 bytes matched");

    CHECK(dev_list_pop_head(&g_list, buf, ISZ), "pop 2");
    CHECK_EQ(memcmp(buf, "AB", 2U), 0U, "2 bytes matched");
    printf("    PASS\n"); g_passes++;
}

/* ── 24: destroy resets to empty ── */
TEST(24_destroy_resets)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_head(&g_list, "X", 1U), "push");
    CHECK(dev_list_push_head(&g_list, "Y", 1U), "push");
    dev_list_destroy(&g_list);
    CHECK_EQ(dev_list_size(&g_list), 0U, "size 0 after destroy");
    CHECK(dev_list_push_head(&g_list, "Z", 1U), "reuse after destroy");
    printf("    PASS\n"); g_passes++;
}

/* ── 25: remove_tail single element ── */
TEST(25_remove_tail_single)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_head(&g_list, "ONLY", 4U), "push");
    dev_list_remove_tail(&g_list);
    CHECK_EQ(dev_list_size(&g_list), 0U, "empty");
    printf("    PASS\n"); g_passes++;
}

/* ── 26: iteration via head/next ── */
TEST(26_iteration)
{
    const dev_list_node_t *n;
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK(dev_list_push_tail(&g_list, "A", 1U), "push A");
    CHECK(dev_list_push_tail(&g_list, "B", 1U), "push B");
    CHECK(dev_list_push_tail(&g_list, "C", 1U), "push C");

    n = dev_list_head(&g_list);
    CHECK(n != NULL, "head not null");
    CHECK_EQ(dev_list_node_data_size(n), 1U, "size A");

    n = dev_list_next(n);
    CHECK(n != NULL, "B not null");
    n = dev_list_next(n);
    CHECK(n != NULL, "C not null");
    n = dev_list_next(n);
    CHECK(n == NULL, "end null");
    printf("    PASS\n"); g_passes++;
}

/* ── 27: size after push/pop/remove sequence ── */
TEST(27_size_tracking)
{
    CHECK(dev_list_init(&g_list, g_nodes, g_data, CAP, ISZ), "init");
    CHECK_EQ(dev_list_size(&g_list), 0U, "0");
    dev_list_push_head(&g_list, "X", 1U);
    CHECK_EQ(dev_list_size(&g_list), 1U, "1");
    dev_list_push_tail(&g_list, "Y", 1U);
    CHECK_EQ(dev_list_size(&g_list), 2U, "2");
    dev_list_remove_head(&g_list);
    CHECK_EQ(dev_list_size(&g_list), 1U, "1");
    dev_list_remove_tail(&g_list);
    CHECK_EQ(dev_list_size(&g_list), 0U, "0");
    printf("    PASS\n"); g_passes++;
}

/* ── Main ── */

int main(void)
{
    printf("=== dev_list Test Suite ===\n\n");

    RUN_TEST(1_init_valid);
    RUN_TEST(2_init_null_list);
    RUN_TEST(3_init_null_nodes);
    RUN_TEST(4_init_null_data);
    RUN_TEST(5_init_zero_cap);
    RUN_TEST(6_init_zero_item);
    RUN_TEST(7_init_overflow);
    RUN_TEST(8_push_head_pop_head);
    RUN_TEST(9_push_tail_pop_tail);
    RUN_TEST(10_order_head_tail);
    RUN_TEST(11_order_tail_reverse);
    RUN_TEST(12_capacity_exhaustion);
    RUN_TEST(13_pop_restores_capacity);
    RUN_TEST(14_remove_head_empty);
    RUN_TEST(15_remove_tail_empty);
    RUN_TEST(16_pop_head_empty);
    RUN_TEST(17_pop_tail_empty);
    RUN_TEST(18_push_null_data);
    RUN_TEST(19_push_zero_size);
    RUN_TEST(20_push_oversize);
    RUN_TEST(21_pop_out_too_small);
    RUN_TEST(22_pop_null_out);
    RUN_TEST(23_variable_size);
    RUN_TEST(24_destroy_resets);
    RUN_TEST(25_remove_tail_single);
    RUN_TEST(26_iteration);
    RUN_TEST(27_size_tracking);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
