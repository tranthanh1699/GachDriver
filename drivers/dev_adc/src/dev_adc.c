#include "dev_adc.h"
#include "dev_adc_port.h"
#include "dev_common.h"

static bool g_initialized = false;

static dev_err_t map_err(dev_err_t e) {
    if (e == DEV_OK)                return DEV_OK;
    if (e == DEV_ERR_INVALID_ARG)   return DEV_ERR_INVALID_ARG;
    if (e == DEV_ERR_NULL_PTR)      return DEV_ERR_NULL_PTR;
    if (e == DEV_ERR_NOT_SUPPORTED) return DEV_ERR_NOT_SUPPORTED;
    if (e == DEV_ERR_TIMEOUT)       return DEV_ERR_TIMEOUT;
    if (e == DEV_ERR_BUSY)          return DEV_ERR_BUSY;
    if (e == DEV_ERR_OUT_OF_RANGE)  return DEV_ERR_OUT_OF_RANGE;
    return DEV_ERR_HW_FAILURE;
}

dev_err_t dev_adc_init(void)
{
    if (g_initialized) return DEV_ERR_ALREADY_INITIALIZED;
    g_initialized = true;
    dev_err_t e = dev_adc_port_init();
    if (e != DEV_OK) { g_initialized = false; return map_err(e); }
    return DEV_OK;
}

dev_err_t dev_adc_deinit(void)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    g_initialized = false;
    dev_err_t e = dev_adc_port_deinit();
    return (e != DEV_OK) ? map_err(e) : DEV_OK;
}

bool dev_adc_is_initialized(void) { return g_initialized; }

dev_err_t dev_adc_read_raw(dev_adc_channel_t ch, dev_adc_raw_t *raw)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!raw) return DEV_ERR_NULL_PTR;
    if (!dev_adc_port_is_channel_valid(ch)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_adc_port_read_raw(ch, raw));
}

#if (DEV_ADC_CFG_MV_CONVERSION_ENABLED == DEV_ON)

dev_err_t dev_adc_read_mv(dev_adc_channel_t ch, dev_adc_mv_t *mv)
{
    dev_err_t e;
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!mv) return DEV_ERR_NULL_PTR;
    if (!dev_adc_port_is_channel_valid(ch)) return DEV_ERR_INVALID_ARG;

    /* Prefer port mV; fall back to raw + conversion */
    e = dev_adc_port_read_mv(ch, mv);
    if (e == DEV_ERR_NOT_SUPPORTED) {
        dev_adc_raw_t raw;
        e = dev_adc_port_read_raw(ch, &raw);
        if (e != DEV_OK) return map_err(e);
        return dev_adc_raw_to_mv(ch, raw, mv);
    }
    return map_err(e);
}

dev_err_t dev_adc_raw_to_mv(dev_adc_channel_t ch, dev_adc_raw_t raw, dev_adc_mv_t *mv)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!mv) return DEV_ERR_NULL_PTR;
    if (!dev_adc_port_is_channel_valid(ch)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_adc_port_raw_to_mv(ch, raw, mv));
}

#else

dev_err_t dev_adc_read_mv(dev_adc_channel_t ch, dev_adc_mv_t *mv)
    { DEV_UNUSED(ch); DEV_UNUSED(mv); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_raw_to_mv(dev_adc_channel_t ch, dev_adc_raw_t raw, dev_adc_mv_t *mv)
    { DEV_UNUSED(ch); DEV_UNUSED(raw); DEV_UNUSED(mv); return DEV_ERR_NOT_SUPPORTED; }

#endif

#if (DEV_ADC_CFG_AVERAGE_ENABLED == DEV_ON)

dev_err_t dev_adc_read_average_raw(dev_adc_channel_t ch,
                                   const dev_adc_average_config_t *cfg,
                                   dev_adc_raw_t *raw_avg)
{
    uint64_t sum = 0ULL;
    dev_adc_raw_t sample;
    dev_err_t e;

    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!cfg || !raw_avg) return DEV_ERR_NULL_PTR;
    if (cfg->sample_count == 0U) return DEV_ERR_INVALID_ARG;
    if (cfg->sample_count > DEV_ADC_CFG_MAX_AVERAGE_SAMPLES) return DEV_ERR_OUT_OF_RANGE;
    if (cfg->sample_interval_us != 0U) return DEV_ERR_NOT_SUPPORTED;
    if (!dev_adc_port_is_channel_valid(ch)) return DEV_ERR_INVALID_ARG;

    for (dev_adc_sample_count_t i = 0U; i < cfg->sample_count; i++) {
        e = dev_adc_read_raw(ch, &sample);
        if (e != DEV_OK) return e;
        sum += sample;
    }

    *raw_avg = (dev_adc_raw_t)(sum / (uint64_t)cfg->sample_count);
    return DEV_OK;
}

dev_err_t dev_adc_read_average_mv(dev_adc_channel_t ch,
                                  const dev_adc_average_config_t *cfg,
                                  dev_adc_mv_t *mv_avg)
{
    /* Preferred: average raw first, then convert average raw to mV */
    dev_adc_raw_t raw_avg;
    dev_err_t e = dev_adc_read_average_raw(ch, cfg, &raw_avg);
    if (e != DEV_OK) return e;
    return dev_adc_raw_to_mv(ch, raw_avg, mv_avg);
}

#else

dev_err_t dev_adc_read_average_raw(dev_adc_channel_t ch,
                                   const dev_adc_average_config_t *cfg,
                                   dev_adc_raw_t *raw_avg)
    { DEV_UNUSED(ch); DEV_UNUSED(cfg); DEV_UNUSED(raw_avg); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_read_average_mv(dev_adc_channel_t ch,
                                  const dev_adc_average_config_t *cfg,
                                  dev_adc_mv_t *mv_avg)
    { DEV_UNUSED(ch); DEV_UNUSED(cfg); DEV_UNUSED(mv_avg); return DEV_ERR_NOT_SUPPORTED; }

#endif

dev_err_t dev_adc_calibrate(dev_adc_channel_t ch)
{
    if (!g_initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!dev_adc_port_is_channel_valid(ch)) return DEV_ERR_INVALID_ARG;
    return map_err(dev_adc_port_calibrate(ch));
}
