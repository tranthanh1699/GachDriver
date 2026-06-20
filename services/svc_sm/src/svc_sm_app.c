#include "dev_error.h"
#include "dev_compiler.h"

/* ── Application lifecycle callbacks ──
 * Weak defaults that the application can override with strong symbols. */

/**
 * @brief Weak default application lifecycle implementations.
 *
 * These are provided as weak symbols so that an application can override
 * them by linking its own app_lifecycle.c. The weak defaults return DEV_OK
 * for all callbacks to keep the system running when no application-specific
 * logic is needed.
 */

DEV_WEAK dev_err_t app_init(void)
{
    return DEV_OK;
}

DEV_WEAK dev_err_t app_start(void)
{
    return DEV_OK;
}

DEV_WEAK dev_err_t app_run(void)
{
    return DEV_OK;
}

DEV_WEAK dev_err_t app_stop(void)
{
    return DEV_OK;
}

DEV_WEAK dev_err_t app_shutdown(void)
{
    return DEV_OK;
}

DEV_WEAK dev_err_t app_error(void)
{
    return DEV_OK;
}
