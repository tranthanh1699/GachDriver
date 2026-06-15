#include "dev_adc_port_mock.h"
#include "dev_compiler.h"

static dev_adc_raw_t s_raw[DEV_ADC_CFG_MAX_CHANNELS];
static dev_adc_mv_t  s_mv[DEV_ADC_CFG_MAX_CHANNELS];
static bool          s_use_mv[DEV_ADC_CFG_MAX_CHANNELS];
static dev_err_t     s_error = DEV_OK;

void dev_adc_port_mock_reset(void) { s_error = DEV_OK; }
void dev_adc_port_mock_set_error(dev_err_t e) { s_error = e; }
void dev_adc_port_mock_clear_error(void) { s_error = DEV_OK; }
void dev_adc_port_mock_set_raw(dev_adc_channel_t ch, dev_adc_raw_t v)
    { if (ch < DEV_ADC_CFG_MAX_CHANNELS) { s_raw[ch] = v; s_use_mv[ch] = false; } }
void dev_adc_port_mock_set_mv(dev_adc_channel_t ch, dev_adc_mv_t v)
    { if (ch < DEV_ADC_CFG_MAX_CHANNELS) { s_mv[ch] = v; s_use_mv[ch] = true; } }

bool dev_adc_port_is_channel_valid(dev_adc_channel_t ch) { return (ch < DEV_ADC_CFG_MAX_CHANNELS); }

dev_err_t dev_adc_port_init(void)   { return s_error; }
dev_err_t dev_adc_port_deinit(void) { return s_error; }
dev_err_t dev_adc_port_read_raw(dev_adc_channel_t ch, dev_adc_raw_t *raw)
    { if (s_error != DEV_OK) return s_error; if (!dev_adc_port_is_channel_valid(ch)) return DEV_ERR_INVALID_ARG;
      *raw = s_raw[ch]; return DEV_OK; }
dev_err_t dev_adc_port_read_mv(dev_adc_channel_t ch, dev_adc_mv_t *mv)
    { if (s_error != DEV_OK) return s_error; if (!dev_adc_port_is_channel_valid(ch)) return DEV_ERR_INVALID_ARG;
      *mv = s_use_mv[ch] ? s_mv[ch] : (dev_adc_mv_t)(((uint64_t)s_raw[ch] * 3300UL) / 4095UL); return DEV_OK; }
dev_err_t dev_adc_port_raw_to_mv(dev_adc_channel_t ch, dev_adc_raw_t raw, dev_adc_mv_t *mv)
    { if (!dev_adc_port_is_channel_valid(ch) || !mv) return DEV_ERR_INVALID_ARG;
      *mv = (dev_adc_mv_t)(((uint64_t)raw * 3300UL) / 4095UL); return DEV_OK; }
dev_err_t dev_adc_port_calibrate(dev_adc_channel_t ch)
    { DEV_UNUSED(ch); return DEV_ERR_NOT_SUPPORTED; }
