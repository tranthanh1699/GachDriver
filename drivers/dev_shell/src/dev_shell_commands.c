#include "dev_shell.h"
#include "dev_shell_cfg.h"
#include "dev_shell_commands.h"
#include "dev_shell_parser.h"
#include "dev_common.h"
#include <stdio.h>

/* Private: runtime command getter from dev_shell.c (used by help) */
extern const dev_shell_cmd_t *dev_shell_private_get_runtime_cmd(uint16_t index);

dev_err_t dev_shell_cmd_help(uint8_t argc, char *argv[])
{
    const dev_shell_cmd_t *cmd = NULL;
    char buf[DEV_SHELL_CFG_MAX_OUTPUT_LENGTH];

    if (argc > 1U) {
        dev_err_t e = dev_shell_find_command(argv[1], &cmd);
        if (e != DEV_OK) { dev_shell_write_line("Unknown command"); return e; }
        (void)snprintf(buf, sizeof(buf), "Command: %s\r\nUsage: %s\r\nDescription: %s",
                       cmd->name, cmd->usage, cmd->help);
        dev_shell_write_line(buf);
        return DEV_OK;
    }

    /* Static commands */
    dev_shell_write_line("Built-in commands:");
    for (uint16_t i = 0U; i < g_dev_shell_command_count; i++) {
        (void)snprintf(buf, sizeof(buf), "  %-10s %s",
                       g_dev_shell_commands[i].name, g_dev_shell_commands[i].help);
        dev_shell_write_line(buf);
    }

    /* Runtime commands */
#if (DEV_SHELL_CFG_RUNTIME_COMMAND_ENABLED == DEV_ON)
    {
        uint16_t rt = dev_shell_get_runtime_command_count();
        dev_shell_write_line("");
        if (rt == 0U) {
            dev_shell_write_line("Runtime commands: none");
        } else {
            dev_shell_write_line("Runtime commands:");
            for (uint16_t i = 0U; i < rt; i++) {
                const dev_shell_cmd_t *c;
                c = dev_shell_private_get_runtime_cmd(i);
                (void)snprintf(buf, sizeof(buf), "  %-10s %s", c->name, c->help);
                dev_shell_write_line(buf);
            }
        }
    }
#endif

    return DEV_OK;
}

dev_err_t dev_shell_cmd_explain(uint8_t argc, char *argv[])
{
    (void)argc; (void)argv;
    dev_shell_write_line("dev_shell is a small UART command shell.");
    dev_shell_write_line("Type 'help' to list commands.");
    dev_shell_write_line("Type 'help <command>' to show command usage.");
    dev_shell_write_line("Commands are separated by spaces.");
    dev_shell_write_line("Arguments can be string, int, uint, hex, or bool.");
#if (DEV_SHELL_CFG_RUNTIME_COMMAND_ENABLED == DEV_ON)
    dev_shell_write_line("Commands can be registered at runtime.");
    dev_shell_write_line("Runtime capacity is controlled by DEV_SHELL_CFG_MAX_RUNTIME_COMMANDS.");
#endif
    return DEV_OK;
}

dev_err_t dev_shell_hello(uint8_t argc, char *argv[])
{
    if (argc > 1U) {
        char buf[DEV_SHELL_CFG_MAX_OUTPUT_LENGTH];
        (void)snprintf(buf, sizeof(buf), "Hello, %s!", argv[1]);
        return dev_shell_write_line(buf);
    }
    return dev_shell_write_line("Hello, World!");
}

/*
 * Static command table — add your commands here.
 * Each entry: { name, help, usage, function }
 */
const dev_shell_cmd_t g_dev_shell_commands[] = {
    { "help",    "Show command list or command details", "help [command]", dev_shell_cmd_help },
    { "explain", "Explain shell usage",                  "explain",        dev_shell_cmd_explain },
    { "hello",   "Print Hello, World!",                 "hello",          dev_shell_hello }
};

const uint16_t g_dev_shell_command_count = (uint16_t)(sizeof(g_dev_shell_commands) / sizeof(g_dev_shell_commands[0]));
