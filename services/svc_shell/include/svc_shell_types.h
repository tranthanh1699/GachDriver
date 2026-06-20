#ifndef SVC_SHELL_TYPES_H
#define SVC_SHELL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_error.h"

typedef dev_err_t (*svc_shell_cmd_fn_t)(uint8_t argc, char *argv[]);

typedef struct {
    const char        *name;
    const char        *help;
    const char        *usage;
    svc_shell_cmd_fn_t function;
} svc_shell_cmd_t;

#ifdef __cplusplus
}
#endif

#endif /* SVC_SHELL_TYPES_H */
