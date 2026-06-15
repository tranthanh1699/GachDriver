#ifndef DEV_ADC_TYPES_H
#define DEV_ADC_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef uint16_t dev_adc_channel_t;
typedef uint16_t dev_adc_raw_t;
typedef uint32_t dev_adc_mv_t;
typedef uint16_t dev_adc_sample_count_t;
typedef uint32_t dev_adc_sample_interval_us_t;

typedef struct {
    dev_adc_sample_count_t      sample_count;
    dev_adc_sample_interval_us_t sample_interval_us;
} dev_adc_average_config_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_ADC_TYPES_H */
