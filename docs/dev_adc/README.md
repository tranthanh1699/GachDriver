# dev_adc — Simplified ADC Wrapper

## 1. Overview

Hardware-independent ADC driver using logical channel IDs. Application code never includes vendor HAL headers.

- **Raw read**: returns `dev_adc_raw_t` (0..4095 for 12-bit)
- **mV read**: returns `dev_adc_mv_t` (converted via integer math)
- **Average**: N-sample mean with `uint64_t` accumulator (no overflow)
- **STM32 Cube-managed**: wraps Cube HAL handles, no GPIO/clock/resolution init

## 2. Quick Start

```c
#include "dev_adc.h"

dev_adc_raw_t raw;
dev_adc_mv_t   mv;

dev_adc_init();

dev_adc_read_raw(DEV_ADC_BATTERY_SENSE, &raw);   // raw 0..4095
dev_adc_read_mv(DEV_ADC_BATTERY_SENSE, &mv);      // millivolts
```

## 3. Configuration (`dev_adc_cfg.h`)

```c
#define DEV_ADC_CFG_MAX_CHANNELS            (16U)      // max channels supported
#define DEV_ADC_CFG_AVERAGE_ENABLED         (1U)       // enables average APIs
#define DEV_ADC_CFG_MV_CONVERSION_ENABLED   (1U)       // enables mV APIs
#define DEV_ADC_CFG_MAX_AVERAGE_SAMPLES     (64U)      // max samples per average
#define DEV_ADC_REFERENCE_3300MV            (3300UL)   // reference voltage
#define DEV_ADC_MAX_RAW_12BIT               (4095UL)   // max raw value
#define DEV_ADC_STM32_CUBE_MANAGED_HW_INIT  (1U)       // Cube handles HW init

#define DEV_ADC_BATTERY_SENSE     ((dev_adc_channel_t)0U)
#define DEV_ADC_TEMPERATURE_SENSE ((dev_adc_channel_t)1U)
#define DEV_ADC_POTENTIOMETER     ((dev_adc_channel_t)2U)
```

## 4. API

| Function | Returns |
|----------|---------|
| `init()` / `deinit()` | `DEV_OK`, `ALREADY_INITIALIZED`, `NOT_INITIALIZED` |
| `read_raw(ch, *raw)` | `DEV_OK`, `INVALID_ARG`, `NULL_PTR` |
| `read_mv(ch, *mv)` | `DEV_OK`, `INVALID_ARG`, `NULL_PTR`, `NOT_SUPPORTED` |
| `raw_to_mv(ch, raw, *mv)` | Integer: `(raw * VREF) / MAX_RAW` |
| `read_average_raw(ch, *cfg, *avg)` | Mean of N samples |
| `read_average_mv(ch, *cfg, *avg)` | Mean of N mV readings |
| `calibrate(ch)` | `DEV_ERR_NOT_SUPPORTED` in Cube mode |

## 5. Adding a Channel (STM32 Cube-Managed)

1. Uncomment `#define HAL_ADC_MODULE_ENABLED` in `stm32h7xx_hal_conf.h`
2. In CubeMX, enable ADC1/ADC2 with channels
3. In `dev_adc_cfg.h`, add the channel ID + bump MAX_CHANNELS
4. In `dev_adc_port_stm32.c`, add to `s_adc_map[]`:
```c
[DEV_ADC_NEW_CH] = { DEV_ADC_NEW_CH, &hadc1, ADC_CHANNEL_5, 4095UL, 3300UL },
```

## 6. Build

```cmake
set(DEV_ADC_PORT "stm32" CACHE STRING "ADC port: mock, stm32, esp32")
add_subdirectory(drivers/dev_adc)
target_link_libraries(${PROJECT_NAME} dev_adc)
```
