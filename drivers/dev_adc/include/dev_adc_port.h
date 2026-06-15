#ifndef DEV_ADC_PORT_H
#define DEV_ADC_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_adc_types.h"
#include "dev_adc_cfg.h"
#include "dev_error.h"

dev_err_t dev_adc_port_init(void);
dev_err_t dev_adc_port_deinit(void);
dev_err_t dev_adc_port_read_raw(dev_adc_channel_t ch, dev_adc_raw_t *raw);
dev_err_t dev_adc_port_read_mv(dev_adc_channel_t ch, dev_adc_mv_t *mv);
dev_err_t dev_adc_port_raw_to_mv(dev_adc_channel_t ch, dev_adc_raw_t raw, dev_adc_mv_t *mv);
dev_err_t dev_adc_port_calibrate(dev_adc_channel_t ch);
bool     dev_adc_port_is_channel_valid(dev_adc_channel_t ch);

#ifdef __cplusplus
}
#endif

#endif /* DEV_ADC_PORT_H */
