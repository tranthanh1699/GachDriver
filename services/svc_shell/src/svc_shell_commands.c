#include "svc_shell.h"
#include "svc_shell_cfg.h"
#include "svc_shell_commands.h"
#include "svc_shell_parser.h"
#include "dev_common.h"
#include <stdio.h>

/* Private: runtime command getter from svc_shell.c (used by help) */
extern const svc_shell_cmd_t *svc_shell_private_get_runtime_cmd(uint16_t index);

dev_err_t svc_shell_cmd_help(uint8_t argc, char *argv[])
{
    const svc_shell_cmd_t *cmd = NULL;
    char buf[SVC_SHELL_CFG_MAX_OUTPUT_LENGTH];

    if (argc > 1U) {
        dev_err_t e = svc_shell_find_command(argv[1], &cmd);
        if (e != DEV_OK) { svc_shell_write_line("Unknown command"); return e; }
        (void)snprintf(buf, sizeof(buf), "Command: %s\r\nUsage: %s\r\nDescription: %s",
                       cmd->name, cmd->usage, cmd->help);
        svc_shell_write_line(buf);
        return DEV_OK;
    }

    /* Static commands */
    svc_shell_write_line("Built-in commands:");
    for (uint16_t i = 0U; i < g_svc_shell_command_count; i++) {
        (void)snprintf(buf, sizeof(buf), "  %-10s %s",
                       g_svc_shell_commands[i].name, g_svc_shell_commands[i].help);
        svc_shell_write_line(buf);
    }

    /* Runtime commands */
#if (SVC_SHELL_CFG_RUNTIME_COMMAND_ENABLED == DEV_ON)
    {
        uint16_t rt = svc_shell_get_runtime_command_count();
        svc_shell_write_line("");
        if (rt == 0U) {
            svc_shell_write_line("Runtime commands: none");
        } else {
            svc_shell_write_line("Runtime commands:");
            for (uint16_t i = 0U; i < rt; i++) {
                const svc_shell_cmd_t *c;
                c = svc_shell_private_get_runtime_cmd(i);
                (void)snprintf(buf, sizeof(buf), "  %-10s %s", c->name, c->help);
                svc_shell_write_line(buf);
            }
        }
    }
#endif

    return DEV_OK;
}

dev_err_t svc_shell_cmd_explain(uint8_t argc, char *argv[])
{
    (void)argc; (void)argv;
    svc_shell_write_line("svc_shell is a small UART command shell.");
    svc_shell_write_line("Type 'help' to list commands.");
    svc_shell_write_line("Type 'help <command>' to show command usage.");
    svc_shell_write_line("Commands are separated by spaces.");
    svc_shell_write_line("Arguments can be string, int, uint, hex, or bool.");
#if (SVC_SHELL_CFG_RUNTIME_COMMAND_ENABLED == DEV_ON)
    svc_shell_write_line("Commands can be registered at runtime.");
    svc_shell_write_line("Runtime capacity is controlled by SVC_SHELL_CFG_MAX_RUNTIME_COMMANDS.");
#endif
    return DEV_OK;
}

dev_err_t svc_shell_hello(uint8_t argc, char *argv[])
{
    if (argc > 1U) {
        char buf[SVC_SHELL_CFG_MAX_OUTPUT_LENGTH];
        (void)snprintf(buf, sizeof(buf), "Hello, %s!", argv[1]);
        return svc_shell_write_line(buf);
    }
    return svc_shell_write_line("Hello, World!");
}

/*
 * Static command table — add your commands here.
 * Each entry: { name, help, usage, function }
 */
const svc_shell_cmd_t g_svc_shell_commands[] = {
    { "help",    "Show command list or command details", "help [command]", svc_shell_cmd_help },
    { "explain", "Explain shell usage",                  "explain",        svc_shell_cmd_explain },
    { "hello",   "Print Hello, World!",                 "hello",          svc_shell_hello }
};

const uint16_t g_svc_shell_command_count = (uint16_t)(sizeof(g_svc_shell_commands) / sizeof(g_svc_shell_commands[0]));
