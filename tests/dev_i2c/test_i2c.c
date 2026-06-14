#include "dev_i2c.h"
#include "dev_i2c_port_mock.h"
#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)
#define CHECK(cond, msg) do { if (!(cond)) { printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); g_failures++; return; } } while (false)
#define CHECK_EQ(a, b, msg) do { int _a=(int)(a),_b=(int)(b); if(_a!=_b){printf("  FAIL: %s (exp %d, got %d) (%s:%d)\n",msg,_b,_a,__FILE__,__LINE__);g_failures++;return;} } while(false)
#define CHECK_NEQ(a, b, msg) do { int _a=(int)(a),_b=(int)(b); if(_a==_b){printf("  FAIL: %s (got %d) (%s:%d)\n",msg,_a,__FILE__,__LINE__);g_failures++;return;} } while(false)
#define RUN_TEST(name) do { printf("  test_%s...\n", #name); dev_i2c_port_mock_reset(); if(dev_i2c_is_initialized()) dev_i2c_deinit(); test_##name(); } while(false)

static uint8_t g_mem[256];
static uint8_t g_buf[16];

#define ADDR  ((dev_i2c_addr_t)0x48U)
#define BUS   DEV_I2C_BUS_SENSOR
#define TO    DEV_I2C_TIMEOUT_DEFAULT_MS

