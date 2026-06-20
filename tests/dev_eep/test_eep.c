#include "svc_eep.h"
#include "dev_i2c.h"
#include "dev_i2c_port_mock.h"
#include "dev_crc.h"
#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)

#define CHECK(cond, msg) do {                                  \
    if (!(cond)) {                                             \
        printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        g_failures++; return;                                   \
    }                                                           \
} while (false)

#define CHECK_EQ(a, b, msg) do {                               \
    if ((a) != (b)) {                                          \
        printf("  FAIL: %s (expected 0x%x, got 0x%x) (%s:%d)\n", \
               msg, (unsigned)(b), (unsigned)(a), __FILE__, __LINE__); \
        g_failures++; return;                                   \
    }                                                           \
} while (false)

#define CHECK_ERR(e, expected, msg) do {                       \
    if ((e) != (expected)) {                                   \
        printf("  FAIL: %s (expected %d, got %d) (%s:%d)\n",   \
               msg, (int)(expected), (int)(e), __FILE__, __LINE__); \
        g_failures++; return;                                   \
    }                                                           \
} while (false)

#define RUN_TEST(name) do { printf("  test_%s...\n", #name); test_##name(); } while (false)

/* ── Helper: build valid EEPROM image with magic + version + CRC ── */

static uint8_t s_eeprom_image[SVC_EEP_MAIN_TOTAL_SIZE];

static void build_valid_eeprom_image(void)
{
    uint32_t magic   = SVC_EEP_MAGIC_VALUE;
    uint16_t version = SVC_EEP_LAYOUT_VERSION;
    uint16_t crc;

    (void)memset(s_eeprom_image, 0xFFU, sizeof(s_eeprom_image));

    /* Write magic */
    (void)memcpy(&s_eeprom_image[SVC_EEP_LAYOUT_MAGIC_OFFSET],
                 &magic, SVC_EEP_LAYOUT_MAGIC_SIZE);

    /* Write version */
    (void)memcpy(&s_eeprom_image[SVC_EEP_LAYOUT_VERSION_OFFSET],
                 &version, SVC_EEP_LAYOUT_VERSION_SIZE);

    /* Compute and write CRC */
    (void)dev_crc16_compute(&s_eeprom_image[SVC_EEP_CRC_START_OFFSET],
                            (size_t)SVC_EEP_CRC_DATA_LENGTH, &crc);
    (void)memcpy(&s_eeprom_image[SVC_EEP_LAYOUT_CRC_OFFSET],
                 &crc, SVC_EEP_LAYOUT_CRC_SIZE);
}

/* ── Helper: attach mock EEPROM device ── */

static void setup_mock_eeprom(void)
{
    build_valid_eeprom_image();
    dev_i2c_port_mock_reset();
    dev_i2c_port_mock_attach_device(DEV_I2C_BUS_EEPROM,
                                    ((dev_i2c_addr_t)0x50U),
                                    s_eeprom_image,
                                    SVC_EEP_MAIN_TOTAL_SIZE);
}

/* ── Test cases ── */

TEST(1_not_init_returns_error)
{
    uint8_t data[4U];
    /* All APIs should return DEV_ERR_NOT_INITIALIZED before init */
    CHECK_ERR(svc_eep_read(SVC_EEP_MAIN, 0U, data, 4U),
              DEV_ERR_NOT_INITIALIZED, "read before init");
    CHECK_ERR(svc_eep_write(SVC_EEP_MAIN, 0U, data, 4U),
              DEV_ERR_NOT_INITIALIZED, "write before init");
    CHECK_ERR(svc_eep_shutdown(),
              DEV_ERR_NOT_INITIALIZED, "shutdown before init");
    CHECK_EQ(svc_eep_is_initialized(), false, "not initialized");
    printf("    PASS\n"); g_passes++;
}

TEST(2_init_success)
{
    if (svc_eep_is_initialized()) { (void)svc_eep_shutdown(); }
    setup_mock_eeprom();

    CHECK_ERR(svc_eep_init(), DEV_OK, "init");
    CHECK_EQ(svc_eep_is_initialized(), true, "is initialized");
    printf("    PASS\n"); g_passes++;
}

