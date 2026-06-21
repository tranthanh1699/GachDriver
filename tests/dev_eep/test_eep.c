#include "svc_eep.h"
#include "dev_eep.h"
#include "dev_i2c.h"
#include "dev_i2c_port_mock.h"
#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_passes   = 0;

#define TEST(name)  static void test_##name(void)

#define CHECK(cond, msg) do {                                              \
    if (!(cond)) {                                                         \
        printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);          \
        g_failures++; return;                                              \
    }                                                                      \
} while (false)

#define CHECK_EQ(a, b, msg) do {                                           \
    if ((a) != (b)) {                                                      \
        printf("  FAIL: %s (expected 0x%x, got 0x%x) (%s:%d)\n",          \
               msg, (unsigned)(b), (unsigned)(a), __FILE__, __LINE__);    \
        g_failures++; return;                                              \
    }                                                                      \
} while (false)

#define CHECK_ERR(e, expected, msg) do {                                   \
    if ((e) != (expected)) {                                               \
        printf("  FAIL: %s (expected %d, got %d) (%s:%d)\n",              \
               msg, (int)(expected), (int)(e), __FILE__, __LINE__);       \
        g_failures++; return;                                              \
    }                                                                      \
} while (false)

#define RUN_TEST(name) do { printf("  test_%s...\n", #name); test_##name(); } while (false)

/* ── Helpers ── */

static uint8_t s_eeprom_image[SVC_EEP_MAIN_TOTAL_SIZE];

/* Force clean init: deinit if needed, then fresh setup + init */
static void force_clean_init(void)
{
    if (svc_eep_is_initialized())
    {
        (void)svc_eep_deinit();
    }
    (void)memset(s_eeprom_image, 0xFFU, sizeof(s_eeprom_image));
    dev_i2c_port_mock_reset();
    dev_i2c_port_mock_attach_device(DEV_I2C_BUS_EEPROM,
                                    ((dev_i2c_addr_t)0x50U),
                                    s_eeprom_image,
                                    SVC_EEP_MAIN_TOTAL_SIZE);
    (void)svc_eep_init();
}

/* Force clean deinit */
static void force_clean_deinit(void)
{
    if (svc_eep_is_initialized())
    {
        (void)svc_eep_deinit();
    }
}

/* Write preset data to s_eeprom_image BEFORE mock attach, then init */
static void init_with_preset_block(svc_eep_block_id_t block_id,
                                   const uint8_t *data)
{
    const svc_eep_block_cfg_t *cfg = &s_svc_eep_block_cfg[block_id];

    if (svc_eep_is_initialized())
    {
        (void)svc_eep_deinit();
    }

    /* Write preset into the image buffer BEFORE attaching to mock */
    (void)memset(s_eeprom_image, 0xFFU, sizeof(s_eeprom_image));
    (void)memcpy(&s_eeprom_image[cfg->eep_offset], data, cfg->block_size);

    dev_i2c_port_mock_reset();
    dev_i2c_port_mock_attach_device(DEV_I2C_BUS_EEPROM,
                                    ((dev_i2c_addr_t)0x50U),
                                    s_eeprom_image,
                                    SVC_EEP_MAIN_TOTAL_SIZE);
    (void)svc_eep_init();
}

/* Verify EEPROM content by reading through dev_eep (bypasses mirror) */
static void verify_eeprom(svc_eep_block_id_t block_id, const uint8_t *expected)
{
    const svc_eep_block_cfg_t *cfg = &s_svc_eep_block_cfg[block_id];
    uint8_t temp[64];
    dev_err_t result;

    result = dev_eep_read(DEV_EEP_MAIN, cfg->eep_offset, temp,
                          (uint32_t)cfg->block_size);
    CHECK_ERR(result, DEV_OK, "dev_eep_read for verify");
    CHECK(memcmp(temp, expected, (size_t)cfg->block_size) == 0, "EEPROM data matches");
}

/* ── Test cases ── */

