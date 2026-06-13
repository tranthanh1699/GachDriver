#include <stdio.h>
#include "dev_gpio.h"
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
    int _a = (int)(a); int _b = (int)(b);                   \
    if (_a != _b) {                                        \
        printf("  FAIL: %s (expected %d, got %d) (%s:%d)\n", \
               msg, _b, _a, __FILE__, __LINE__);            \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define CHECK_EQ_PTR(a, b, msg) do {                       \
    if ((a) != (b)) {                                      \
        printf("  FAIL: %s (%p vs %p) (%s:%d)\n",          \
               msg, (void*)(b), (void*)(a), __FILE__, __LINE__); \
        g_failures++;                                      \
        return;                                             \
    }                                                       \
} while (false)

#define RUN_TEST(name) do {                                \
    printf("  test_%s...\n", #name);                       \
    dev_gpio_port_mock_clear_error();                       \
    setup();                                                \
    test_##name();                                         \
} while (false)

static void setup(void)
{
    if (dev_gpio_is_initialized()) {
        (void)dev_gpio_deinit();
    }
    dev_gpio_port_mock_clear_error();
}

static dev_gpio_pin_t g_last_isr_pin;
static void          *g_last_isr_arg;
static int            g_isr_call_count;

static void test_isr_callback(dev_gpio_pin_t pin, void *user_arg)
{
    g_last_isr_pin  = pin;
    g_last_isr_arg  = user_arg;
    g_isr_call_count++;
}

static void reset_isr_state(void)
{
    g_last_isr_pin  = (dev_gpio_pin_t)0xFFFFU;
    g_last_isr_arg  = NULL;
    g_isr_call_count = 0;
}

/* ── Test 1 ── */
TEST(1_init_succeeds)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK(dev_gpio_is_initialized(), "is_initialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 2 ── */
TEST(2_double_init)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "first init");
    CHECK_EQ(dev_gpio_init(), DEV_ERR_ALREADY_INITIALIZED, "double init");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 3 ── */
TEST(3_output_high)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_high(DEV_GPIO_LED_STATUS), DEV_OK, "high");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_HIGH, "level HIGH");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 4 ── */
TEST(4_output_low)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_low(DEV_GPIO_LED_STATUS), DEV_OK, "low");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_LOW, "level LOW");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 5 ── */
TEST(5_output_level_high)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output_level(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH), DEV_OK, "output_level");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_HIGH, "level HIGH");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 6 ── */
TEST(6_output_level_low)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output_level(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_LOW), DEV_OK, "output_level");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_LOW, "level LOW");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 7 ── */
TEST(7_input)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK(!dev_gpio_port_mock_is_output(DEV_GPIO_BUTTON_USER), "is input");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 8 ── */
TEST(8_input_pullup)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input_pullup(DEV_GPIO_BUTTON_USER), DEV_OK, "input_pullup");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 9 ── */
TEST(9_input_pulldown)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input_pulldown(DEV_GPIO_BUTTON_USER), DEV_OK, "input_pulldown");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 10 ── */
TEST(10_write_valid)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH), DEV_OK, "write");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_HIGH, "level");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 11 ── */
TEST(11_write_invalid_level)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_write(DEV_GPIO_LED_STATUS, (dev_gpio_level_t)99U), DEV_ERR_INVALID_ARG, "invalid level");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 12 ── */
TEST(12_read_null)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_read(DEV_GPIO_LED_STATUS, NULL), DEV_ERR_NULL_PTR, "null ptr");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 13 ── */
TEST(13_read_valid)
{
    dev_gpio_level_t lvl;
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output_level(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH), DEV_OK, "output_level");
    CHECK_EQ(dev_gpio_read(DEV_GPIO_LED_STATUS, &lvl), DEV_OK, "read");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_HIGH, "level");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 14 ── */
TEST(14_toggle)
{
    dev_gpio_level_t lvl;
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_toggle(DEV_GPIO_LED_STATUS), DEV_OK, "toggle");
    CHECK_EQ(dev_gpio_read(DEV_GPIO_LED_STATUS, &lvl), DEV_OK, "read");
    CHECK_EQ(lvl, DEV_GPIO_LEVEL_HIGH, "HIGH after toggle from LOW");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 15 ── */