/* 1: init */
TEST(1_init_ok)  { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); CHECK(dev_i2c_is_initialized(), "init?"); printf("    PASS\n"); g_passes++; }
/* 2: double init */
TEST(2_double_init) { CHECK_EQ(dev_i2c_init(), DEV_OK, "1st"); CHECK_EQ(dev_i2c_init(), DEV_ERR_ALREADY_INITIALIZED, "2nd"); printf("    PASS\n"); g_passes++; }
/* 3: deinit before init */
TEST(3_deinit_before) { CHECK_EQ(dev_i2c_deinit(), DEV_ERR_NOT_INITIALIZED, "deinit"); printf("    PASS\n"); g_passes++; }
/* 4: write before init */
TEST(4_write_before) { CHECK_EQ(dev_i2c_write(BUS, ADDR, g_buf, 1U, TO), DEV_ERR_NOT_INITIALIZED, "write"); printf("    PASS\n"); g_passes++; }
/* 5: read before init */
TEST(5_read_before) { CHECK_EQ(dev_i2c_read(BUS, ADDR, g_buf, 1U, TO), DEV_ERR_NOT_INITIALIZED, "read"); printf("    PASS\n"); g_passes++; }
/* 6: invalid bus */
TEST(6_invalid_bus) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); CHECK_EQ(dev_i2c_write(99, ADDR, g_buf, 1U, TO), DEV_ERR_INVALID_ARG, "bus"); printf("    PASS\n"); g_passes++; }
/* 7: invalid address */
TEST(7_invalid_addr) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); CHECK_EQ(dev_i2c_write(BUS, 0x80U, g_buf, 1U, TO), DEV_ERR_INVALID_ARG, "addr"); printf("    PASS\n"); g_passes++; }
/* 8: null write buffer */
TEST(8_null_write) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); CHECK_EQ(dev_i2c_write(BUS, ADDR, NULL, 1U, TO), DEV_ERR_NULL_PTR, "null"); printf("    PASS\n"); g_passes++; }
/* 9: null read buffer */
TEST(9_null_read) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); CHECK_EQ(dev_i2c_read(BUS, ADDR, NULL, 1U, TO), DEV_ERR_NULL_PTR, "null"); printf("    PASS\n"); g_passes++; }
/* 10: zero length */
TEST(10_zero_len) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); CHECK_EQ(dev_i2c_write(BUS, ADDR, g_buf, 0U, TO), DEV_ERR_INVALID_ARG, "zero"); printf("    PASS\n"); g_passes++; }
/* 11-12: write/read success */
TEST(11_write_ok) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); dev_i2c_port_mock_attach_device(BUS, ADDR, g_mem, 256U); CHECK_EQ(dev_i2c_write(BUS, ADDR, (uint8_t*)"HELLO", 5U, TO), DEV_OK, "write"); printf("    PASS\n"); g_passes++; }
TEST(12_read_ok)  { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); memcpy(g_mem, "WORLD", 5); dev_i2c_port_mock_attach_device(BUS, ADDR, g_mem, 256U); CHECK_EQ(dev_i2c_read(BUS, ADDR, g_buf, 5U, TO), DEV_OK, "read"); CHECK_EQ(memcmp(g_buf,"WORLD",5),0,"data"); printf("    PASS\n"); g_passes++; }
/* 13: write-read */
TEST(13_write_read) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); dev_i2c_port_mock_attach_device(BUS, ADDR, g_mem, 256U); CHECK_EQ(dev_i2c_write_read(BUS, ADDR, (uint8_t*)"CMD", 3U, g_buf, 4U, TO), DEV_OK, "wr"); printf("    PASS\n"); g_passes++; }
/* 14-15: mem write/read 8-bit */
TEST(14_mem_write8) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); dev_i2c_port_mock_attach_device(BUS, ADDR, g_mem, 256U); CHECK_EQ(dev_i2c_mem_write(BUS, ADDR, 0x10U, DEV_I2C_MEM_ADDR_SIZE_8BIT, (uint8_t*)"AB", 2U, TO), DEV_OK, "memw"); printf("    PASS\n"); g_passes++; }
TEST(15_mem_read8)  { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); memcpy(&g_mem[0x10],"XY",2); dev_i2c_port_mock_attach_device(BUS, ADDR, g_mem, 256U); CHECK_EQ(dev_i2c_mem_read(BUS, ADDR, 0x10U, DEV_I2C_MEM_ADDR_SIZE_8BIT, g_buf, 2U, TO), DEV_OK, "memr"); CHECK_EQ(memcmp(g_buf,"XY",2),0,"data"); printf("    PASS\n"); g_passes++; }
/* 16-17: mem write/read 16-bit */
TEST(16_mem_write16) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); dev_i2c_port_mock_attach_device(BUS, ADDR, g_mem, 256U); CHECK_EQ(dev_i2c_mem_write(BUS, ADDR, 0x00F0U, DEV_I2C_MEM_ADDR_SIZE_16BIT, (uint8_t*)"CD", 2U, TO), DEV_OK, "mw16"); printf("    PASS\n"); g_passes++; }
TEST(17_mem_read16)  { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); memcpy(&g_mem[0xF0],"EF",2); dev_i2c_port_mock_attach_device(BUS, ADDR, g_mem, 256U); CHECK_EQ(dev_i2c_mem_read(BUS, ADDR, 0x00F0U, DEV_I2C_MEM_ADDR_SIZE_16BIT, g_buf, 2U, TO), DEV_OK, "mr16"); CHECK_EQ(memcmp(g_buf,"EF",2),0,"data"); printf("    PASS\n"); g_passes++; }
/* 18: probe ok */
TEST(18_probe_ok) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); dev_i2c_port_mock_attach_device(BUS, ADDR, g_mem, 256U); CHECK_EQ(dev_i2c_probe(BUS, ADDR, TO), DEV_OK, "probe"); printf("    PASS\n"); g_passes++; }
/* 19: probe NACK */
TEST(19_probe_nack) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); CHECK_EQ(dev_i2c_probe(BUS, ADDR, TO), DEV_ERR_NO_ACK, "nack"); printf("    PASS\n"); g_passes++; }
/* 20: timeout */
TEST(20_timeout) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); dev_i2c_port_mock_set_error(DEV_ERR_TIMEOUT); CHECK_EQ(dev_i2c_write(BUS, ADDR, g_buf, 1U, TO), DEV_ERR_TIMEOUT, "to"); printf("    PASS\n"); g_passes++; }
/* 21: bus error */
TEST(21_bus_error) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); dev_i2c_port_mock_set_error(DEV_ERR_BUS); CHECK_EQ(dev_i2c_write(BUS, ADDR, g_buf, 1U, TO), DEV_ERR_BUS, "bus"); printf("    PASS\n"); g_passes++; }
/* 22: NACK */
TEST(22_nack) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); dev_i2c_port_mock_set_error(DEV_ERR_NO_ACK); CHECK_EQ(dev_i2c_write(BUS, ADDR, g_buf, 1U, TO), DEV_ERR_NO_ACK, "nack"); printf("    PASS\n"); g_passes++; }
/* 23: recover unsupported */
TEST(23_recover) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); CHECK_EQ(dev_i2c_recover_bus(BUS), DEV_ERR_NOT_SUPPORTED, "recov"); printf("    PASS\n"); g_passes++; }
/* 24: unsupported speed */
TEST(24_speed) { CHECK_EQ(dev_i2c_init(), DEV_OK, "init"); CHECK_NEQ(dev_i2c_set_speed(BUS, DEV_I2C_SPEED_HIGH), DEV_OK, "spd"); printf("    PASS\n"); g_passes++; }

int main(void)
{
    printf("=== dev_i2c Test Suite ===\n\n");
    RUN_TEST(1_init_ok); RUN_TEST(2_double_init); RUN_TEST(3_deinit_before);
    RUN_TEST(4_write_before); RUN_TEST(5_read_before); RUN_TEST(6_invalid_bus);
    RUN_TEST(7_invalid_addr); RUN_TEST(8_null_write); RUN_TEST(9_null_read);
    RUN_TEST(10_zero_len); RUN_TEST(11_write_ok); RUN_TEST(12_read_ok);
    RUN_TEST(13_write_read); RUN_TEST(14_mem_write8); RUN_TEST(15_mem_read8);
    RUN_TEST(16_mem_write16); RUN_TEST(17_mem_read16); RUN_TEST(18_probe_ok);
    RUN_TEST(19_probe_nack); RUN_TEST(20_timeout); RUN_TEST(21_bus_error);
    RUN_TEST(22_nack); RUN_TEST(23_recover); RUN_TEST(24_speed);
    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);
    return (g_failures > 0) ? 1 : 0;
}
