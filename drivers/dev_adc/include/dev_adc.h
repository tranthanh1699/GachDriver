#ifndef DEV_ADC_H
#define DEV_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_adc_types.h"
#include "dev_adc_cfg.h"
#include "dev_error.h"

dev_err_t dev_adc_init(void);
dev_err_t dev_adc_deinit(void);
bool     dev_adc_is_initialized(void);

dev_err_t dev_adc_read_raw(dev_adc_channel_t channel, dev_adc_raw_t *raw);
dev_err_t dev_adc_read_mv(dev_adc_channel_t channel, dev_adc_mv_t *mv);

dev_err_t dev_adc_read_average_raw(dev_adc_channel_t channel,
                                   const dev_adc_average_config_t *cfg,
                                   dev_adc_raw_t *raw_avg);
dev_err_t dev_adc_read_average_mv(dev_adc_channel_t channel,
                                  const dev_adc_average_config_t *cfg,
                                  dev_adc_mv_t *mv_avg);

dev_err_t dev_adc_raw_to_mv(dev_adc_channel_t channel, dev_adc_raw_t raw, dev_adc_mv_t *mv);
dev_err_t dev_adc_calibrate(dev_adc_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* DEV_ADC_H */