TEST(15_set_pull)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_set_pull(DEV_GPIO_BUTTON_USER, DEV_GPIO_PULL_UP), DEV_OK, "set_pull UP");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 16 ── */
TEST(16_high)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");
    CHECK_EQ(dev_gpio_high(DEV_GPIO_LED_STATUS), DEV_OK, "high");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_HIGH, "HIGH");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 17 ── */
TEST(17_low)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output_level(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH), DEV_OK, "out_lvl");
    CHECK_EQ(dev_gpio_low(DEV_GPIO_LED_STATUS), DEV_OK, "low");
    CHECK_EQ(dev_gpio_port_mock_get_level(DEV_GPIO_LED_STATUS), (int)DEV_GPIO_LEVEL_LOW, "LOW");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 18 ── */
TEST(18_interrupt_register)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, (void*)0x42U), DEV_OK, "interrupt");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 19 ── */
TEST(19_interrupt_clear)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_DISABLE,
                                NULL, NULL), DEV_OK, "clear");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 20 ── */
TEST(20_interrupt_null_cb_error)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                NULL, NULL), DEV_ERR_NULL_PTR, "null cb");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 21 ── */
TEST(21_interrupt_enable_trigger)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, (void*)0xDEADU), DEV_OK, "register");
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER), DEV_OK, "enable");

    dev_gpio_port_mock_trigger_isr(DEV_GPIO_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 1, "call count");
    CHECK_EQ(g_last_isr_pin, DEV_GPIO_BUTTON_USER, "correct pin");
    CHECK_EQ_PTR(g_last_isr_arg, (void*)0xDEADU, "correct arg");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 22 ── */
TEST(22_interrupt_disable)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER), DEV_OK, "enable");
    CHECK_EQ(dev_gpio_interrupt_disable(DEV_GPIO_BUTTON_USER), DEV_OK, "disable");

    g_isr_call_count = 0;
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "NOT called when disabled");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 23 ── */
TEST(23_deinit)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_deinit(), DEV_OK, "deinit");
    CHECK(!dev_gpio_is_initialized(), "uninitialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 24 ── */
TEST(24_reinit)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_deinit(), DEV_OK, "deinit");
    CHECK_EQ(dev_gpio_init(), DEV_OK, "reinit");
    CHECK(dev_gpio_is_initialized(), "initialized");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 25 ── */
TEST(25_pin_out_of_range)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output((dev_gpio_pin_t)DEV_GPIO_CFG_MAX_PINS),
             DEV_ERR_INVALID_ARG, "pin >= MAX");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 26 ── */
TEST(26_before_init)
{
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_ERR_NOT_INITIALIZED, "not init");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 27 ── */
TEST(27_error_injection)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_output(DEV_GPIO_LED_STATUS), DEV_OK, "output");

    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    CHECK_EQ(dev_gpio_write(DEV_GPIO_LED_STATUS, DEV_GPIO_LEVEL_HIGH),
             DEV_ERR_HW_FAILURE, "injected error");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 28 ── */
TEST(28_enable_rollback)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");

    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_HW_FAILURE, "enable fails");

    g_isr_call_count = 0;
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "NOT called after rollback");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 29 ── */
TEST(29_interrupt_disabled_build)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U)
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL),
             DEV_ERR_NOT_SUPPORTED, "interrupt disabled build");
#endif
    printf("    PASS\n"); g_passes++;
}

/* ── Test 30 ── */
TEST(30_enable_disable_disabled_build)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U)
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_NOT_SUPPORTED, "enable disabled build");
    CHECK_EQ(dev_gpio_interrupt_disable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_NOT_SUPPORTED, "disable disabled build");
#endif
    printf("    PASS\n"); g_passes++;
}

