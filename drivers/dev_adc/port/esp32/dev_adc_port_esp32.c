/*
 * ESP32 ADC port — PLACEHOLDER STUB (planned future work).
 *
 * Returns DEV_ERR_NOT_SUPPORTED for all operations.
 * To complete: include "driver/adc.h", call adc_oneshot_unit_init(),
 * adc_oneshot_config_channel(), adc_oneshot_read().
 * See docs/dev_adc/README.md for the planned API shape.
 */
#include "dev_adc_port_esp32.h"
#include "dev_common.h"

#define DEV_ADC_ESP32_ATTEN_11DB   3
#define DEV_ADC_ESP32_BITWIDTH_12  12

static const dev_adc_hw_channel_t s_map[DEV_ADC_CFG_MAX_CHANNELS] = {
    [DEV_ADC_BATTERY_SENSE]   = { DEV_ADC_BATTERY_SENSE,   1, 6, DEV_ADC_ESP32_ATTEN_11DB, DEV_ADC_ESP32_BITWIDTH_12, DEV_ADC_REFERENCE_3300MV },
    [DEV_ADC_TEMPERATURE_SENSE] = { DEV_ADC_TEMPERATURE_SENSE, 1, 7, DEV_ADC_ESP32_ATTEN_11DB, DEV_ADC_ESP32_BITWIDTH_12, DEV_ADC_REFERENCE_3300MV },
};

bool dev_adc_port_is_channel_valid(dev_adc_channel_t ch) { DEV_UNUSED(ch); return false; }
dev_err_t dev_adc_port_init(void)    { return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_deinit(void)  { return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_read_raw(dev_adc_channel_t c, dev_adc_raw_t *r)
    { DEV_UNUSED(c);DEV_UNUSED(r); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_read_mv(dev_adc_channel_t c, dev_adc_mv_t *m)
    { DEV_UNUSED(c);DEV_UNUSED(m); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_raw_to_mv(dev_adc_channel_t c, dev_adc_raw_t r, dev_adc_mv_t *m)
    { DEV_UNUSED(c);DEV_UNUSED(r);DEV_UNUSED(m); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_calibrate(dev_adc_channel_t c)
    { DEV_UNUSED(c); return DEV_ERR_NOT_SUPPORTED; }
