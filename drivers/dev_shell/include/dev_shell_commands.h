#ifndef DEV_SHELL_COMMANDS_H
#define DEV_SHELL_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_shell_types.h"

/* Built-in command callbacks */
dev_err_t dev_shell_cmd_help(uint8_t argc, char *argv[]);
dev_err_t dev_shell_cmd_explain(uint8_t argc, char *argv[]);

/* Command table — defined in dev_shell_commands.c */
extern const dev_shell_cmd_t g_dev_shell_commands[];
extern const uint16_t         g_dev_shell_command_count;

#ifdef __cplusplus
}
#endif

#endif /* DEV_SHELL_COMMANDS_H */
