#include "app_lifecycle.h"
#include "svc_shell.h"
#include "svc_eep.h"
/**
 * @brief Application lifecycle implementation.
 *
 * This file contains the application lifecycle callbacks.
 * Modify these functions to add product-specific behavior.
 *
 * Each function is called by svc_sm at a defined point in the
 * system state machine. See app_lifecycle.h for details.
 */

dev_err_t app_write_eep(uint8_t argc, char *argv[])
{
    svc_shell_write_line("Test Write"); 
    return DEV_OK;
}

svc_shell_cmd_t test_write_cmd = {
    .name = "test_write",
    .help = "Write test data to EEPROM",
    .usage = "test_write <field_id> <value>",
    .function = app_write_eep, 
};


dev_err_t app_init(void)
{
    /* TODO: Add application initialization logic here */
    
    svc_shell_register_command(&test_write_cmd); 
    return DEV_OK;
}

dev_err_t app_start(void)
{
    /* TODO: Add application startup logic here */

    return DEV_OK;
}

dev_err_t app_run(void)
{
    /* TODO: Add periodic application logic here.
     * This is called from the superloop — return quickly.
     */

    return DEV_OK;
}

dev_err_t app_stop(void)
{
    /* TODO: Add pre-shutdown application logic here */

    return DEV_OK;
}

dev_err_t app_shutdown(void)
{
    /* TODO: Add shutdown application logic here */

    return DEV_OK;
}

dev_err_t app_error(void)
{
    /* TODO: Add application-level error handling here */

    return DEV_OK;
}
