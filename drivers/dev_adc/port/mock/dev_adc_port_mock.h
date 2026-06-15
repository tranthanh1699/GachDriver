#ifndef DEV_ADC_PORT_MOCK_H
#define DEV_ADC_PORT_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_adc_port.h"

void dev_adc_port_mock_reset(void);
void dev_adc_port_mock_set_error(dev_err_t e);
void dev_adc_port_mock_clear_error(void);
void dev_adc_port_mock_set_raw(dev_adc_channel_t ch, dev_adc_raw_t raw);
void dev_adc_port_mock_set_mv(dev_adc_channel_t ch, dev_adc_mv_t mv);

#ifdef __cplusplus
}
#endif

#endif /* DEV_ADC_PORT_MOCK_H */
