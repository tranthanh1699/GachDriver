#include "dev_common.h"

extern void HAL_Delay(uint32_t Delay); 

/* Placeholder for shared helpers. Currently all functionality is
 * provided via headers (types, macros) and dev_assert.c. */

DEV_WEAK void dev_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
    /* Default: no-op. User must provide real implementation. */
}
