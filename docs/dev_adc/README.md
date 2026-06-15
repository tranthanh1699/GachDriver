# dev_adc — Simplified ADC Wrapper

## 1. Overview

Hardware-independent ADC driver using logical channel IDs. Application code never includes vendor HAL headers.

- **Raw read**: returns `dev_adc_raw_t` (0..4095 for 12-bit)
- **mV read**: port mV preferred; falls back to raw + conversion automatically
- **Average**: N-sample mean, averages raw then converts to mV (one conversion per batch)
- **STM32 Cube-managed**: wraps Cube HAL handles, no GPIO/clock/resolution init

## 2. Quick Start

```c
#include "dev_adc.h"

dev_adc_raw_t raw;
dev_adc_mv_t   mv;

dev_adc_init();
dev_adc_read_raw(DEV_ADC_BATTERY_SENSE, &raw);   // 0..4095
dev_adc_read_mv(DEV_ADC_BATTERY_SENSE, &mv);      // millivolts
```

## 3. Configuration (`dev_adc_cfg.h`)

```c
#define DEV_ADC_CFG_MAX_CHANNELS            (16U)      // max channels supported
#define DEV_ADC_CFG_AVERAGE_ENABLED         (1U)       // 0U = average APIs return NOT_SUPPORTED
#define DEV_ADC_CFG_MV_CONVERSION_ENABLED   (1U)       // 0U = mV APIs return NOT_SUPPORTED
#define DEV_ADC_CFG_MAX_AVERAGE_SAMPLES     (64U)      // max samples per average call
#define DEV_ADC_REFERENCE_3300MV            (3300UL)   // reference voltage for conversion
#define DEV_ADC_MAX_RAW_12BIT               (4095UL)   // max raw value for 12-bit ADC
#define DEV_ADC_STM32_CUBE_MANAGED_HW_INIT  (1U)       // Cube handles HW init

#define DEV_ADC_BATTERY_SENSE     ((dev_adc_channel_t)0U)
#define DEV_ADC_TEMPERATURE_SENSE ((dev_adc_channel_t)1U)
#define DEV_ADC_POTENTIOMETER     ((dev_adc_channel_t)2U)
#define DEV_ADC_CURRENT_SENSE     ((dev_adc_channel_t)3U)
```

| Macro | Meaning | Change when |
|-------|---------|-------------|
| `MAX_CHANNELS` | Total supported channels | Adding many channels (>16) |
| `AVERAGE_ENABLED` | Enable/disable average APIs | Not using averaging (saves code) |
| `MV_CONVERSION_ENABLED` | Enable/disable mV APIs | Only using raw reads (saves code) |
| `MAX_AVERAGE_SAMPLES` | Max samples per call | Need more than 64-sample averages |
| `REFERENCE_3300MV` | ADC reference voltage | Different VREF (e.g., 2500mV) |
| `MAX_RAW_12BIT` | Max raw value | Different resolution (10-bit: 1023) |

## 4. API

```c
dev_err_t dev_adc_init(void);
dev_err_t dev_adc_deinit(void);
bool     dev_adc_is_initialized(void);

dev_err_t dev_adc_read_raw(channel, *raw);
dev_err_t dev_adc_read_mv(channel, *mv);       // port mV preferred; falls back to raw+convert
dev_err_t dev_adc_raw_to_mv(channel, raw, *mv); // (raw * VREF) / MAX_RAW (integer math)

dev_err_t dev_adc_read_average_raw(channel, *cfg, *raw_avg);
dev_err_t dev_adc_read_average_mv(channel, *cfg, *mv_avg);  // averages raw, then converts

dev_err_t dev_adc_calibrate(channel);           // NOT_SUPPORTED in Cube mode
```

### mV fallback

`dev_adc_read_mv()` first tries `dev_adc_port_read_mv()`. If the port returns `DEV_ERR_NOT_SUPPORTED`, it automatically reads raw and converts using `dev_adc_raw_to_mv()`.

### Average behavior

- Rejects `sample_count == 0` → `DEV_ERR_INVALID_ARG`
- Rejects `sample_count > MAX_AVERAGE_SAMPLES` → `DEV_ERR_OUT_OF_RANGE`
- Rejects `sample_interval_us != 0` → `DEV_ERR_NOT_SUPPORTED` (no delay hook yet)
- Accumulates in `uint64_t` — safe up to `64 × 4095 = 262080` (no overflow at 12-bit)
- If one sample fails, stops and returns the error
- `average_mv` averages raw first, then converts once → one conversion per batch

## 5. Usage Examples

