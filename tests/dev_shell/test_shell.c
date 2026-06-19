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

/* ── Runtime command tests ── */

static dev_err_t rt_cmd_fn(uint8_t argc, char *argv[]) { (void)argc; (void)argv; return DEV_OK; }
static const dev_shell_cmd_t rt_cmd = { "adc", "Read ADC", "adc <ch>", rt_cmd_fn };

TEST(19_reg_ok)      { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); EQ(dev_shell_register_command(&rt_cmd), DEV_OK, "reg"); printf("    PASS\n"); g_passes++; }
TEST(20_reg_null)    { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); EQ(dev_shell_register_command(NULL), DEV_ERR_NULL_PTR, "null"); printf("    PASS\n"); g_passes++; }
TEST(21_null_name)   { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); dev_shell_cmd_t c={NULL, "h", "u", rt_cmd_fn}; EQ(dev_shell_register_command(&c), DEV_ERR_NULL_PTR, "null name"); printf("    PASS\n"); g_passes++; }
TEST(22_empty_name)  { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); dev_shell_cmd_t c={"", "h", "u", rt_cmd_fn}; EQ(dev_shell_register_command(&c), DEV_ERR_INVALID_ARG, "empty"); printf("    PASS\n"); g_passes++; }
TEST(23_space_name)  { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); dev_shell_cmd_t c={"ab c", "h", "u", rt_cmd_fn}; EQ(dev_shell_register_command(&c), DEV_ERR_INVALID_ARG, "spc"); printf("    PASS\n"); g_passes++; }
TEST(24_tab_name)    { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); dev_shell_cmd_t c={"ab\tc", "h", "u", rt_cmd_fn}; EQ(dev_shell_register_command(&c), DEV_ERR_INVALID_ARG, "tab"); printf("    PASS\n"); g_passes++; }
TEST(25_null_fn)     { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); dev_shell_cmd_t c={"x", "h", "u", NULL}; EQ(dev_shell_register_command(&c), DEV_ERR_NULL_PTR, "fn"); printf("    PASS\n"); g_passes++; }
TEST(26_dup_static)  { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); dev_shell_cmd_t c={"help", "h", "u", rt_cmd_fn}; EQ(dev_shell_register_command(&c), DEV_ERR_CONFIG, "dup static"); printf("    PASS\n"); g_passes++; }
TEST(27_dup_runtime) { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); EQ(dev_shell_register_command(&rt_cmd), DEV_OK, "1st"); EQ(dev_shell_register_command(&rt_cmd), DEV_ERR_CONFIG, "2nd"); printf("    PASS\n"); g_passes++; }
TEST(28_exec_runtime){ EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); EQ(dev_shell_register_command(&rt_cmd), DEV_OK, "reg"); const dev_shell_cmd_t *c; EQ(dev_shell_find_command("adc", &c), DEV_OK, "find"); EQ(c->function(0,NULL), DEV_OK, "exec"); printf("    PASS\n"); g_passes++; }
TEST(29_priority)    { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); dev_shell_cmd_t c={"help", "h", "u", rt_cmd_fn}; EQ(dev_shell_register_command(&c), DEV_ERR_CONFIG, "dup"); const dev_shell_cmd_t *f; EQ(dev_shell_find_command("help", &f), DEV_OK, "find"); CHK(f->function != rt_cmd_fn, "static wins"); printf("    PASS\n"); g_passes++; }
TEST(30_unreg)       { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); EQ(dev_shell_register_command(&rt_cmd), DEV_OK, "reg"); EQ(dev_shell_unregister_command("adc"), DEV_OK, "unreg"); printf("    PASS\n"); g_passes++; }
TEST(31_unreg_nf)    { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); EQ(dev_shell_unregister_command("nonexist"), DEV_ERR_NOT_FOUND, "nf"); printf("    PASS\n"); g_passes++; }
TEST(32_unreg_static){ EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); EQ(dev_shell_unregister_command("help"), DEV_ERR_CONFIG, "static"); printf("    PASS\n"); g_passes++; }
TEST(33_unreg_all)   { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); EQ(dev_shell_register_command(&rt_cmd), DEV_OK, "reg"); EQ(dev_shell_unregister_all_runtime_commands(), DEV_OK, "all"); EQ(dev_shell_get_runtime_command_count(), 0U, "0"); printf("    PASS\n"); g_passes++; }
TEST(34_compact)     { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); dev_shell_cmd_t a={"a","h","u",rt_cmd_fn}; dev_shell_cmd_t b={"b","h","u",rt_cmd_fn}; dev_shell_cmd_t c={"c","h","u",rt_cmd_fn}; EQ(dev_shell_register_command(&a), DEV_OK, "a"); EQ(dev_shell_register_command(&b), DEV_OK, "b"); EQ(dev_shell_register_command(&c), DEV_OK, "c"); EQ(dev_shell_unregister_command("b"), DEV_OK, "unreg b"); const dev_shell_cmd_t *f; EQ(dev_shell_find_command("c", &f), DEV_OK, "c found"); printf("    PASS\n"); g_passes++; }

#if (DEV_SHELL_CFG_MAX_RUNTIME_COMMANDS < 32U)
TEST(35_fill_table)  { EQ(dev_shell_init(DEV_UART_CONSOLE), DEV_OK, "init"); char nm[32][3]={"a0","a1","a2","a3","a4","a5","a6","a7","a8","a9","b0","b1","b2","b3","b4","b5","b6","b7","b8","b9","c0","c1","c2","c3","c4","c5","c6","c7","c8","c9","d0","d1"}; dev_shell_cmd_t c={"a0","h","u",rt_cmd_fn}; uint16_t n=0U; while(n<DEV_SHELL_CFG_MAX_RUNTIME_COMMANDS){c.name=nm[n]; EQ(dev_shell_register_command(&c),DEV_OK,"add");n++;} EQ(dev_shell_register_command(&rt_cmd),DEV_ERR_OVERFLOW,"full"); EQ(dev_shell_unregister_all_runtime_commands(),DEV_OK,"clr"); printf("    PASS\n"); g_passes++; }
#endif

int main(void)
{
    printf("=== dev_shell Test Suite ===\n\n");
    RUN(1_init); RUN(2_double); RUN(3_deinit); RUN(4_handle_before);
    RUN(5_write_before); RUN(6_bad_uart); RUN(7_help); RUN(8_explain);
    RUN(9_unknown); RUN(10_parse);
    RUN(11_arg_u32); RUN(12_arg_i32); RUN(13_arg_hex); RUN(14_arg_bool);
    RUN(15_bad_parse); RUN(16_null_ptr); RUN(17_find_null); RUN(18_handle_empty);
    RUN(19_reg_ok); RUN(20_reg_null); RUN(21_null_name); RUN(22_empty_name);
    RUN(23_space_name); RUN(24_tab_name); RUN(25_null_fn); RUN(26_dup_static);
    RUN(27_dup_runtime); RUN(28_exec_runtime); RUN(29_priority); RUN(30_unreg);
    RUN(31_unreg_nf); RUN(32_unreg_static); RUN(33_unreg_all); RUN(34_compact);
#if (DEV_SHELL_CFG_MAX_RUNTIME_COMMANDS < 32U)
    RUN(35_fill_table);
#endif
    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
