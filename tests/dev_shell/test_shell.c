#include "dev_shell.h"
#include "dev_uart_port_mock.h"
#include <stdio.h>
#include <string.h>

static int g_failures = 0, g_passes = 0;
#define TEST(n) static void test_##n(void)
#define CHK(c,m) do{if(!(c)){printf("  FAIL: %s (%s:%d)\n",m,__FILE__,__LINE__);g_failures++;return;}}while(0)
#define EQ(a,b,m) do{int _a=(int)(a),_b=(int)(b);if(_a!=_b){printf("  FAIL: %s exp %d got %d (%s:%d)\n",m,_b,_a,__FILE__,__LINE__);g_failures++;return;}}while(0)
#define RUN(n) do{printf("  test_%s...\n",#n);dev_uart_port_mock_reset();if(dev_shell_is_initialized())dev_shell_deinit();test_##n();}while(0)

TEST(1_init)      { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); CHK(dev_shell_is_initialized(), "?"); printf("    PASS\n"); g_passes++; }
TEST(2_double)    { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "1st"); EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_ERR_ALREADY_INITIALIZED, "2nd"); printf("    PASS\n"); g_passes++; }
TEST(3_deinit)    { EQ(dev_shell_deinit(), DEV_ERR_NOT_INITIALIZED, "before"); printf("    PASS\n"); g_passes++; }
TEST(4_handle_before) { EQ(dev_shell_handle(), DEV_ERR_NOT_INITIALIZED, "before"); printf("    PASS\n"); g_passes++; }
TEST(5_write_before) { EQ(dev_shell_write("x"), DEV_ERR_NOT_INITIALIZED, "before"); printf("    PASS\n"); g_passes++; }
TEST(6_bad_uart)  { EQ(dev_shell_init(99), DEV_ERR_INVALID_ARG, "bad"); printf("    PASS\n"); g_passes++; }

/* 7: help cmd */
TEST(7_help) {
    EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init");
    const dev_shell_cmd_t *c;
    EQ(dev_shell_find_command("help", &c), DEV_OK, "find help");
    EQ(c->function(0U, NULL), DEV_OK, "help()");
    printf("    PASS\n"); g_passes++;
}

/* 8: explain cmd */
TEST(8_explain) {
    EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init");
    const dev_shell_cmd_t *c;
    EQ(dev_shell_find_command("explain", &c), DEV_OK, "find");
    EQ(c->function(0U, NULL), DEV_OK, "explain()");
    printf("    PASS\n"); g_passes++;
}

/* 9: unknown cmd */
TEST(9_unknown) {
    EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init");
    const dev_shell_cmd_t *c;
    EQ(dev_shell_find_command("nonexist", &c), DEV_ERR_NOT_FOUND, "not found");
    printf("    PASS\n"); g_passes++;
}

/* 10: parse args */
TEST(10_parse) {
    char line[64] = "cmd arg1 arg2";
    char *argv[4]; uint8_t argc;
    EQ(dev_shell_parse_line(line, argv, 4U, &argc), DEV_OK, "parse");
    EQ(argc, 3U, "3 args");
    CHK(strcmp(argv[0], "cmd") == 0, "argv0");
    CHK(strcmp(argv[1], "arg1") == 0, "argv1");
    printf("    PASS\n"); g_passes++;
}

/* 11-14: arg conversions */
TEST(11_arg_u32)  { uint32_t v; EQ(dev_shell_arg_to_u32("42", &v), DEV_OK, "u32"); EQ(v, 42U, "42"); printf("    PASS\n"); g_passes++; }
TEST(12_arg_i32)  { int32_t v;  EQ(dev_shell_arg_to_i32("-5", &v), DEV_OK, "i32"); EQ(v, -5, "-5"); printf("    PASS\n"); g_passes++; }
TEST(13_arg_hex)  { uint32_t v; EQ(dev_shell_arg_to_hex_u32("FF", &v), DEV_OK, "hex"); EQ(v, 255U, "255"); printf("    PASS\n"); g_passes++; }
TEST(14_arg_bool) { bool v; EQ(dev_shell_arg_to_bool("on", &v), DEV_OK, "bool"); CHK(v, "true"); EQ(dev_shell_arg_to_bool("off", &v), DEV_OK, "off"); CHK(!v, "false"); printf("    PASS\n"); g_passes++; }
TEST(15_bad_parse){ uint32_t v; EQ(dev_shell_arg_to_u32("abc", &v), DEV_ERR_PARSE, "bad"); printf("    PASS\n"); g_passes++; }
TEST(16_null_ptr) { EQ(dev_shell_parse_line(NULL, NULL, 1U, NULL), DEV_ERR_NULL_PTR, "null"); printf("    PASS\n"); g_passes++; }

/* 17: find_command null */
TEST(17_find_null) { const dev_shell_cmd_t *c; EQ(dev_shell_find_command(NULL, &c), DEV_ERR_NULL_PTR, "null"); printf("    PASS\n"); g_passes++; }

/* 18: handle no data */
TEST(18_handle_empty) { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); EQ(dev_shell_handle(), DEV_OK, "empty"); printf("    PASS\n"); g_passes++; }

int main(void)
{
    printf("=== dev_shell Test Suite ===\n\n");
    RUN(1_init); RUN(2_double); RUN(3_deinit); RUN(4_handle_before);
    RUN(5_write_before); RUN(6_bad_uart); RUN(7_help); RUN(8_explain);
    RUN(9_unknown); RUN(10_parse);
    RUN(11_arg_u32); RUN(12_arg_i32); RUN(13_arg_hex); RUN(14_arg_bool);
    RUN(15_bad_parse); RUN(16_null_ptr); RUN(17_find_null); RUN(18_handle_empty);
    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
