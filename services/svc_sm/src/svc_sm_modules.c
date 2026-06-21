#include "svc_sm_modules.h"

#include "svc_eep.h"
#include "svc_shell.h"
#include "app_lifecycle.h"
#include "dev_uart_cfg.h"
#include "dev_i2c.h"
#include "dev_eep.h"

/* ── Wrappers for APIs that require arguments ── */

static dev_err_t svc_sm_shell_init_wrapper(void)
{
    return svc_shell_init(DEV_UART_CONSOLE);
}

/* ── Module table ── */

const svc_sm_module_t g_svc_sm_modules[] =
{
    {
        "dev_i2c",
        dev_i2c_init,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        true  /* critical — if I2C fails, we can't access EEPROM */
    },
    {
        "svc_eep",
        svc_eep_init,
        NULL,
        NULL,
        NULL,
        svc_eep_shutdown,
        svc_eep_deinit,
        NULL,
        false  /* critical — if EEPROM fails, we can't save state */ 
    },
    {
        "svc_shell",
        svc_sm_shell_init_wrapper,
        NULL,
        svc_shell_handle,
        NULL,
        NULL,
        svc_shell_deinit,
        NULL,
        false  /* non-critical */
    },
    {
        "app",
        app_init,        /* init     — POST_INIT: modules are up */
        app_start,       /* start    — POST_INIT: after app_init */
        app_run,         /* handle   — RUN: every superloop iteration */
        app_stop,        /* stop     — reserved */
        app_shutdown,    /* shutdown — save state before modules stop */
        NULL,            /* deinit   — not needed */
        app_error,       /* error_handler — called once in ERROR state */
        true             /* critical — app failure is fatal */
    },

};

const uint16_t g_svc_sm_module_count =
    (uint16_t)(sizeof(g_svc_sm_modules) / sizeof(g_svc_sm_modules[0]));