TEST(3_double_init_fails)
{
    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    CHECK_ERR(svc_eep_init(), DEV_ERR_ALREADY_INITIALIZED, "double init");
    printf("    PASS\n"); g_passes++;
}

TEST(4_shutdown_success)
{
    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    CHECK_ERR(svc_eep_shutdown(), DEV_OK, "shutdown");
    CHECK_EQ(svc_eep_is_initialized(), false, "not initialized after shutdown");
    printf("    PASS\n"); g_passes++;
}

TEST(5_deinit_clears_state)
{
    setup_mock_eeprom();
    (void)svc_eep_init();

    CHECK_ERR(svc_eep_deinit(), DEV_OK, "deinit");
    CHECK_EQ(svc_eep_is_initialized(), false, "not initialized");
    printf("    PASS\n"); g_passes++;
}

TEST(6_deinit_before_init_fails)
{
    if (svc_eep_is_initialized()) { (void)svc_eep_shutdown(); }

    CHECK_ERR(svc_eep_deinit(), DEV_ERR_NOT_INITIALIZED, "deinit before init");
    printf("    PASS\n"); g_passes++;
}

TEST(7_write_and_read_back)
{
    uint8_t write_data[4U] = { 0xAAU, 0xBBU, 0xCCU, 0xDDU };
    uint8_t read_data[4U]  = { 0U };

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    CHECK_ERR(svc_eep_write(SVC_EEP_MAIN, 0U, write_data, 4U), DEV_OK, "write");
    CHECK_ERR(svc_eep_read(SVC_EEP_MAIN, 0U, read_data, 4U), DEV_OK, "read");

    CHECK_EQ(read_data[0U], 0xAAU, "byte 0");
    CHECK_EQ(read_data[1U], 0xBBU, "byte 1");
    CHECK_EQ(read_data[2U], 0xCCU, "byte 2");
    CHECK_EQ(read_data[3U], 0xDDU, "byte 3");
    printf("    PASS\n"); g_passes++;
}

TEST(8_write_marks_dirty)
{
    uint8_t data[4U] = { 0x11U, 0x22U, 0x33U, 0x44U };

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    (void)svc_eep_clear_dirty(SVC_EEP_MAIN);
    CHECK_EQ(svc_eep_is_dirty(SVC_EEP_MAIN), false, "not dirty initially");

    CHECK_ERR(svc_eep_write(SVC_EEP_MAIN, 16U, data, 4U), DEV_OK, "write");
    CHECK_EQ(svc_eep_is_dirty(SVC_EEP_MAIN), true, "dirty after write");
    printf("    PASS\n"); g_passes++;
}

TEST(9_identical_write_does_not_mark_dirty)
{
    uint8_t data[4U] = { 0xDEU, 0xADU, 0xBEU, 0xEFU };

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    /* Write first time */
    CHECK_ERR(svc_eep_write(SVC_EEP_MAIN, 32U, data, 4U), DEV_OK, "first write");
    (void)svc_eep_clear_dirty(SVC_EEP_MAIN);

    /* Write same data again */
    CHECK_ERR(svc_eep_write(SVC_EEP_MAIN, 32U, data, 4U), DEV_OK, "second write");
    CHECK_EQ(svc_eep_is_dirty(SVC_EEP_MAIN), false, "not dirty after identical write");
    printf("    PASS\n"); g_passes++;
}

TEST(10_field_read_write)
{
    uint32_t boot_count;
    uint32_t read_val;

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    boot_count = 42U;
    CHECK_ERR(svc_eep_write_u32(SVC_EEP_FIELD_BOOT_COUNT, boot_count), DEV_OK, "write u32");

    read_val = 0U;
    CHECK_ERR(svc_eep_read_u32(SVC_EEP_FIELD_BOOT_COUNT, &read_val), DEV_OK, "read u32");
    CHECK_EQ(read_val, 42U, "boot_count == 42");
    printf("    PASS\n"); g_passes++;
}

TEST(11_field_u8_read_write)
{
    uint8_t val;

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    /* Use magic field's first byte for u8 test */
    CHECK_ERR(svc_eep_write_u8(SVC_EEP_FIELD_MAGIC, 0x50U), DEV_OK, "write u8");

    val = 0U;
    CHECK_ERR(svc_eep_read_u8(SVC_EEP_FIELD_MAGIC, &val), DEV_OK, "read u8");
    CHECK_EQ(val, 0x50U, "magic byte == 0x50");
    printf("    PASS\n"); g_passes++;
}

