#include <stdio.h>
#include <string.h>
#include "dev_gpio.h"
#include "dev_gpio_board_cfg.h"
#include "dev_gpio_port_mock.h"
#include "dev_assert.h"

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)

#define CHECK(cond, msg) do {                              \
    if (!(cond)) {                                         \
        printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define CHECK_EQ(a, b, msg) do {                           \
    if ((a) != (b)) {                                      \
        printf("  FAIL: %s (expected %d, got %d) (%s:%d)\n", \
               msg, (int)(b), (int)(a), __FILE__, __LINE__); \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define CHECK_EQ_PTR(a, b, msg) do {                       \
    if ((a) != (b)) {                                      \
        printf("  FAIL: %s (expected %p, got %p) (%s:%d)\n", \
               msg, (void*)(b), (void*)(a), __FILE__, __LINE__); \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define RUN_TEST(name) do {                                \
    printf("  test_%s...\n", #name);                       \
    dev_gpio_port_mock_clear_error();                       \
    test_##name();                                         \
} while (false)

/* ── Setup / teardown helpers ── */

static void setup(void)
{
    if (dev_gpio_is_initialized()) {
        (void)dev_gpio_deinit();
    }
    dev_gpio_port_mock_clear_error();
}

/* ── ISR callback test helper ── */

static dev_gpio_channel_t g_last_isr_channel;
static void              *g_last_isr_arg;
static int                g_isr_call_count;

static void test_isr_callback(dev_gpio_channel_t channel, void *user_arg)
{
    g_last_isr_channel = channel;
    g_last_isr_arg     = user_arg;
    g_isr_call_count++;
}

static void reset_isr_state(void)
{
    g_last_isr_channel = (dev_gpio_channel_t)0xFFFFU;
    g_last_isr_arg     = NULL;
    g_isr_call_count   = 0;
}

/* ── Test 1: init with valid config ── */
TEST(1_init_valid_config)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    CHECK(dev_gpio_is_initialized(), "should be initialized");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 2: init with NULL config ── */
TEST(2_init_null_config)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(NULL);
    CHECK_EQ(err, DEV_ERR_NULL_PTR, "null config should fail");
    CHECK(!dev_gpio_is_initialized(), "should not be initialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 3: init with NULL channels ── */
TEST(3_init_null_channels)
{
    dev_err_t err;
    dev_gpio_config_t bad_cfg = { NULL, 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_NULL_PTR, "null channels should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 4: init with channel_count=0 ── */
TEST(4_init_zero_channels)
{
    dev_err_t err;
    dev_gpio_channel_config_t ch = {0};
    dev_gpio_config_t bad_cfg = { &ch, 0U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "zero channels should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 5: init with channel_count > MAX_CHANNELS ── */
TEST(5_init_too_many_channels)
{
    dev_err_t err;
    dev_gpio_channel_config_t ch = {0};
    dev_gpio_config_t bad_cfg = { &ch, DEV_GPIO_CFG_MAX_CHANNELS + 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "too many channels should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 6: init with sparse channel IDs ── */
TEST(6_init_sparse_channels)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "sparse channel init should succeed");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 7: unknown channel at runtime ── */
TEST(7_unknown_channel)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_read((dev_gpio_channel_t)99U, &lvl);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "unknown channel should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 8: init with invalid direction enum ── */
TEST(8_init_invalid_direction)
{
    dev_err_t err;
    dev_gpio_channel_config_t ch = {
        .channel = 0U, .direction = (dev_gpio_direction_t)99U,
        .pull = DEV_GPIO_PULL_NONE, .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt = DEV_GPIO_INTR_DISABLE, .callback = NULL, .callback_arg = NULL
    };
    dev_gpio_config_t bad_cfg = { &ch, 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "invalid direction should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 9: init with invalid pull enum ── */
TEST(9_init_invalid_pull)
{
    dev_err_t err;
    dev_gpio_channel_config_t ch = {
        .channel = 0U, .direction = DEV_GPIO_DIRECTION_INPUT,
        .pull = (dev_gpio_pull_t)99U, .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt = DEV_GPIO_INTR_DISABLE, .callback = NULL, .callback_arg = NULL
    };
    dev_gpio_config_t bad_cfg = { &ch, 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "invalid pull should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 10: init with invalid interrupt enum ── */
TEST(10_init_invalid_interrupt)
{
    dev_err_t err;
    dev_gpio_channel_config_t ch = {
        .channel = 0U, .direction = DEV_GPIO_DIRECTION_INPUT,
        .pull = DEV_GPIO_PULL_NONE, .default_level = DEV_GPIO_LEVEL_LOW,
        .interrupt = (dev_gpio_intr_type_t)99U, .callback = NULL, .callback_arg = NULL
    };
    dev_gpio_config_t bad_cfg = { &ch, 1U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "invalid interrupt should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 11: double init ── */
TEST(11_double_init)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "first init should succeed");
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_ERR_ALREADY_INITIALIZED, "double init should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 12: duplicate channel detection ── */
TEST(12_duplicate_channels)
{
    dev_err_t err;
    dev_gpio_channel_config_t chs[2] = {
        { .channel = 5U, .direction = DEV_GPIO_DIRECTION_OUTPUT,
          .pull = DEV_GPIO_PULL_NONE, .default_level = DEV_GPIO_LEVEL_LOW,
          .interrupt = DEV_GPIO_INTR_DISABLE, .callback = NULL, .callback_arg = NULL },
        { .channel = 5U, .direction = DEV_GPIO_DIRECTION_INPUT,
          .pull = DEV_GPIO_PULL_UP, .default_level = DEV_GPIO_LEVEL_LOW,
          .interrupt = DEV_GPIO_INTR_DISABLE, .callback = NULL, .callback_arg = NULL },
    };
    dev_gpio_config_t bad_cfg = { chs, 2U };
    setup();
    err = dev_gpio_init(&bad_cfg);
    CHECK_EQ(err, DEV_ERR_CONFIG, "duplicate channels should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 13: port init failure propagates ── */
TEST(13_port_init_failure)
{
    dev_err_t err;
    setup();
    dev_gpio_port_mock_set_error_for_op(DEV_GPIO_PORT_MOCK_OP_INIT, DEV_ERR_HW_FAILURE);
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "port init failure should propagate");
    CHECK(!dev_gpio_is_initialized(), "should remain uninitialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 14: port channel config failure + cleanup ── */
TEST(14_port_config_channel_failure)
{
    dev_err_t err;
    setup();
    dev_gpio_port_mock_set_error_for_op(DEV_GPIO_PORT_MOCK_OP_CONFIG_CHANNEL, DEV_ERR_HW_FAILURE);
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "config channel failure should propagate");
    CHECK(!dev_gpio_is_initialized(), "should remain uninitialized after cleanup");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 15: read before init ── */
TEST(15_read_before_init)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_ERR_NOT_INITIALIZED, "read before init should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 16: write before init ── */
TEST(16_write_before_init)
{
    dev_err_t err;
    setup();
    err = dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, DEV_GPIO_LEVEL_HIGH);
    CHECK_EQ(err, DEV_ERR_NOT_INITIALIZED, "write before init should fail");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 17: read with NULL level pointer ── */
TEST(17_read_null_level)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, NULL);
    CHECK_EQ(err, DEV_ERR_NULL_PTR, "null level pointer should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 18: read from unknown channel ── */
TEST(18_read_unknown_channel)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_read((dev_gpio_channel_t)99U, &lvl);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "unknown channel read should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 19: write with invalid level ── */
TEST(19_write_invalid_level)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, (dev_gpio_level_t)99U);
    CHECK_EQ(err, DEV_ERR_INVALID_ARG, "invalid level should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 20: write to input-only channel ── */
TEST(20_write_to_input)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_write(DEV_GPIO_CHANNEL_BUTTON_USER, DEV_GPIO_LEVEL_HIGH);
    CHECK_EQ(err, DEV_ERR_INVALID_STATE, "write to input should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 21: toggle input-only channel ── */
TEST(21_toggle_input)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_toggle(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_ERR_INVALID_STATE, "toggle input should fail");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 22: read from output channel (default level) ── */
TEST(22_read_output_default)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_OK, "read output should succeed");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_LOW, "default output should be LOW");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 23: write then read output ── */
TEST(23_write_then_read)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, DEV_GPIO_LEVEL_HIGH);
    CHECK_EQ(err, DEV_OK, "write should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_OK, "read should succeed");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_HIGH, "level should be HIGH");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 24: toggle output ── */
TEST(24_toggle_output)
{
    dev_err_t err;
    dev_gpio_level_t lvl;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_toggle(DEV_GPIO_CHANNEL_LED_STATUS);
    CHECK_EQ(err, DEV_OK, "toggle should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_OK, "read should succeed");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_HIGH, "should be HIGH after toggle from LOW");
    err = dev_gpio_toggle(DEV_GPIO_CHANNEL_LED_STATUS);
    CHECK_EQ(err, DEV_OK, "second toggle should succeed");
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_OK, "read should succeed");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_LOW, "should be LOW after toggle from HIGH");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 25: set pull mode ── */
TEST(25_set_pull)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_set_pull(DEV_GPIO_CHANNEL_BUTTON_USER, DEV_GPIO_PULL_UP);
    CHECK_EQ(err, DEV_OK, "set pull should succeed");
    CHECK_EQ(dev_gpio_port_mock_get_pull(DEV_GPIO_CHANNEL_BUTTON_USER),
             DEV_GPIO_PULL_UP, "pull should be UP");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 26: register callback ── */
TEST(26_register_callback)
{
    dev_err_t err;
    setup();
    reset_isr_state();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER, test_isr_callback, (void *)0x1234U);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 27: enable interrupt, mock trigger ISR ── */
TEST(27_enable_interrupt_and_trigger)
{
    dev_err_t err;
    setup();
    reset_isr_state();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER, test_isr_callback, (void *)0xABCDU);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    err = dev_gpio_enable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_OK, "enable interrupt should succeed");
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 1, "callback should be called once");
    CHECK_EQ(g_last_isr_channel, DEV_GPIO_CHANNEL_BUTTON_USER, "correct channel");
    CHECK_EQ_PTR(g_last_isr_arg, (void *)0xABCDU, "correct user arg");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 28: disable interrupt ── */
TEST(28_disable_interrupt)
{
    dev_err_t err;
    setup();
    reset_isr_state();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER, test_isr_callback, NULL);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    err = dev_gpio_enable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_OK, "enable should succeed");
    err = dev_gpio_disable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_OK, "disable should succeed");
    g_isr_call_count = 0;
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "callback should NOT be called when disabled");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 29: deinit ── */
TEST(29_deinit)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_deinit();
    CHECK_EQ(err, DEV_OK, "deinit should succeed");
    CHECK(!dev_gpio_is_initialized(), "should be uninitialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 30: reinit after deinit ── */
TEST(30_reinit_after_deinit)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "first init should succeed");
    err = dev_gpio_deinit();
    CHECK_EQ(err, DEV_OK, "deinit should succeed");
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "reinit should succeed");
    CHECK(dev_gpio_is_initialized(), "should be initialized");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 31: unsupported both-edges interrupt ── */
TEST(31_unsupported_both_edges)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_config_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER, DEV_GPIO_INTR_BOTH_EDGES);
    CHECK_EQ(err, DEV_ERR_NOT_SUPPORTED, "both-edges should be unsupported");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 32: unsupported pull-down ── */
TEST(32_unsupported_pull_down)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_set_pull(DEV_GPIO_CHANNEL_BUTTON_USER, DEV_GPIO_PULL_DOWN);
    CHECK_EQ(err, DEV_ERR_NOT_SUPPORTED, "pull-down should be unsupported");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 33: error injection — port write fails ── */
TEST(33_error_injection_write)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    err = dev_gpio_write(DEV_GPIO_CHANNEL_LED_STATUS, DEV_GPIO_LEVEL_HIGH);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "port write failure should propagate");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 34: error injection — port read fails, *level unchanged ── */
TEST(34_error_injection_read)
{
    dev_err_t err;
    dev_gpio_level_t lvl = (dev_gpio_level_t)0xFFU;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    err = dev_gpio_read(DEV_GPIO_CHANNEL_LED_STATUS, &lvl);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "port read failure should propagate");
    CHECK((lvl != DEV_GPIO_LEVEL_LOW) && (lvl != DEV_GPIO_LEVEL_HIGH),
          "level should be unchanged on failure");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 35: enable interrupt — port fails, common rolls back ── */
TEST(35_enable_interrupt_port_fail_rollback)
{
    dev_err_t err;
    setup();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER, test_isr_callback, NULL);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    dev_gpio_port_mock_set_error_for_op(DEV_GPIO_PORT_MOCK_OP_ENABLE_INTERRUPT, DEV_ERR_HW_FAILURE);
    err = dev_gpio_enable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "enable should fail with HW_FAILURE");
    reset_isr_state();
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "callback should NOT be called after rollback");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Test 36: deinit with port failure, forced UNINITIALIZED ── */
TEST(36_deinit_port_fail_forced_uninit)
{
    dev_err_t err;
    setup();
    reset_isr_state();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "init should succeed");
    err = dev_gpio_register_callback(DEV_GPIO_CHANNEL_BUTTON_USER, test_isr_callback, (void *)0xDEADU);
    CHECK_EQ(err, DEV_OK, "register callback should succeed");
    err = dev_gpio_enable_interrupt(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(err, DEV_OK, "enable should succeed");
    dev_gpio_port_mock_set_error_for_op(DEV_GPIO_PORT_MOCK_OP_DEINIT, DEV_ERR_HW_FAILURE);
    err = dev_gpio_deinit();
    CHECK_EQ(err, DEV_ERR_HW_FAILURE, "deinit should return port error");
    CHECK(!dev_gpio_is_initialized(), "state must be UNINITIALIZED regardless");
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_CHANNEL_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "callbacks should be cleared");
    dev_gpio_port_mock_clear_error();
    err = dev_gpio_init(&g_dev_gpio_config);
    CHECK_EQ(err, DEV_OK, "reinit after failed deinit should succeed");
    (void)dev_gpio_deinit();
    printf("    PASS\n"); g_passes++;
}

/* ── Main ── */
int main(void)
{
    dev_assert_config_t assert_cfg = {
        .backend = DEV_ASSERT_BACKEND_NONE,
        .output_hook = NULL,
        .user_hook = NULL,
        .reset_hook = NULL,
        .text_buffer = NULL,
        .text_buffer_size = 0U
    };
    dev_assert_init(&assert_cfg);

    printf("=== GPIO Driver Test Suite ===\n\n");

    RUN_TEST(1_init_valid_config);
    RUN_TEST(2_init_null_config);
    RUN_TEST(3_init_null_channels);
    RUN_TEST(4_init_zero_channels);
    RUN_TEST(5_init_too_many_channels);
    RUN_TEST(6_init_sparse_channels);
    RUN_TEST(7_unknown_channel);
    RUN_TEST(8_init_invalid_direction);
    RUN_TEST(9_init_invalid_pull);
    RUN_TEST(10_init_invalid_interrupt);
    RUN_TEST(11_double_init);
    RUN_TEST(12_duplicate_channels);
    RUN_TEST(13_port_init_failure);
    RUN_TEST(14_port_config_channel_failure);
    RUN_TEST(15_read_before_init);
    RUN_TEST(16_write_before_init);
    RUN_TEST(17_read_null_level);
    RUN_TEST(18_read_unknown_channel);
    RUN_TEST(19_write_invalid_level);
    RUN_TEST(20_write_to_input);
    RUN_TEST(21_toggle_input);
    RUN_TEST(22_read_output_default);
    RUN_TEST(23_write_then_read);
    RUN_TEST(24_toggle_output);
    RUN_TEST(25_set_pull);
    RUN_TEST(26_register_callback);
    RUN_TEST(27_enable_interrupt_and_trigger);
    RUN_TEST(28_disable_interrupt);
    RUN_TEST(29_deinit);
    RUN_TEST(30_reinit_after_deinit);
    RUN_TEST(31_unsupported_both_edges);
    RUN_TEST(32_unsupported_pull_down);
    RUN_TEST(33_error_injection_write);
    RUN_TEST(34_error_injection_read);
    RUN_TEST(35_enable_interrupt_port_fail_rollback);
    RUN_TEST(36_deinit_port_fail_forced_uninit);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);

    return (g_failures > 0) ? 1 : 0;
}
