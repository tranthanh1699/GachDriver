#include "osal_port_baremetal.h"

#include "stm32h7xx_hal.h"

dev_err_t osal_port_init(void)
{
    return DEV_OK;
}

uint32_t osal_port_get_tick_ms(void)
{
    return HAL_GetTick();
}

void osal_port_delay_ms(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

bool osal_port_is_kernel_running(void)
{
    return false;
}

dev_err_t osal_port_kernel_start(void)
{
    (void)osal_port_kernel_start;
    return DEV_ERR_NOT_SUPPORTED;
}