TEST(12_invalid_eep_id)
{
    uint8_t data[4U];

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    CHECK_ERR(svc_eep_read(99U, 0U, data, 4U),
              DEV_ERR_INVALID_ARG, "invalid eep_id for read");
    CHECK_ERR(svc_eep_write(99U, 0U, data, 4U),
              DEV_ERR_INVALID_ARG, "invalid eep_id for write");
    printf("    PASS\n"); g_passes++;
}

TEST(13_invalid_field_id)
{
    uint32_t val;

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    CHECK_ERR(svc_eep_read_u32(99U, &val),
              DEV_ERR_INVALID_ARG, "invalid field_id for read");
    CHECK_ERR(svc_eep_write_u32(99U, 0U),
              DEV_ERR_INVALID_ARG, "invalid field_id for write");
    printf("    PASS\n"); g_passes++;
}

TEST(14_null_pointer)
{
    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    CHECK_ERR(svc_eep_read(SVC_EEP_MAIN, 0U, NULL, 4U),
              DEV_ERR_NULL_PTR, "null data for read");
    CHECK_ERR(svc_eep_write(SVC_EEP_MAIN, 0U, NULL, 4U),
              DEV_ERR_NULL_PTR, "null data for write");
    CHECK_ERR(svc_eep_read_u32(SVC_EEP_FIELD_BOOT_COUNT, NULL),
              DEV_ERR_NULL_PTR, "null value for read_u32");
    printf("    PASS\n"); g_passes++;
}

TEST(15_address_out_of_range)
{
    uint8_t data[4U];

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    CHECK_ERR(svc_eep_read(SVC_EEP_MAIN, SVC_EEP_MAIN_TOTAL_SIZE, data, 1U),
              DEV_ERR_OUT_OF_RANGE, "addr == total_size");
    CHECK_ERR(svc_eep_write(SVC_EEP_MAIN, SVC_EEP_MAIN_TOTAL_SIZE + 1U, data, 1U),
              DEV_ERR_OUT_OF_RANGE, "addr > total_size");
    printf("    PASS\n"); g_passes++;
}

TEST(16_length_out_of_range)
{
    uint8_t data[16U];

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    /* addr + length exceeds total size */
    CHECK_ERR(svc_eep_read(SVC_EEP_MAIN,
                           SVC_EEP_MAIN_TOTAL_SIZE - 4U, data, 8U),
              DEV_ERR_OUT_OF_RANGE, "addr + length > total_size");
    printf("    PASS\n"); g_passes++;
}

TEST(17_zero_length)
{
    uint8_t data[1U];

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    CHECK_ERR(svc_eep_read(SVC_EEP_MAIN, 0U, data, 0U),
              DEV_ERR_INVALID_ARG, "zero-length read");
    printf("    PASS\n"); g_passes++;
}

TEST(18_get_field_info)
{
    const svc_eep_field_t *field = NULL;

    CHECK_ERR(svc_eep_get_field_info(SVC_EEP_FIELD_BOOT_COUNT, &field),
              DEV_OK, "get field info");
    CHECK(field != NULL, "field not null");
    CHECK_EQ(field->field_id, SVC_EEP_FIELD_BOOT_COUNT, "field_id match");
    CHECK_EQ(field->offset, SVC_EEP_LAYOUT_BOOT_COUNT_OFFSET, "offset match");
    CHECK_EQ(field->size, SVC_EEP_LAYOUT_BOOT_COUNT_SIZE, "size match");
    printf("    PASS\n"); g_passes++;
}

TEST(19_get_field_info_null_ptr)
{
    CHECK_ERR(svc_eep_get_field_info(SVC_EEP_FIELD_BOOT_COUNT, NULL),
              DEV_ERR_NULL_PTR, "null ptr");
    printf("    PASS\n"); g_passes++;
}

