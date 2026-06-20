#include "svc_sm.h"
#include "svc_sm_modules.h"
/* ── Application lifecycle callbacks (defined in app/src/app_lifecycle.c) ── */
extern dev_err_t app_init(void);
extern dev_err_t app_start(void);
extern dev_err_t app_run(void);
extern dev_err_t app_stop(void);
extern dev_err_t app_shutdown(void);
extern dev_err_t app_error(void);
#include "dev_assert.h"

/* ── Private state ── */

static bool            g_svc_sm_initialized  = false;
static svc_sm_state_t  g_svc_sm_state        = SVC_SM_STATE_UNINIT;
static svc_sm_state_t  g_svc_sm_prev_state   = SVC_SM_STATE_UNINIT;
static svc_sm_request_t g_svc_sm_pending_request = SVC_SM_REQUEST_NONE;
static svc_sm_error_info_t g_svc_sm_last_error;
static bool            g_svc_sm_error_stored  = false;
static bool            g_svc_sm_app_error_called = false;

/* ── Forward declarations ── */

static bool svc_sm_is_transition_allowed(svc_sm_state_t from, svc_sm_state_t to);
static dev_err_t svc_sm_set_state(svc_sm_state_t new_state);
static dev_err_t svc_sm_run_modules_init(void);
static dev_err_t svc_sm_run_modules_start(void);
static dev_err_t svc_sm_run_modules_handle(void);
static dev_err_t svc_sm_run_modules_stop(void);
static dev_err_t svc_sm_run_modules_shutdown(void);
static dev_err_t svc_sm_run_modules_deinit(void);
static void svc_sm_store_error(dev_err_t error, const char *module_name);
static dev_err_t svc_sm_execute_shutdown(void);

/* ── State transition validation ── */

static bool svc_sm_is_transition_allowed(svc_sm_state_t from, svc_sm_state_t to)
{
    bool allowed = false;

    switch (from)
    {
    case SVC_SM_STATE_UNINIT:
        allowed = (to == SVC_SM_STATE_STARTUP);
        break;

    case SVC_SM_STATE_STARTUP:
        allowed = (to == SVC_SM_STATE_INIT)
               || (to == SVC_SM_STATE_ERROR);
        break;

    case SVC_SM_STATE_INIT:
        allowed = (to == SVC_SM_STATE_POST_INIT)
               || (to == SVC_SM_STATE_ERROR);
        break;

    case SVC_SM_STATE_POST_INIT:
        allowed = (to == SVC_SM_STATE_RUN)
               || (to == SVC_SM_STATE_ERROR);
        break;

    case SVC_SM_STATE_RUN:
        allowed = (to == SVC_SM_STATE_PREPARE_SHUTDOWN)
               || (to == SVC_SM_STATE_ERROR);
#if (SVC_SM_CFG_SLEEP_ENABLED == 1U)
        allowed = allowed || (to == SVC_SM_STATE_PREPARE_SLEEP);
#endif
        break;

    case SVC_SM_STATE_PREPARE_SHUTDOWN:
        allowed = (to == SVC_SM_STATE_SHUTDOWN)
               || (to == SVC_SM_STATE_ERROR);
        break;

    case SVC_SM_STATE_SHUTDOWN:
        /* terminal state — no further transitions allowed */
        break;

    case SVC_SM_STATE_ERROR:
        allowed = (to == SVC_SM_STATE_PREPARE_SHUTDOWN);
        break;

#if (SVC_SM_CFG_SLEEP_ENABLED == 1U)
    case SVC_SM_STATE_PREPARE_SLEEP:
        allowed = (to == SVC_SM_STATE_SLEEP)
               || (to == SVC_SM_STATE_ERROR);
        break;

    case SVC_SM_STATE_SLEEP:
        allowed = (to == SVC_SM_STATE_WAKEUP)
               || (to == SVC_SM_STATE_ERROR);
        break;

    case SVC_SM_STATE_WAKEUP:
        allowed = (to == SVC_SM_STATE_RUN)
               || (to == SVC_SM_STATE_ERROR);
        break;
#endif

    default:
        break;
    }

    return allowed;
}

/* ── Private state setter ── */

static dev_err_t svc_sm_set_state(svc_sm_state_t new_state)
{
    if (!svc_sm_is_transition_allowed(g_svc_sm_state, new_state))
    {
        return DEV_ERR_INVALID_STATE;
    }

    g_svc_sm_prev_state = g_svc_sm_state;
    g_svc_sm_state = new_state;

    return DEV_OK;
}

