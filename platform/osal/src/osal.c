#include "osal.h"
#include "osal_port.h"

static bool g_osal_initialized = false;

dev_err_t osal_init(void)
{
    dev_err_t ret;

    if (g_osal_initialized)
    {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    ret = osal_port_init();
    if (ret == DEV_OK)
    {
        g_osal_initialized = true;
    }

    return ret;
}

bool osal_is_initialized(void)
{
    return g_osal_initialized;
}

uint32_t osal_get_tick_ms(void)
{
    return osal_port_get_tick_ms();
}

void osal_delay_ms(uint32_t delay_ms)
{
    osal_port_delay_ms(delay_ms);
}

bool osal_is_kernel_running(void)
{
    return osal_port_is_kernel_running();
}

dev_err_t osal_kernel_start(void)
{
    return osal_port_kernel_start();
}
