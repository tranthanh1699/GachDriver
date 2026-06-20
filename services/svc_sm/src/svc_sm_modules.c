#include "svc_sm_modules.h"

#include "svc_eep.h"
#include "svc_shell.h"
#include "dev_uart_cfg.h"

/* ── Wrappers for APIs that require arguments ── */

static dev_err_t svc_sm_shell_init_wrapper(void)
{
    return svc_shell_init(DEV_UART_CONSOLE);
}

/* ── Module table ── */

const svc_sm_module_t g_svc_sm_modules[] =
{
    {
        "svc_shell",
        svc_sm_shell_init_wrapper,
        NULL,
        svc_shell_handle,
        NULL,
        NULL,
        svc_shell_deinit,
        false  /* non-critical */
    },
};

const uint16_t g_svc_sm_module_count =
    (uint16_t)(sizeof(g_svc_sm_modules) / sizeof(g_svc_sm_modules[0]));