TEST(1_not_init_returns_error)
{
    uint8_t data[32U];
    void *ptr;
    uint16_t len;

    force_clean_deinit();

    CHECK_ERR(svc_eep_load_block(SVC_EEP_BLOCK_SYSTEM_CFG),
              DEV_ERR_NOT_INITIALIZED, "load before init");
    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_SYSTEM_CFG, data, 32U),
              DEV_ERR_NOT_INITIALIZED, "read before init");
    CHECK_ERR(svc_eep_write_direct(SVC_EEP_BLOCK_SYSTEM_CFG, data, 32U),
              DEV_ERR_NOT_INITIALIZED, "write_direct before init");
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, data, 32U),
              DEV_ERR_NOT_INITIALIZED, "write_mirror before init");
    CHECK_ERR(svc_eep_get_mirror_ptr(SVC_EEP_BLOCK_SYSTEM_CFG, &ptr, &len),
              DEV_ERR_NOT_INITIALIZED, "get_mirror_ptr before init");
    CHECK_ERR(svc_eep_sync_block(SVC_EEP_BLOCK_SYSTEM_CFG),
              DEV_ERR_NOT_INITIALIZED, "sync_block before init");
    CHECK_ERR(svc_eep_sync_all(),
              DEV_ERR_NOT_INITIALIZED, "sync_all before init");
    CHECK_ERR(svc_eep_shutdown(),
              DEV_ERR_NOT_INITIALIZED, "shutdown before init");
    CHECK_EQ(svc_eep_is_initialized(), false, "not initialized");
    printf("    PASS\n"); g_passes++;
}

TEST(2_init_success)
{
    force_clean_init();

    CHECK_EQ(svc_eep_is_initialized(), true, "is initialized");

    /* Verify no blocks are loaded after init */
    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_SYSTEM_CFG), false, "system_cfg not loaded");
    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_USER_DATA), false, "user_data not loaded");
    CHECK_EQ(svc_eep_is_dirty(), false, "nothing dirty");

    printf("    PASS\n"); g_passes++;
}

TEST(3_double_init_fails)
{
    force_clean_init();

    CHECK_ERR(svc_eep_init(), DEV_ERR_ALREADY_INITIALIZED, "double init");
    printf("    PASS\n"); g_passes++;
}

TEST(4_shutdown_success)
{
    force_clean_init();

    CHECK_ERR(svc_eep_shutdown(), DEV_OK, "shutdown");
    CHECK_EQ(svc_eep_is_initialized(), false, "not initialized after shutdown");
    printf("    PASS\n"); g_passes++;
}

TEST(5_deinit_clears_state)
{
    force_clean_init();

    CHECK_ERR(svc_eep_deinit(), DEV_OK, "deinit");
    CHECK_EQ(svc_eep_is_initialized(), false, "not initialized");
    printf("    PASS\n"); g_passes++;
}

TEST(6_deinit_before_init_fails)
{
    force_clean_deinit();

    CHECK_ERR(svc_eep_deinit(), DEV_ERR_NOT_INITIALIZED, "deinit before init");
    printf("    PASS\n"); g_passes++;
}

TEST(7_load_block_reads_eeprom)
{
    uint8_t preset[32U];
    uint8_t read_data[32U];

    (void)memset(preset, 0xABU, sizeof(preset));
    init_with_preset_block(SVC_EEP_BLOCK_SYSTEM_CFG, preset);

    CHECK_ERR(svc_eep_load_block(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "load system_cfg");
    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_SYSTEM_CFG), true, "loaded");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), false, "not dirty after load");

    /* Verify loaded data matches the EEPROM preset */
    (void)memset(read_data, 0, sizeof(read_data));
    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_SYSTEM_CFG, read_data, 32U),
              DEV_OK, "read after load");
    CHECK(memcmp(read_data, preset, 32U) == 0, "data matches preset");

    printf("    PASS\n"); g_passes++;
}

