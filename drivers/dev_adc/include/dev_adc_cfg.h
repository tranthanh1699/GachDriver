#ifndef DEV_ADC_CFG_H
#define DEV_ADC_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_adc_types.h"

#define DEV_ADC_CFG_MAX_CHANNELS                (16U)
#define DEV_ADC_CFG_RUNTIME_CHECK_ENABLED       DEV_ON
#define DEV_ADC_CFG_AVERAGE_ENABLED             DEV_ON
#define DEV_ADC_CFG_MV_CONVERSION_ENABLED       DEV_ON
#define DEV_ADC_CFG_MAX_AVERAGE_SAMPLES         (64U)
#define DEV_ADC_REFERENCE_3300MV                (3300UL)
#define DEV_ADC_MAX_RAW_12BIT                   (4095UL)
#define DEV_ADC_STM32_CUBE_MANAGED_HW_INIT      DEV_ON

#define DEV_ADC_BATTERY_SENSE                   ((dev_adc_channel_t)0U)
#define DEV_ADC_TEMPERATURE_SENSE               ((dev_adc_channel_t)1U)
#define DEV_ADC_POTENTIOMETER                   ((dev_adc_channel_t)2U)
#define DEV_ADC_CURRENT_SENSE                   ((dev_adc_channel_t)3U)

#ifdef __cplusplus
}
#endif

#endif /* DEV_ADC_CFG_H */
