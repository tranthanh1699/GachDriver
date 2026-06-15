#ifndef DEV_ADC_PORT_ESP32_H
#define DEV_ADC_PORT_ESP32_H
#ifdef __cplusplus
extern "C" {
#endif
#include "dev_adc_port.h"
typedef struct { dev_adc_channel_t id; int unit; int ch; int atten; int bw; uint32_t ref_mv; } dev_adc_hw_channel_t;
#ifdef __cplusplus
}
#endif
#endif