TEST(8_read_block_loads_on_demand)
{
    uint8_t preset[64U];
    uint8_t read_data[64U];

    (void)memset(preset, 0xCDU, sizeof(preset));
    init_with_preset_block(SVC_EEP_BLOCK_USER_DATA, preset);

    /* Block should not be loaded yet */
    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_USER_DATA), false, "not loaded initially");

    /* Read should trigger automatic load */
    (void)memset(read_data, 0, sizeof(read_data));
    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_USER_DATA, read_data, 64U),
              DEV_OK, "read triggers load");
    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_USER_DATA), true, "loaded after read");
    CHECK(memcmp(read_data, preset, 64U) == 0, "data matches preset");

    printf("    PASS\n"); g_passes++;
}

TEST(9_write_mirror_ram_only)
{
    uint8_t write_data[32U];
    uint8_t read_data[32U];

    force_clean_init();

    (void)memset(write_data, 0x55U, sizeof(write_data));
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, write_data, 32U),
              DEV_OK, "write_mirror");
    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_SYSTEM_CFG), true, "loaded after mirror write");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), true, "dirty after mirror write");

    /* Read back from mirror */
    (void)memset(read_data, 0, sizeof(read_data));
    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_SYSTEM_CFG, read_data, 32U),
              DEV_OK, "read after mirror write");
    CHECK(memcmp(read_data, write_data, 32U) == 0, "mirror data matches");

    printf("    PASS\n"); g_passes++;
}

TEST(10_write_direct_eeprom_immediately)
{
    uint8_t write_data[16U];
    uint8_t read_data[16U];

    force_clean_init();

    (void)memset(write_data, 0x3CU, sizeof(write_data));
    CHECK_ERR(svc_eep_write_direct(SVC_EEP_BLOCK_DEVICE_INFO, write_data, 16U),
              DEV_OK, "write_direct");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_DEVICE_INFO), false, "not dirty after direct write");
    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_DEVICE_INFO), true, "loaded after direct write");

    /* Read back from mirror */
    (void)memset(read_data, 0, sizeof(read_data));
    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_DEVICE_INFO, read_data, 16U),
              DEV_OK, "read after direct write");
    CHECK(memcmp(read_data, write_data, 16U) == 0, "mirror data matches");

    /* Verify EEPROM was written (through dev_eep, bypassing mirror) */
    verify_eeprom(SVC_EEP_BLOCK_DEVICE_INFO, write_data);

    printf("    PASS\n"); g_passes++;
}

TEST(11_sync_block_writes_dirty_to_eeprom)
{
    uint8_t write_data[32U];

    force_clean_init();

    /* Write to mirror (RAM only) */
    (void)memset(write_data, 0x99U, sizeof(write_data));
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, write_data, 32U),
              DEV_OK, "write_mirror");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), true, "dirty");

    /* Sync — write mirror to EEPROM */
    CHECK_ERR(svc_eep_sync_block(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "sync_block");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), false, "clean after sync");

    /* Verify data was written to EEPROM */
    verify_eeprom(SVC_EEP_BLOCK_SYSTEM_CFG, write_data);

    printf("    PASS\n"); g_passes++;
}

TEST(12_sync_all_writes_only_dirty)
{
    uint8_t data_a[32U];
    uint8_t data_b[64U];
    uint8_t ff_expected[16U];

    force_clean_init();

    /* Write system_cfg mirror (dirty) */
    (void)memset(data_a, 0x11U, sizeof(data_a));
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, data_a, 32U), DEV_OK, "write A");

    /* Write user_data mirror (dirty) */
    (void)memset(data_b, 0x22U, sizeof(data_b));
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_USER_DATA, data_b, 64U), DEV_OK, "write B");

    /* device_info is NOT written — should stay clean */

    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), true, "A dirty");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_USER_DATA), true, "B dirty");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_DEVICE_INFO), false, "C clean");

    CHECK_ERR(svc_eep_sync_all(), DEV_OK, "sync_all");

    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), false, "A clean");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_USER_DATA), false, "B clean");
    CHECK_EQ(svc_eep_is_dirty(), false, "nothing dirty");

    /* Verify dirty blocks were written to EEPROM */
    verify_eeprom(SVC_EEP_BLOCK_SYSTEM_CFG, data_a);
    verify_eeprom(SVC_EEP_BLOCK_USER_DATA, data_b);

    /* device_info was never written — should still be 0xFF from init */
    (void)memset(ff_expected, 0xFFU, sizeof(ff_expected));
    verify_eeprom(SVC_EEP_BLOCK_DEVICE_INFO, ff_expected);

    printf("    PASS\n"); g_passes++;
}

