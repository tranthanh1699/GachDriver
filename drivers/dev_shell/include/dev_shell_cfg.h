#ifndef DEV_SHELL_CFG_H
#define DEV_SHELL_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_shell_types.h"

#define DEV_SHELL_CFG_MAX_COMMANDS            (32U)
#define DEV_SHELL_CFG_MAX_ARGS                (8U)
#define DEV_SHELL_CFG_MAX_LINE_LENGTH         (128U)
#define DEV_SHELL_CFG_ECHO_ENABLED            (1U)
#define DEV_SHELL_CFG_PROMPT_ENABLED          (1U)
#define DEV_SHELL_CFG_CRLF_ENABLED            (1U)
#define DEV_SHELL_CFG_BACKSPACE_ENABLED       (1U)
#define DEV_SHELL_CFG_QUOTE_PARSE_ENABLED     (1U)
#define DEV_SHELL_CFG_RUNTIME_CHECK_ENABLED   (1U)
#define DEV_SHELL_CFG_DEFAULT_PROMPT          "root:"
#define DEV_SHELL_CFG_MAX_OUTPUT_LENGTH       (256U)

/* Command table — defined in dev_shell_commands.c */
extern const dev_shell_cmd_t g_dev_shell_commands[];
extern const uint16_t         g_dev_shell_command_count;

/* Built-in commands */
dev_err_t dev_shell_cmd_help(uint8_t argc, char *argv[]);
dev_err_t dev_shell_cmd_explain(uint8_t argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* DEV_SHELL_CFG_H */
