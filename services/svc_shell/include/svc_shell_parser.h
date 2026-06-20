#ifndef SVC_SHELL_PARSER_H
#define SVC_SHELL_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_error.h"
#include "svc_shell_cfg.h"

dev_err_t svc_shell_parse_line(char *line, char *argv[], uint8_t max_args, uint8_t *argc);
dev_err_t svc_shell_arg_to_i32(const char *arg, int32_t *value);
dev_err_t svc_shell_arg_to_u32(const char *arg, uint32_t *value);
dev_err_t svc_shell_arg_to_u16(const char *arg, uint16_t *value);
dev_err_t svc_shell_arg_to_u8(const char *arg, uint8_t *value);
dev_err_t svc_shell_arg_to_bool(const char *arg, bool *value);
dev_err_t svc_shell_arg_to_hex_u32(const char *arg, uint32_t *value);
bool     svc_shell_arg_is_equal(const char *arg, const char *expected);

#ifdef __cplusplus
}
#endif

#endif /* SVC_SHELL_PARSER_H */