/* ── Module iteration helpers ── */

static dev_err_t svc_sm_run_modules_init(void)
{
    uint16_t i;
    dev_err_t ret = DEV_OK;

    for (i = 0U; i < g_svc_sm_module_count; i++)
    {
        const svc_sm_module_t *mod = &g_svc_sm_modules[i];

        if (mod->init == NULL)
        {
            continue;
        }

        ret = mod->init();
        if (ret != DEV_OK)
        {
            if (mod->critical)
            {
                svc_sm_store_error(ret, mod->name);
                (void)svc_sm_set_state(SVC_SM_STATE_ERROR);
                return ret;
            }
            /* non-critical: store but continue */
            svc_sm_store_error(ret, mod->name);
        }
    }

    return DEV_OK;
}

static dev_err_t svc_sm_run_modules_start(void)
{
    uint16_t i;
    dev_err_t ret = DEV_OK;

    for (i = 0U; i < g_svc_sm_module_count; i++)
    {
        const svc_sm_module_t *mod = &g_svc_sm_modules[i];

        if (mod->start == NULL)
        {
            continue;
        }

        ret = mod->start();
        if (ret != DEV_OK)
        {
            if (mod->critical)
            {
                svc_sm_store_error(ret, mod->name);
                (void)svc_sm_set_state(SVC_SM_STATE_ERROR);
                return ret;
            }
            svc_sm_store_error(ret, mod->name);
        }
    }

    return DEV_OK;
}

static dev_err_t svc_sm_run_modules_handle(void)
{
    uint16_t i;
    dev_err_t ret = DEV_OK;

    for (i = 0U; i < g_svc_sm_module_count; i++)
    {
        const svc_sm_module_t *mod = &g_svc_sm_modules[i];

        if (mod->handle == NULL)
        {
            continue;
        }

        ret = mod->handle();
        if (ret != DEV_OK)
        {
            if (mod->critical)
            {
                svc_sm_store_error(ret, mod->name);
                (void)svc_sm_set_state(SVC_SM_STATE_ERROR);
                return ret;
            }
            svc_sm_store_error(ret, mod->name);
        }
    }

    return DEV_OK;
}

static dev_err_t svc_sm_run_modules_stop(void)
{
    uint16_t i;
    dev_err_t ret = DEV_OK;

    for (i = g_svc_sm_module_count; i > 0U; i--)
    {
        const svc_sm_module_t *mod = &g_svc_sm_modules[i - 1U];

        if (mod->stop == NULL)
        {
            continue;
        }

        ret = mod->stop();
        if (ret != DEV_OK)
        {
            svc_sm_store_error(ret, mod->name);
        }
    }

    return ret;
}

static dev_err_t svc_sm_run_modules_shutdown(void)
{
    uint16_t i;
    dev_err_t ret = DEV_OK;

    for (i = g_svc_sm_module_count; i > 0U; i--)
    {
        const svc_sm_module_t *mod = &g_svc_sm_modules[i - 1U];

        if (mod->shutdown == NULL)
        {
            continue;
        }

        ret = mod->shutdown();
        if (ret != DEV_OK)
        {
            svc_sm_store_error(ret, mod->name);
            /* Critical shutdown failure enters ERROR */
            if (mod->critical)
            {
                (void)svc_sm_set_state(SVC_SM_STATE_ERROR);
                return ret;
            }
        }
    }

    return DEV_OK;
}

static dev_err_t svc_sm_run_modules_deinit(void)
{
    uint16_t i;
    dev_err_t ret = DEV_OK;

    for (i = g_svc_sm_module_count; i > 0U; i--)
    {
        const svc_sm_module_t *mod = &g_svc_sm_modules[i - 1U];

        if (mod->deinit == NULL)
        {
            continue;
        }

        ret = mod->deinit();
        if (ret != DEV_OK)
        {
            svc_sm_store_error(ret, mod->name);
        }
    }

    return ret;
}

/* ── Error storage ── */

static void svc_sm_store_error(dev_err_t error, const char *module_name)
{
    g_svc_sm_last_error.error       = error;
    g_svc_sm_last_error.state       = g_svc_sm_state;
    g_svc_sm_last_error.module_name = module_name;
    g_svc_sm_error_stored           = true;
}

/* ── Shutdown execution ── */

