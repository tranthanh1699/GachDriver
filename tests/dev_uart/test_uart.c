#include "dev_uart.h"
#include "dev_uart_port_mock.h"
#include <stdio.h>
#include <string.h>

static int g_failures = 0, g_passes = 0;
#define TEST(n) static void test_##n(void)
#define CHK(c,m) do{if(!(c)){printf("  FAIL: %s (%s:%d)\n",m,__FILE__,__LINE__);g_failures++;return;}}while(0)
#define EQ(a,b,m) do{int _a=(int)(a),_b=(int)(b);if(_a!=_b){printf("  FAIL: %s exp %d got %d (%s:%d)\n",m,_b,_a,__FILE__,__LINE__);g_failures++;return;}}while(0)
#define RUN(n) do{printf("  test_%s...\n",#n);dev_uart_port_mock_reset();if(dev_uart_is_initialized())dev_uart_deinit();test_##n();}while(0)

#define UART DEV_UART_CONSOLE
#define TO   DEV_UART_TIMEOUT_DEFAULT_MS
#define NOWAIT DEV_UART_TIMEOUT_NO_WAIT

/* 1-3: lifecycle */
TEST(1_init)     { EQ(dev_uart_init(), DEV_OK, "init"); CHK(dev_uart_is_initialized(),"?"); printf("    PASS\n"); g_passes++; }
TEST(2_double)   { EQ(dev_uart_init(), DEV_OK, "1st"); EQ(dev_uart_init(), DEV_ERR_ALREADY_INITIALIZED,"2nd"); printf("    PASS\n"); g_passes++; }
TEST(3_deinit)   { EQ(dev_uart_deinit(), DEV_ERR_NOT_INITIALIZED, "before"); printf("    PASS\n"); g_passes++; }

/* 4-5: before init */
TEST(4_write_before) { EQ(dev_uart_write(UART, (uint8_t*)"X", 1U, TO), DEV_ERR_NOT_INITIALIZED, "w"); printf("    PASS\n"); g_passes++; }
TEST(5_read_before)  { uint16_t n; EQ(dev_uart_read(UART, (uint8_t*)"X", 1U, &n, TO), DEV_ERR_NOT_INITIALIZED, "r"); printf("    PASS\n"); g_passes++; }

/* 6-9: invalid args */
TEST(6_bad_uart) { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_write(99, (uint8_t*)"X", 1U, TO), DEV_ERR_INVALID_ARG, "id"); printf("    PASS\n"); g_passes++; }
TEST(7_null_w)   { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_write(UART, NULL, 1U, TO), DEV_ERR_NULL_PTR, "null"); printf("    PASS\n"); g_passes++; }
TEST(8_null_r)   { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_read(UART, NULL, 1U, NULL, TO), DEV_ERR_NULL_PTR, "null"); printf("    PASS\n"); g_passes++; }
TEST(9_zero_len) { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_write(UART, (uint8_t*)"X", 0U, TO), DEV_ERR_INVALID_ARG, "0"); printf("    PASS\n"); g_passes++; }

/* 10-11: write/read success */
TEST(10_write_ok) { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_write(UART, (uint8_t*)"AB", 2U, TO), DEV_OK, "w"); printf("    PASS\n"); g_passes++; }
TEST(11_read_ok)  { EQ(dev_uart_init(), DEV_OK, "init"); dev_uart_port_mock_push_rx_byte(UART, 'H'); dev_uart_port_mock_push_rx_byte(UART, 'i');
    uint8_t b[2]; uint16_t n; EQ(dev_uart_read(UART, b, 2U, &n, TO), DEV_OK, "r"); EQ(n,2U,"n"); EQ(b[0],'H',"H"); EQ(b[1],'i',"i"); printf("    PASS\n"); g_passes++; }

/* 12-13: byte + string */
TEST(12_byte)     { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_write_byte(UART, 'Z', TO), DEV_OK, "wb"); printf("    PASS\n"); g_passes++; }
TEST(13_string)   { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_write_string(UART, "Hi!", TO), DEV_OK, "ws"); printf("    PASS\n"); g_passes++; }

/* 14-15: read no-wait empty + timeout */
TEST(14_empty)    { EQ(dev_uart_init(), DEV_OK, "init"); uint16_t n; EQ(dev_uart_read(UART, (uint8_t*)"X", 1U, &n, NOWAIT), DEV_ERR_EMPTY, "empty"); printf("    PASS\n"); g_passes++; }
/* 16: rx_available */
TEST(16_avail)    { EQ(dev_uart_init(), DEV_OK, "init"); EQ((int)dev_uart_rx_available(UART), 0, "0"); dev_uart_port_mock_push_rx_byte(UART, 'A'); EQ((int)dev_uart_rx_available(UART), 1, "1"); printf("    PASS\n"); g_passes++; }
/* 17-18: flush */
TEST(17_flush_rx) { EQ(dev_uart_init(), DEV_OK, "init"); dev_uart_port_mock_push_rx_byte(UART, 'A'); EQ(dev_uart_flush_rx(UART), DEV_OK, "flush"); EQ((int)dev_uart_rx_available(UART), 0, "0"); printf("    PASS\n"); g_passes++; }
TEST(18_flush_tx) { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_flush_tx(UART), DEV_OK, "ftx"); printf("    PASS\n"); g_passes++; }
/* 19-20: rx start/stop */
TEST(19_rx_start) { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_rx_start(UART), DEV_OK, "start"); printf("    PASS\n"); g_passes++; }
TEST(20_rx_stop)  { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_rx_stop(UART), DEV_OK, "stop"); printf("    PASS\n"); g_passes++; }
/* 21: config */
TEST(21_config)   { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_config(UART, DEV_UART_BAUDRATE_9600, DEV_UART_DATA_BITS_8, DEV_UART_STOP_BITS_1, DEV_UART_PARITY_NONE, DEV_UART_FLOW_CONTROL_NONE), DEV_OK, "cfg"); printf("    PASS\n"); g_passes++; }
/* 22: set_baudrate */
TEST(22_baud)     { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_set_baudrate(UART, DEV_UART_BAUDRATE_38400), DEV_OK, "baud"); printf("    PASS\n"); g_passes++; }
/* 23: invalid enum */
TEST(23_bad_enum) { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_config(UART, 0U, 99, 99, 99, 99), DEV_ERR_INVALID_ARG, "enum"); printf("    PASS\n"); g_passes++; }
/* 24: flow control unsupported on mock */
TEST(24_flow)     { EQ(dev_uart_init(), DEV_OK, "init"); EQ(dev_uart_config(UART, DEV_UART_BAUDRATE_115200, DEV_UART_DATA_BITS_8, DEV_UART_STOP_BITS_1, DEV_UART_PARITY_NONE, DEV_UART_FLOW_CONTROL_RTS), DEV_ERR_NOT_SUPPORTED, "rts"); printf("    PASS\n"); g_passes++; }

int main(void)
{
    printf("=== dev_uart Test Suite ===\n\n");
    RUN(1_init); RUN(2_double); RUN(3_deinit); RUN(4_write_before); RUN(5_read_before);
    RUN(6_bad_uart); RUN(7_null_w); RUN(8_null_r); RUN(9_zero_len);
    RUN(10_write_ok); RUN(11_read_ok); RUN(12_byte); RUN(13_string);
    RUN(14_empty); RUN(16_avail); RUN(17_flush_rx); RUN(18_flush_tx);
    RUN(19_rx_start); RUN(20_rx_stop); RUN(21_config); RUN(22_baud); RUN(23_bad_enum); RUN(24_flow);
    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
