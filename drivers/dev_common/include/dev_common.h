#ifndef DEV_COMMON_H
#define DEV_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_error.h"
#include "dev_assert.h"
#include "dev_compiler.h"
#include "dev_version.h"

/**
 * @brief Blocking delay in milliseconds.
 *
 * Default implementation is a weak no-op. The application or board layer
 * shall provide a real implementation (e.g., HAL_Delay, vTaskDelay).
 *
 * @param ms Milliseconds to delay.
 */
void dev_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* DEV_COMMON_H */
