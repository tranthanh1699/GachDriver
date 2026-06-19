#ifndef DEV_SHELL_H
#define DEV_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_shell_types.h"
#include "dev_shell_cfg.h"
#include "dev_shell_parser.h"
#include "dev_uart.h"

dev_err_t dev_shell_init(dev_uart_id_t uart_id);
dev_err_t dev_shell_deinit(void);
dev_err_t dev_shell_handle(void);
dev_err_t dev_shell_print_prompt(void);
dev_err_t dev_shell_write(const char *text);
dev_err_t dev_shell_write_line(const char *text);
dev_err_t dev_shell_write_data(const uint8_t *data, uint16_t length);
dev_err_t dev_shell_execute_line(char *line);
dev_err_t dev_shell_find_command(const char *name, const dev_shell_cmd_t **command);
bool     dev_shell_is_initialized(void);

dev_err_t dev_shell_register_command(const dev_shell_cmd_t *command);
dev_err_t dev_shell_unregister_command(const char *name);
dev_err_t dev_shell_unregister_all_runtime_commands(void);
uint16_t dev_shell_get_runtime_command_count(void);
dev_err_t dev_shell_get_runtime_command(uint16_t index, const dev_shell_cmd_t **command);

#ifdef __cplusplus
}
#endif

#endif /* DEV_SHELL_H */