TEST(20_clear_dirty)
{
    uint8_t data[8U];

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    (void)memset(data, 0xFFU, sizeof(data));
    CHECK_ERR(svc_eep_write(SVC_EEP_MAIN, 64U, data, 8U), DEV_OK, "write");
    CHECK_EQ(svc_eep_is_dirty(SVC_EEP_MAIN), true, "dirty");

    CHECK_ERR(svc_eep_clear_dirty(SVC_EEP_MAIN), DEV_OK, "clear_dirty");
    CHECK_EQ(svc_eep_is_dirty(SVC_EEP_MAIN), false, "not dirty after clear");
    printf("    PASS\n"); g_passes++;
}

TEST(21_dirty_page_count)
{
    uint8_t data[32U];

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    (void)svc_eep_clear_dirty(SVC_EEP_MAIN);

    /* Write across 2 pages (page_size = 16, 32 bytes = 2 pages) */
    (void)memset(data, 0xABU, sizeof(data));
    CHECK_ERR(svc_eep_write(SVC_EEP_MAIN, 80U, data, 32U), DEV_OK, "write 32 bytes");

    CHECK_EQ(svc_eep_get_dirty_page_count(SVC_EEP_MAIN), 2U, "2 dirty pages");
    printf("    PASS\n"); g_passes++;
}

TEST(22_field_length_mismatch)
{
    uint8_t val[8];

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    /* SVC_EEP_FIELD_BOOT_COUNT is 4 bytes, but we pass 5 > 4 */
    CHECK_ERR(svc_eep_read_field(SVC_EEP_FIELD_BOOT_COUNT, val, 5U),
              DEV_ERR_INVALID_ARG, "length mismatch for read_field");
    printf("    PASS\n"); g_passes++;
}

TEST(23_mark_dirty_invalid_eep_id)
{
    CHECK_ERR(svc_eep_mark_dirty(99U, 0U, 4U),
              DEV_ERR_INVALID_ARG, "mark_dirty invalid eep_id");
    printf("    PASS\n"); g_passes++;
}

TEST(24_device_name_field)
{
    const char *name = "GachDriver";
    char read_name[SVC_EEP_LAYOUT_DEVICE_NAME_SIZE];

    if (!svc_eep_is_initialized())
    {
        setup_mock_eeprom();
        (void)svc_eep_init();
    }

    CHECK_ERR(svc_eep_write_field(SVC_EEP_FIELD_DEVICE_NAME,
                                  name, (svc_eep_size_t)strlen(name) + 1U),
              DEV_OK, "write device name");

    (void)memset(read_name, 0, sizeof(read_name));
    CHECK_ERR(svc_eep_read_field(SVC_EEP_FIELD_DEVICE_NAME,
                                 read_name, SVC_EEP_LAYOUT_DEVICE_NAME_SIZE),
              DEV_OK, "read device name");

    CHECK(strcmp(read_name, name) == 0, "device name matches");
    printf("    PASS\n"); g_passes++;
}

int main(void)
{
    printf("=== svc_eep host tests ===\n\n");

    /* Initialize I2C (mock) once for all tests */
    (void)dev_i2c_init();

    RUN_TEST(1_not_init_returns_error);
    RUN_TEST(2_init_success);

    /* Reset state for next group */
    if (svc_eep_is_initialized()) { (void)svc_eep_deinit(); }

    RUN_TEST(3_double_init_fails);

    (void)svc_eep_deinit();

    RUN_TEST(4_shutdown_success);
    RUN_TEST(5_deinit_clears_state);
    RUN_TEST(6_deinit_before_init_fails);
    RUN_TEST(7_write_and_read_back);
    RUN_TEST(8_write_marks_dirty);
    RUN_TEST(9_identical_write_does_not_mark_dirty);
    RUN_TEST(10_field_read_write);
    RUN_TEST(11_field_u8_read_write);
    RUN_TEST(12_invalid_eep_id);
    RUN_TEST(13_invalid_field_id);
    RUN_TEST(14_null_pointer);
    RUN_TEST(15_address_out_of_range);
    RUN_TEST(16_length_out_of_range);
    RUN_TEST(17_zero_length);
    RUN_TEST(18_get_field_info);
    RUN_TEST(19_get_field_info_null_ptr);
    RUN_TEST(20_clear_dirty);
    RUN_TEST(21_dirty_page_count);
    RUN_TEST(22_field_length_mismatch);
    RUN_TEST(23_mark_dirty_invalid_eep_id);
    RUN_TEST(24_device_name_field);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);

    return (g_failures > 0) ? 1 : 0;
}
