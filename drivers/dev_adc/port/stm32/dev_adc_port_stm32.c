#include "dev_adc_port_stm32.h"
#include "dev_compiler.h"

#ifdef HAL_ADC_MODULE_ENABLED

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

static const dev_adc_hw_channel_t s_adc_map[DEV_ADC_CFG_MAX_CHANNELS] = {
    [DEV_ADC_BATTERY_SENSE]   = { DEV_ADC_BATTERY_SENSE,   &hadc1, ADC_CHANNEL_1, 4095UL, 3300UL },
    [DEV_ADC_TEMPERATURE_SENSE] = { DEV_ADC_TEMPERATURE_SENSE, &hadc1, ADC_CHANNEL_2, 4095UL, 3300UL },
    [DEV_ADC_POTENTIOMETER]   = { DEV_ADC_POTENTIOMETER,   &hadc2, ADC_CHANNEL_3, 4095UL, 3300UL },
    [DEV_ADC_CURRENT_SENSE]   = { DEV_ADC_CURRENT_SENSE,   &hadc2, ADC_CHANNEL_4, 4095UL, 3300UL },
};

bool dev_adc_port_is_channel_valid(dev_adc_channel_t ch)
    { return (ch < DEV_ADC_CFG_MAX_CHANNELS) && (s_adc_map[ch].handle != NULL); }

static const dev_adc_hw_channel_t *find_ch(dev_adc_channel_t ch)
    { return (ch < DEV_ADC_CFG_MAX_CHANNELS && s_adc_map[ch].handle) ? &s_adc_map[ch] : NULL; }

static dev_err_t stm32_map(HAL_StatusTypeDef h)
{
    switch (h) {
    case HAL_OK:      return DEV_OK;
    case HAL_TIMEOUT: return DEV_ERR_TIMEOUT;
    case HAL_BUSY:    return DEV_ERR_BUSY;
    case HAL_ERROR:   return DEV_ERR_HW_FAILURE;
    default:          return DEV_ERR_HW_FAILURE;
    }
}

dev_err_t dev_adc_port_init(void)    { return DEV_OK; }
dev_err_t dev_adc_port_deinit(void)  { return DEV_OK; }

dev_err_t dev_adc_port_read_raw(dev_adc_channel_t ch, dev_adc_raw_t *raw)
{
    const dev_adc_hw_channel_t *c = find_ch(ch);
    if (!c) return DEV_ERR_INVALID_ARG;

    HAL_StatusTypeDef hal;
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = c->stm32_channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    HAL_ADC_ConfigChannel(c->handle, &sConfig);

    hal = HAL_ADC_Start(c->handle);
    if (hal != HAL_OK) return stm32_map(hal);

    hal = HAL_ADC_PollForConversion(c->handle, 100U);
    if (hal != HAL_OK) { HAL_ADC_Stop(c->handle); return stm32_map(hal); }

    *raw = (dev_adc_raw_t)HAL_ADC_GetValue(c->handle);
    HAL_ADC_Stop(c->handle);
    return DEV_OK;
}

dev_err_t dev_adc_port_read_mv(dev_adc_channel_t ch, dev_adc_mv_t *mv)
{
    dev_adc_raw_t raw; dev_err_t e = dev_adc_port_read_raw(ch, &raw);
    if (e != DEV_OK) return e;
    return dev_adc_port_raw_to_mv(ch, raw, mv);
}

dev_err_t dev_adc_port_raw_to_mv(dev_adc_channel_t ch, dev_adc_raw_t raw, dev_adc_mv_t *mv)
{
    const dev_adc_hw_channel_t *c = find_ch(ch);
    if (!c || !mv) return DEV_ERR_INVALID_ARG;
    *mv = (dev_adc_mv_t)(((uint64_t)raw * c->reference_mv) / c->max_raw);
    return DEV_OK;
}

dev_err_t dev_adc_port_calibrate(dev_adc_channel_t ch)
{
    const dev_adc_hw_channel_t *c = find_ch(ch);
    if (!c) return DEV_ERR_INVALID_ARG;
    HAL_StatusTypeDef hal = HAL_ADCEx_Calibration_Start(c->handle, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    return stm32_map(hal);
}

#else /* HAL_ADC_MODULE_ENABLED not defined */

bool dev_adc_port_is_channel_valid(dev_adc_channel_t ch) { DEV_UNUSED(ch); return false; }
dev_err_t dev_adc_port_init(void)    { return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_deinit(void)  { return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_read_raw(dev_adc_channel_t ch, dev_adc_raw_t *r)
    { DEV_UNUSED(ch); DEV_UNUSED(r); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_read_mv(dev_adc_channel_t ch, dev_adc_mv_t *m)
    { DEV_UNUSED(ch); DEV_UNUSED(m); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_raw_to_mv(dev_adc_channel_t ch, dev_adc_raw_t r, dev_adc_mv_t *m)
    { DEV_UNUSED(ch); DEV_UNUSED(r); DEV_UNUSED(m); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t dev_adc_port_calibrate(dev_adc_channel_t ch)
    { DEV_UNUSED(ch); return DEV_ERR_NOT_SUPPORTED; }

#endif