TEST(13_get_mirror_ptr)
{
    uint8_t preset[32U];
    void *ptr;
    uint16_t len;

    (void)memset(preset, 0x77U, sizeof(preset));
    init_with_preset_block(SVC_EEP_BLOCK_SYSTEM_CFG, preset);

    CHECK_ERR(svc_eep_get_mirror_ptr(SVC_EEP_BLOCK_SYSTEM_CFG, &ptr, &len),
              DEV_OK, "get_mirror_ptr");
    CHECK(ptr != NULL, "ptr not null");
    CHECK_EQ(len, 32U, "length matches block size");

    /* Verify content matches what was in EEPROM */
    CHECK(memcmp(ptr, preset, 32U) == 0, "mirror data matches EEPROM");

    printf("    PASS\n"); g_passes++;
}

TEST(14_mark_dirty_after_ptr_access)
{
    uint8_t preset[32U];
    uint8_t expected[32U];
    void *ptr;
    uint16_t len;
    uint8_t *byte_ptr;

    (void)memset(preset, 0x77U, sizeof(preset));
    init_with_preset_block(SVC_EEP_BLOCK_SYSTEM_CFG, preset);

    /* Get pointer to mirror */
    CHECK_ERR(svc_eep_get_mirror_ptr(SVC_EEP_BLOCK_SYSTEM_CFG, &ptr, &len),
              DEV_OK, "get_mirror_ptr");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), false, "not dirty initially");

    /* Modify through pointer */
    byte_ptr = (uint8_t *)ptr;
    byte_ptr[0] = 0xEEU;

    /* Mark dirty */
    CHECK_ERR(svc_eep_mark_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "mark_dirty");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), true, "dirty after mark");

    /* Sync and verify */
    CHECK_ERR(svc_eep_sync_block(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "sync");

    /* Build expected: preset with first byte modified */
    (void)memcpy(expected, preset, 32U);
    expected[0] = 0xEEU;
    verify_eeprom(SVC_EEP_BLOCK_SYSTEM_CFG, expected);

    printf("    PASS\n"); g_passes++;
}

TEST(15_invalid_block_id)
{
    uint8_t data[32U];
    void *ptr;
    uint16_t len;

    force_clean_init();

    CHECK_ERR(svc_eep_load_block(SVC_EEP_BLOCK_COUNT),
              DEV_ERR_INVALID_ARG, "load invalid id");
    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_COUNT, data, 32U),
              DEV_ERR_INVALID_ARG, "read invalid id");
    CHECK_ERR(svc_eep_write_direct(SVC_EEP_BLOCK_COUNT, data, 32U),
              DEV_ERR_INVALID_ARG, "write_direct invalid id");
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_COUNT, data, 32U),
              DEV_ERR_INVALID_ARG, "write_mirror invalid id");
    CHECK_ERR(svc_eep_get_mirror_ptr(SVC_EEP_BLOCK_COUNT, &ptr, &len),
              DEV_ERR_INVALID_ARG, "get_mirror_ptr invalid id");
    CHECK_ERR(svc_eep_mark_dirty(SVC_EEP_BLOCK_COUNT),
              DEV_ERR_INVALID_ARG, "mark_dirty invalid id");
    CHECK_ERR(svc_eep_sync_block(SVC_EEP_BLOCK_COUNT),
              DEV_ERR_INVALID_ARG, "sync_block invalid id");

    printf("    PASS\n"); g_passes++;
}

