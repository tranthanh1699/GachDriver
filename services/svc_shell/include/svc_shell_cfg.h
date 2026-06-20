#ifndef SVC_SHELL_CFG_H
#define SVC_SHELL_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_shell_types.h"

#define SVC_SHELL_CFG_MAX_COMMANDS            (32U)
#define SVC_SHELL_CFG_MAX_ARGS                (8U)
#define SVC_SHELL_CFG_MAX_LINE_LENGTH         (128U)
#define SVC_SHELL_CFG_ECHO_ENABLED            DEV_ON
#define SVC_SHELL_CFG_PROMPT_ENABLED          DEV_ON
#define SVC_SHELL_CFG_CRLF_ENABLED            DEV_ON
#define SVC_SHELL_CFG_BACKSPACE_ENABLED       DEV_ON
#define SVC_SHELL_CFG_QUOTE_PARSE_ENABLED     DEV_ON
#define SVC_SHELL_CFG_RUNTIME_CHECK_ENABLED   DEV_ON
#define SVC_SHELL_CFG_RUNTIME_COMMAND_ENABLED DEV_ON
#define SVC_SHELL_CFG_MAX_RUNTIME_COMMANDS    (8U)
#define SVC_SHELL_CFG_DEFAULT_PROMPT          "root:"
#define SVC_SHELL_CFG_MAX_OUTPUT_LENGTH       (256U)

/* Command table — defined in svc_shell_commands.c */
extern const svc_shell_cmd_t g_svc_shell_commands[];
extern const uint16_t         g_svc_shell_command_count;

/* Built-in commands */
dev_err_t svc_shell_cmd_help(uint8_t argc, char *argv[]);
dev_err_t svc_shell_cmd_explain(uint8_t argc, char *argv[]);
dev_err_t svc_shell_hello(uint8_t argc, char *argv[]);
#ifdef __cplusplus
}
#endif

#endif /* SVC_SHELL_CFG_H */