static dev_err_t svc_sm_execute_shutdown(void)
{
    dev_err_t ret;

    (void)svc_sm_set_state(SVC_SM_STATE_PREPARE_SHUTDOWN);

    ret = app_shutdown();
    if (ret != DEV_OK)
    {
        svc_sm_store_error(ret, "app");
    }

    (void)svc_sm_run_modules_stop();
    (void)svc_sm_run_modules_shutdown();
    (void)svc_sm_run_modules_deinit();

    return svc_sm_set_state(SVC_SM_STATE_SHUTDOWN);
}

/* ── Public API ── */

dev_err_t svc_sm_init(void)
{
    if (g_svc_sm_initialized)
    {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    g_svc_sm_state             = SVC_SM_STATE_UNINIT;
    g_svc_sm_prev_state        = SVC_SM_STATE_UNINIT;
    g_svc_sm_pending_request   = SVC_SM_REQUEST_NONE;
    g_svc_sm_error_stored      = false;
    g_svc_sm_app_error_called  = false;
    g_svc_sm_initialized       = true;

    return DEV_OK;
}

dev_err_t svc_sm_startup(void)
{
    dev_err_t ret;

    if (!g_svc_sm_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* STARTUP */
    ret = svc_sm_set_state(SVC_SM_STATE_STARTUP);
    if (ret != DEV_OK)
    {
        return ret;
    }

    /* INIT */
    ret = svc_sm_set_state(SVC_SM_STATE_INIT);
    if (ret != DEV_OK)
    {
        return ret;
    }

    ret = svc_sm_run_modules_init();
    if (ret != DEV_OK)
    {
        return ret;
    }

    ret = svc_sm_run_modules_start();
    if (ret != DEV_OK)
    {
        return ret;
    }

    /* POST_INIT */
    ret = svc_sm_set_state(SVC_SM_STATE_POST_INIT);
    if (ret != DEV_OK)
    {
        return ret;
    }

    ret = app_init();
    if (ret != DEV_OK)
    {
        svc_sm_store_error(ret, "app");
        (void)svc_sm_set_state(SVC_SM_STATE_ERROR);
        return ret;
    }

    ret = app_start();
    if (ret != DEV_OK)
    {
        svc_sm_store_error(ret, "app");
        (void)svc_sm_set_state(SVC_SM_STATE_ERROR);
        return ret;
    }

    /* RUN */
    ret = svc_sm_set_state(SVC_SM_STATE_RUN);
    if (ret != DEV_OK)
    {
        return ret;
    }

    return DEV_OK;
}

dev_err_t svc_sm_handle(void)
{
    dev_err_t ret;

    if (!g_svc_sm_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Process pending error request (any state) */
    if (g_svc_sm_pending_request == SVC_SM_REQUEST_ERROR)
    {
        g_svc_sm_pending_request = SVC_SM_REQUEST_NONE;
        (void)svc_sm_set_state(SVC_SM_STATE_ERROR);
    }

    switch (g_svc_sm_state)
    {
    case SVC_SM_STATE_RUN:

        /* Process pending shutdown request */
        if (g_svc_sm_pending_request == SVC_SM_REQUEST_SHUTDOWN)
        {
            g_svc_sm_pending_request = SVC_SM_REQUEST_NONE;
            return svc_sm_execute_shutdown();
        }

#if (SVC_SM_CFG_SLEEP_ENABLED == 1U)
        if (g_svc_sm_pending_request == SVC_SM_REQUEST_SLEEP)
        {
            g_svc_sm_pending_request = SVC_SM_REQUEST_NONE;
            (void)svc_sm_set_state(SVC_SM_STATE_PREPARE_SLEEP);
            ret = svc_sm_set_state(SVC_SM_STATE_SLEEP);
            if (ret != DEV_OK)
            {
                return ret;
            }
        }
#endif

#if (SVC_SM_CFG_CALL_MODULE_HANDLE_IN_RUN == 1U)
        ret = svc_sm_run_modules_handle();
        if (ret != DEV_OK)
        {
            return ret;
        }
#endif

#if (SVC_SM_CFG_CALL_APP_RUN_IN_HANDLE == 1U)
        ret = app_run();
        if (ret != DEV_OK)
        {
            svc_sm_store_error(ret, "app");
            (void)svc_sm_set_state(SVC_SM_STATE_ERROR);
            return ret;
        }
#endif

        break;

    case SVC_SM_STATE_ERROR:

        /* Call app_error() once per error entry */
        if (!g_svc_sm_app_error_called)
        {
            g_svc_sm_app_error_called = true;
            ret = app_error();
            if (ret != DEV_OK)
            {
                return ret;
            }
        }

        /* Allow shutdown from ERROR */
        if (g_svc_sm_pending_request == SVC_SM_REQUEST_SHUTDOWN)
        {
            g_svc_sm_pending_request = SVC_SM_REQUEST_NONE;
            return svc_sm_execute_shutdown();
        }

        break;

#if (SVC_SM_CFG_SLEEP_ENABLED == 1U)
    case SVC_SM_STATE_SLEEP:

        if (g_svc_sm_pending_request == SVC_SM_REQUEST_WAKEUP)
        {
            g_svc_sm_pending_request = SVC_SM_REQUEST_NONE;
            ret = svc_sm_set_state(SVC_SM_STATE_WAKEUP);
            if (ret != DEV_OK)
            {
                return ret;
            }
            ret = svc_sm_set_state(SVC_SM_STATE_RUN);
            if (ret != DEV_OK)
            {
                return ret;
            }
        }
        break;
#endif

    case SVC_SM_STATE_SHUTDOWN:
        /* Terminal — nothing to handle */
        break;

    default:
        break;
    }

    return DEV_OK;
}

dev_err_t svc_sm_shutdown(void)
{
    return svc_sm_execute_shutdown();
}

dev_err_t svc_sm_deinit(void)
{
    g_svc_sm_initialized = false;
    return DEV_OK;
}

/* ── State queries ── */

svc_sm_state_t svc_sm_get_state(void)
{
    return g_svc_sm_state;
}

svc_sm_state_t svc_sm_get_previous_state(void)
{
    return g_svc_sm_prev_state;
}

bool svc_sm_is_initialized(void)
{
    return g_svc_sm_initialized;
}

bool svc_sm_is_running(void)
{
    return (g_svc_sm_state == SVC_SM_STATE_RUN);
}

/* ── Requests ── */

dev_err_t svc_sm_request_shutdown(void)
{
#if (SVC_SM_CFG_SHUTDOWN_ENABLED == 1U)
    if (g_svc_sm_state == SVC_SM_STATE_RUN)
    {
        g_svc_sm_pending_request = SVC_SM_REQUEST_SHUTDOWN;
        return DEV_OK;
    }
#if (SVC_SM_CFG_ERROR_STATE_ENABLED == 1U)
    if (g_svc_sm_state == SVC_SM_STATE_ERROR)
    {
        g_svc_sm_pending_request = SVC_SM_REQUEST_SHUTDOWN;
        return DEV_OK;
    }
#endif
    return DEV_ERR_INVALID_STATE;
#else
    (void)g_svc_sm_state;
    return DEV_ERR_NOT_SUPPORTED;
#endif
}

dev_err_t svc_sm_request_error(dev_err_t reason)
{
#if (SVC_SM_CFG_ERROR_STATE_ENABLED == 1U)
    svc_sm_store_error(reason, NULL);
    g_svc_sm_pending_request  = SVC_SM_REQUEST_ERROR;
    g_svc_sm_app_error_called = false;
    return DEV_OK;
#else
    (void)reason;
    return DEV_ERR_NOT_SUPPORTED;
#endif
}

/* ── Error info ── */

dev_err_t svc_sm_get_last_error(svc_sm_error_info_t *info)
{
    DEV_CHECK_PTR_RET(info);

    if (!g_svc_sm_error_stored)
    {
        info->error       = DEV_OK;
        info->state       = SVC_SM_STATE_UNINIT;
        info->module_name = NULL;
        return DEV_ERR_NOT_FOUND;
    }

    info->error       = g_svc_sm_last_error.error;
    info->state       = g_svc_sm_last_error.state;
    info->module_name = g_svc_sm_last_error.module_name;

    return DEV_OK;
}

#if (SVC_SM_CFG_SLEEP_ENABLED == 1U)

dev_err_t svc_sm_request_sleep(void)
{
    if (g_svc_sm_state != SVC_SM_STATE_RUN)
    {
        return DEV_ERR_INVALID_STATE;
    }
    g_svc_sm_pending_request = SVC_SM_REQUEST_SLEEP;
    return DEV_OK;
}

dev_err_t svc_sm_request_wakeup(void)
{
    if (g_svc_sm_state != SVC_SM_STATE_SLEEP)
    {
        return DEV_ERR_INVALID_STATE;
    }
    g_svc_sm_pending_request = SVC_SM_REQUEST_WAKEUP;
    return DEV_OK;
}

#endif /* SVC_SM_CFG_SLEEP_ENABLED */