TEST(16_null_pointer)
{
    void *ptr;
    uint16_t len;

    force_clean_init();

    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_SYSTEM_CFG, NULL, 32U),
              DEV_ERR_NULL_PTR, "read null data");
    CHECK_ERR(svc_eep_write_direct(SVC_EEP_BLOCK_SYSTEM_CFG, NULL, 32U),
              DEV_ERR_NULL_PTR, "write_direct null data");
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, NULL, 32U),
              DEV_ERR_NULL_PTR, "write_mirror null data");
    CHECK_ERR(svc_eep_get_mirror_ptr(SVC_EEP_BLOCK_SYSTEM_CFG, NULL, &len),
              DEV_ERR_NULL_PTR, "get_mirror_ptr null ptr");
    CHECK_ERR(svc_eep_get_mirror_ptr(SVC_EEP_BLOCK_SYSTEM_CFG, &ptr, NULL),
              DEV_ERR_NULL_PTR, "get_mirror_ptr null len");

    printf("    PASS\n"); g_passes++;
}

TEST(17_length_mismatch)
{
    uint8_t data[64U];

    force_clean_init();

    /* SYSTEM_CFG is 32 bytes; test wrong lengths */
    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_SYSTEM_CFG, data, 16U),
              DEV_ERR_INVALID_ARG, "read length too small");
    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_SYSTEM_CFG, data, 64U),
              DEV_ERR_INVALID_ARG, "read length too large");
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, data, 16U),
              DEV_ERR_INVALID_ARG, "write_mirror length mismatch");
    CHECK_ERR(svc_eep_write_direct(SVC_EEP_BLOCK_SYSTEM_CFG, data, 64U),
              DEV_ERR_INVALID_ARG, "write_direct length mismatch");

    printf("    PASS\n"); g_passes++;
}

TEST(18_is_block_loaded)
{
    force_clean_init();

    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_SYSTEM_CFG), false, "not loaded initially");

    CHECK_ERR(svc_eep_load_block(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "load");
    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_SYSTEM_CFG), true, "loaded after load");

    /* Invalid ID returns false */
    CHECK_EQ(svc_eep_is_block_loaded(SVC_EEP_BLOCK_COUNT), false, "invalid id not loaded");

    printf("    PASS\n"); g_passes++;
}

TEST(19_is_block_dirty)
{
    uint8_t data[32U];

    force_clean_init();

    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), false, "not dirty initially");

    (void)memset(data, 0xAAU, sizeof(data));
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, data, 32U), DEV_OK, "write_mirror");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), true, "dirty after mirror write");

    CHECK_ERR(svc_eep_sync_block(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "sync");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), false, "clean after sync");

    printf("    PASS\n"); g_passes++;
}

TEST(20_is_dirty_any_block)
{
    uint8_t data[32U];

    force_clean_init();

    CHECK_EQ(svc_eep_is_dirty(), false, "nothing dirty initially");

    (void)memset(data, 0xBBU, sizeof(data));
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, data, 32U), DEV_OK, "write A");
    CHECK_EQ(svc_eep_is_dirty(), true, "dirty after write");

    CHECK_ERR(svc_eep_sync_block(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "sync A");
    CHECK_EQ(svc_eep_is_dirty(), false, "clean after sync");

    printf("    PASS\n"); g_passes++;
}

TEST(21_mark_dirty_requires_loaded)
{
    force_clean_init();

    /* Block is not loaded — mark_dirty should fail */
    CHECK_ERR(svc_eep_mark_dirty(SVC_EEP_BLOCK_USER_DATA),
              DEV_ERR_INVALID_STATE, "mark_dirty on unloaded block");

    /* Load it, then marking dirty should work */
    CHECK_ERR(svc_eep_load_block(SVC_EEP_BLOCK_USER_DATA), DEV_OK, "load");
    CHECK_ERR(svc_eep_mark_dirty(SVC_EEP_BLOCK_USER_DATA), DEV_OK, "mark_dirty after load");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_USER_DATA), true, "dirty");

    printf("    PASS\n"); g_passes++;
}

