#include "dev_adc.h"
#include "dev_adc_port_mock.h"
#include <stdio.h>

static int g_fail = 0, g_pass = 0;
#define T(n) static void t##n(void)
#define C(c,m) do{if(!(c)){printf("  FAIL: %s (%s:%d)\n",m,__FILE__,__LINE__);g_fail++;return;}}while(0)
#define E(a,b,m) do{int _a=(int)(a),_b=(int)(b);if(_a!=_b){printf("  FAIL: %s x%d g%d (%s:%d)\n",m,_b,_a,__FILE__,__LINE__);g_fail++;return;}}while(0)
#define R(n) do{printf("  t%d...\n",n);dev_adc_port_mock_reset();if(dev_adc_is_initialized())dev_adc_deinit();t##n();}while(0)

#define CH DEV_ADC_BATTERY_SENSE

T(1) { E(dev_adc_init(), DEV_OK, "init"); C(dev_adc_is_initialized(), "?"); printf("  PASS\n"); g_pass++; }
T(2) { E(dev_adc_init(), DEV_OK, "1st"); E(dev_adc_init(), DEV_ERR_ALREADY_INITIALIZED, "2nd"); printf("  PASS\n"); g_pass++; }
T(3) { E(dev_adc_deinit(), DEV_ERR_NOT_INITIALIZED, "deinit"); printf("  PASS\n"); g_pass++; }
T(4) { dev_adc_raw_t r; E(dev_adc_read_raw(CH, &r), DEV_ERR_NOT_INITIALIZED, "raw"); printf("  PASS\n"); g_pass++; }
T(5) { dev_adc_raw_t r; E(dev_adc_init(), DEV_OK, "init"); E(dev_adc_read_raw(99, &r), DEV_ERR_INVALID_ARG, "bad ch"); printf("  PASS\n"); g_pass++; }
T(6) { E(dev_adc_init(), DEV_OK, "init"); E(dev_adc_read_raw(CH, NULL), DEV_ERR_NULL_PTR, "null"); printf("  PASS\n"); g_pass++; }
T(7) { dev_adc_raw_t r; E(dev_adc_init(), DEV_OK, "init"); dev_adc_port_mock_set_raw(CH, 2048); E(dev_adc_read_raw(CH, &r), DEV_OK, "raw"); E(r, 2048, "2048"); printf("  PASS\n"); g_pass++; }
T(8) { dev_adc_mv_t m; E(dev_adc_init(), DEV_OK, "init"); dev_adc_port_mock_set_raw(CH, 0); E(dev_adc_read_mv(CH, &m), DEV_OK, "mv"); E(m, 0, "0mV"); printf("  PASS\n"); g_pass++; }
T(9) { dev_adc_mv_t m; E(dev_adc_init(), DEV_OK, "init"); dev_adc_port_mock_set_raw(CH, 4095); E(dev_adc_raw_to_mv(CH, 4095, &m), DEV_OK, "cnv"); E(m, 3300, "3300mV"); printf("  PASS\n"); g_pass++; }
T(10){ E(dev_adc_init(), DEV_OK, "init"); E(dev_adc_calibrate(CH), DEV_ERR_NOT_SUPPORTED, "cal"); printf("  PASS\n"); g_pass++; }
T(11){ dev_adc_average_config_t c={.sample_count=4U,.sample_interval_us=0U}; dev_adc_raw_t a;
       E(dev_adc_init(), DEV_OK, "init"); dev_adc_port_mock_set_raw(CH, 100);
       E(dev_adc_read_average_raw(CH, &c, &a), DEV_OK, "avg"); E(a, 100, "100"); printf("  PASS\n"); g_pass++; }
T(12){ dev_adc_average_config_t c={.sample_count=2U,.sample_interval_us=0U}; dev_adc_mv_t a;
       E(dev_adc_init(), DEV_OK, "init"); dev_adc_port_mock_set_raw(CH, 1650);
       E(dev_adc_read_average_mv(CH, &c, &a), DEV_OK, "avgmv"); printf("  PASS\n"); g_pass++; }
T(13){ dev_adc_raw_t a;
       E(dev_adc_init(), DEV_OK, "init"); E(dev_adc_read_average_raw(CH, NULL, &a), DEV_ERR_NULL_PTR, "null cfg"); printf("  PASS\n"); g_pass++; }
T(14){ dev_adc_average_config_t c={.sample_count=0U}; dev_adc_raw_t a;
       E(dev_adc_init(), DEV_OK, "init"); E(dev_adc_read_average_raw(CH, &c, &a), DEV_ERR_INVALID_ARG, "zero cnt"); printf("  PASS\n"); g_pass++; }
T(15){ dev_adc_average_config_t c={.sample_count=DEV_ADC_CFG_MAX_AVERAGE_SAMPLES+1U}; dev_adc_raw_t a;
       E(dev_adc_init(), DEV_OK, "init"); E(dev_adc_read_average_raw(CH, &c, &a), DEV_ERR_OUT_OF_RANGE, "big cnt"); printf("  PASS\n"); g_pass++; }
T(16){ E(dev_adc_init(), DEV_OK, "init"); dev_adc_port_mock_set_error(DEV_ERR_HW_FAILURE); dev_adc_raw_t r;
       E(dev_adc_read_raw(CH, &r), DEV_ERR_HW_FAILURE, "inj"); printf("  PASS\n"); g_pass++; }

int main(void) {
    printf("=== dev_adc Test Suite ===\n\n");
    R(1);R(2);R(3);R(4);R(5);R(6);R(7);R(8);R(9);R(10);R(11);R(12);R(13);R(14);R(15);R(16);
    printf("\n=== %d passed, %d failed ===\n", g_pass, g_fail);
    return (g_fail > 0) ? 1 : 0;
}