### Read raw
```c
dev_adc_raw_t raw;
dev_adc_read_raw(DEV_ADC_BATTERY_SENSE, &raw);
```

### Read mV
```c
dev_adc_mv_t mv;
dev_adc_read_mv(DEV_ADC_BATTERY_SENSE, &mv);
```

### Average 16 samples
```c
dev_adc_average_config_t cfg = { .sample_count = 16U, .sample_interval_us = 0U };
dev_adc_raw_t avg;
dev_adc_read_average_raw(DEV_ADC_BATTERY_SENSE, &cfg, &avg);
```

### Average mV (averages raw, converts once)
```c
dev_adc_average_config_t cfg = { .sample_count = 32U, .sample_interval_us = 0U };
dev_adc_mv_t mv_avg;
dev_adc_read_average_mv(DEV_ADC_BATTERY_SENSE, &cfg, &mv_avg);
```

## 6. Adding a Channel (STM32 Cube-Managed)

1. Uncomment `#define HAL_ADC_MODULE_ENABLED` in `stm32h7xx_hal_conf.h`
2. In CubeMX, enable the ADC with desired channels (configures GPIO, clock, DMA, NVIC)
3. In `dev_adc_cfg.h`, add `#define DEV_ADC_NEW_CH ((dev_adc_channel_t)N)` and bump `MAX_CHANNELS`
4. In `dev_adc_port_stm32.c`, add to `s_adc_map[]`:
```c
[DEV_ADC_NEW_CH] = { DEV_ADC_NEW_CH, &hadc1, ADC_CHANNEL_5, DEV_ADC_MAX_RAW_12BIT, DEV_ADC_REFERENCE_3300MV },
```

## 7. ESP32 Example (Planned)

```c
/* dev_adc_port_esp32.c — planned implementation shape */
static const dev_adc_hw_channel_t s_map[DEV_ADC_CFG_MAX_CHANNELS] = {
    [DEV_ADC_BATTERY_SENSE] = { DEV_ADC_BATTERY_SENSE, 1, 6, 3, 12, 3300UL },
};

dev_err_t dev_adc_port_init(void) {
    /* adc_oneshot_unit_init_cfg() + adc_oneshot_config_channel() for each */
    return DEV_OK;
}
dev_err_t dev_adc_port_read_raw(ch, *raw) {
    /* adc_oneshot_read() → *raw */
    return DEV_OK;
}
```

Current ESP32 port is a placeholder returning `DEV_ERR_NOT_SUPPORTED`.

## 8. Build

```cmake
set(DEV_ADC_PORT "stm32" CACHE STRING "ADC port: mock, stm32, esp32")
add_subdirectory(drivers/dev_adc)
target_link_libraries(${PROJECT_NAME} dev_adc)
```

## 9. Test Plan (16 tests)

| # | Test | Expected |
|---|------|----------|
| 1 | init | `DEV_OK` |
| 2 | double init | `DEV_ERR_ALREADY_INITIALIZED` |
| 3 | deinit before init | `DEV_ERR_NOT_INITIALIZED` |
| 4 | read before init | `DEV_ERR_NOT_INITIALIZED` |
| 5 | invalid channel | `DEV_ERR_INVALID_ARG` |
| 6 | null output pointer | `DEV_ERR_NULL_PTR` |
| 7 | raw read success | 2048 |
| 8 | mV read success | 0mV |
| 9 | raw-to-mV conversion | 4095 → 3300mV |
| 10 | calibration unsupported | `DEV_ERR_NOT_SUPPORTED` |
| 11 | average raw (4 samples) | mean of 4 identical values |
| 12 | average mV (2 samples) | mean then convert |
| 13 | null config pointer | `DEV_ERR_NULL_PTR` |
| 14 | sample count zero | `DEV_ERR_INVALID_ARG` |
| 15 | sample count > max | `DEV_ERR_OUT_OF_RANGE` |
| 16 | error injection | `DEV_ERR_HW_FAILURE` |

16/16 pass against mock port.

## 10. Review Checklist

- [x] Application includes only `dev_adc.h`
- [x] No `boards/board_xxx/` required
- [x] No `g_dev_adc_config` required
- [x] ADC mapping in selected port
- [x] No vendor headers in public headers
- [x] STM32 port wraps `ADC_HandleTypeDef` handles
- [x] STM32 Cube mode: no GPIO/clock/resolution/NVIC init
- [x] mV falls back to raw + conversion
- [x] Average raw → converts once for mV
- [x] `uint64_t` accumulator, no floating point
- [x] Sample interval non-zero rejected
- [x] Named constants, no magic numbers
- [x] 16 host tests pass
