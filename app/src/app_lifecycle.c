#include "app_lifecycle.h"
#include "dev_error.h"
#include "svc_shell.h"
#include "svc_eep.h"
#include <stdio.h>
#include <stdlib.h>
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
    dev_err_t err = DEV_OK; 
    if(argc > 2)
    {
        uint8_t blockId = atoi(argv[1]); 
        uint8_t Data = atoi(argv[2]); 
        err = svc_eep_write_direct(blockId, &Data, 1); 
    }
    else
    {
        svc_shell_write_line("Data not correct"); 
    }
    return err;
}

dev_err_t app_read_eep(uint8_t argc, char *argv[])
{
    if(argc > 1)
    {
        uint8_t blockId = atoi(argv[1]); 
        uint8_t Data; 
        svc_eep_read_block(blockId, &Data, 1); 
        uint8_t printfBuffer[125]; 
        sprintf((char *)printfBuffer, "Data is %d", Data); 
        svc_shell_write_line((const char *)printfBuffer); 
    }
    else
    {
        svc_shell_write_line("Data not correct"); 
    }
    return DEV_OK;
}

svc_shell_cmd_t test_write_cmd = {
    .name = "write",
    .help = "Write test data to EEPROM",
    .usage = "Write <field_id> <data>",
    .function = app_write_eep, 
};

svc_shell_cmd_t test_read_cmd = {
    .name = "read",
    .help = "Read test data to EEPROM",
    .usage = "Read <field_id>",
    .function = app_read_eep, 
};


dev_err_t app_init(void)
{
    /* TODO: Add application initialization logic here */
    
    svc_shell_register_command(&test_write_cmd); 
    svc_shell_register_command(&test_read_cmd); 
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