/* ── Test 31 ── */
TEST(31_pin_unmapped)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    /* Pin ID < MAX_PINS but >= PIN_COUNT — not in mock's range */
    CHECK_EQ(dev_gpio_output((dev_gpio_pin_t)(DEV_GPIO_CFG_PIN_COUNT + 1U)),
             DEV_ERR_INVALID_ARG, "unmapped pin");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 32 ── */
TEST(32_deinit_clears_callbacks)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER), DEV_OK, "enable");

    CHECK_EQ(dev_gpio_deinit(), DEV_OK, "deinit");
    CHECK_EQ(dev_gpio_init(), DEV_OK, "reinit");

    g_isr_call_count = 0;
    dev_gpio_port_mock_trigger_isr(DEV_GPIO_BUTTON_USER);
    CHECK_EQ(g_isr_call_count, 0, "callback NOT invoked after reinit");
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_INVALID_STATE, "enable fails without callback");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 33 ── */
TEST(33_enable_without_callback)
{
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_INVALID_STATE, "enable without callback");
    printf("    PASS\n"); g_passes++;
}

/* ── Test 34 ── */
TEST(34_disable_fail_blocks_enable)
{
    reset_isr_state();
    CHECK_EQ(dev_gpio_init(), DEV_OK, "init");
    CHECK_EQ(dev_gpio_input(DEV_GPIO_BUTTON_USER), DEV_OK, "input");
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_RISING_EDGE,
                                test_isr_callback, NULL), DEV_OK, "register");

    dev_gpio_port_mock_set_error(DEV_ERR_HW_FAILURE);
    CHECK_EQ(dev_gpio_interrupt(DEV_GPIO_BUTTON_USER, DEV_GPIO_INTR_DISABLE, NULL, NULL),
             DEV_ERR_HW_FAILURE, "disable fails");
    dev_gpio_port_mock_clear_error();

    CHECK_EQ(dev_gpio_interrupt_enable(DEV_GPIO_BUTTON_USER),
             DEV_ERR_INVALID_STATE, "enable blocked after failed disable");
    printf("    PASS\n"); g_passes++;
}

/* ── Main ── */

int main(void)
{
    dev_assert_config_t assert_cfg = {
        .backend = DEV_ASSERT_BACKEND_NONE,
        .output_hook = NULL, .user_hook = NULL, .reset_hook = NULL,
        .text_buffer = NULL, .text_buffer_size = 0U
    };
    dev_assert_init(&assert_cfg);

    printf("=== GPIO Simplified Wrapper Test Suite ===\n\n");

    RUN_TEST(1_init_succeeds);
    RUN_TEST(2_double_init);
    RUN_TEST(3_output_high);
    RUN_TEST(4_output_low);
    RUN_TEST(5_output_level_high);
    RUN_TEST(6_output_level_low);
    RUN_TEST(7_input);
    RUN_TEST(8_input_pullup);
    RUN_TEST(9_input_pulldown);
    RUN_TEST(10_write_valid);
    RUN_TEST(11_write_invalid_level);
    RUN_TEST(12_read_null);
    RUN_TEST(13_read_valid);
    RUN_TEST(14_toggle);
    RUN_TEST(15_set_pull);
    RUN_TEST(16_high);
    RUN_TEST(17_low);
    RUN_TEST(18_interrupt_register);
    RUN_TEST(19_interrupt_clear);
    RUN_TEST(20_interrupt_null_cb_error);
    RUN_TEST(21_interrupt_enable_trigger);
    RUN_TEST(22_interrupt_disable);
    RUN_TEST(23_deinit);
    RUN_TEST(24_reinit);
    RUN_TEST(25_pin_out_of_range);
    RUN_TEST(26_before_init);
    RUN_TEST(27_error_injection);
    RUN_TEST(28_enable_rollback);
    RUN_TEST(29_interrupt_disabled_build);
    RUN_TEST(30_enable_disable_disabled_build);
    RUN_TEST(31_pin_unmapped);
    RUN_TEST(32_deinit_clears_callbacks);
    RUN_TEST(33_enable_without_callback);
    RUN_TEST(34_disable_fail_blocks_enable);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
