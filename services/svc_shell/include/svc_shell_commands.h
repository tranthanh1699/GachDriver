#ifndef SVC_SHELL_COMMANDS_H
#define SVC_SHELL_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_shell_types.h"

/* Built-in command callbacks */
dev_err_t svc_shell_cmd_help(uint8_t argc, char *argv[]);
dev_err_t svc_shell_cmd_explain(uint8_t argc, char *argv[]);

/* Command table — defined in svc_shell_commands.c */
extern const svc_shell_cmd_t g_svc_shell_commands[];
extern const uint16_t         g_svc_shell_command_count;

#ifdef __cplusplus
}
#endif

#endif /* SVC_SHELL_COMMANDS_H */