TEST(22_sync_not_dirty_returns_ok)
{
    force_clean_init();

    /* Load a block but don't modify it — sync should return OK (no-op) */
    CHECK_ERR(svc_eep_load_block(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "load");
    CHECK_ERR(svc_eep_sync_block(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "sync clean block");

    printf("    PASS\n"); g_passes++;
}

TEST(23_full_lifecycle_write_mirror_then_sync)
{
    uint8_t write_data[32U];
    uint8_t read_data[32U];

    force_clean_init();

    /* 1. Write to mirror (RAM only, dirty) */
    (void)memset(write_data, 0x42U, sizeof(write_data));
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, write_data, 32U),
              DEV_OK, "write_mirror");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), true, "dirty");

    /* 2. Read back from mirror */
    (void)memset(read_data, 0, sizeof(read_data));
    CHECK_ERR(svc_eep_read_block(SVC_EEP_BLOCK_SYSTEM_CFG, read_data, 32U),
              DEV_OK, "read");
    CHECK(memcmp(read_data, write_data, 32U) == 0, "mirror data correct");

    /* 3. Sync to EEPROM */
    CHECK_ERR(svc_eep_sync_block(SVC_EEP_BLOCK_SYSTEM_CFG), DEV_OK, "sync");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), false, "clean");

    /* 4. Verify EEPROM */
    verify_eeprom(SVC_EEP_BLOCK_SYSTEM_CFG, write_data);

    printf("    PASS\n"); g_passes++;
}

TEST(24_shutdown_syncs_dirty_blocks)
{
    uint8_t write_data[32U];

    force_clean_init();

    /* Write to mirror (dirty, not synced) */
    (void)memset(write_data, 0x7EU, sizeof(write_data));
    CHECK_ERR(svc_eep_write_mirror(SVC_EEP_BLOCK_SYSTEM_CFG, write_data, 32U),
              DEV_OK, "write_mirror");
    CHECK_EQ(svc_eep_is_block_dirty(SVC_EEP_BLOCK_SYSTEM_CFG), true, "dirty");

    /* Shutdown should auto-sync and deinit */
    CHECK_ERR(svc_eep_shutdown(), DEV_OK, "shutdown");
    CHECK_EQ(svc_eep_is_initialized(), false, "deinitialized");

    /* Re-init dev_eep to verify EEPROM content (svc_eep_shutdown deinited it) */
    (void)dev_eep_init(DEV_EEP_MAIN);
    verify_eeprom(SVC_EEP_BLOCK_SYSTEM_CFG, write_data);
    (void)dev_eep_deinit(DEV_EEP_MAIN);

    printf("    PASS\n"); g_passes++;
}

int main(void)
{
    printf("=== svc_eep block-based host tests ===\n\n");

    /* Initialize I2C (mock) once for all tests */
    (void)dev_i2c_init();

    RUN_TEST(1_not_init_returns_error);
    RUN_TEST(2_init_success);
    RUN_TEST(3_double_init_fails);
    RUN_TEST(4_shutdown_success);
    RUN_TEST(5_deinit_clears_state);
    RUN_TEST(6_deinit_before_init_fails);
    RUN_TEST(7_load_block_reads_eeprom);
    RUN_TEST(8_read_block_loads_on_demand);
    RUN_TEST(9_write_mirror_ram_only);
    RUN_TEST(10_write_direct_eeprom_immediately);
    RUN_TEST(11_sync_block_writes_dirty_to_eeprom);
    RUN_TEST(12_sync_all_writes_only_dirty);
    RUN_TEST(13_get_mirror_ptr);
    RUN_TEST(14_mark_dirty_after_ptr_access);
    RUN_TEST(15_invalid_block_id);
    RUN_TEST(16_null_pointer);
    RUN_TEST(17_length_mismatch);
    RUN_TEST(18_is_block_loaded);
    RUN_TEST(19_is_block_dirty);
    RUN_TEST(20_is_dirty_any_block);
    RUN_TEST(21_mark_dirty_requires_loaded);
    RUN_TEST(22_sync_not_dirty_returns_ok);
    RUN_TEST(23_full_lifecycle_write_mirror_then_sync);
    RUN_TEST(24_shutdown_syncs_dirty_blocks);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passes, g_failures);

    return (g_failures > 0) ? 1 : 0;
}
