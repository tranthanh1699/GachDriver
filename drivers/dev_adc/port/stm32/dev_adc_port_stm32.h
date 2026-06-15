#ifndef DEV_ADC_PORT_STM32_H
#define DEV_ADC_PORT_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_adc_port.h"

/*
 * STM32 Cube-managed ADC port.
 * Wraps Cube-generated ADC handles. No GPIO/clock/resolution init.
 */
#define DEV_ADC_STM32_CUBE_MANAGED_HW_INIT   (1U)
#define DEV_ADC_STM32_POLL_TIMEOUT_MS         (100U)

#ifdef HAL_ADC_MODULE_ENABLED
#include "stm32h7xx_hal.h"

typedef struct {
    dev_adc_channel_t channel_id;
    ADC_HandleTypeDef *handle;
    uint32_t           stm32_channel;
    uint32_t           max_raw;
    uint32_t           reference_mv;
} dev_adc_hw_channel_t;

#endif /* HAL_ADC_MODULE_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* DEV_ADC_PORT_STM32_H */
