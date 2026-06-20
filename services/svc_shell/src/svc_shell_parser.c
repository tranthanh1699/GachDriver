#include "svc_shell_parser.h"
#include "dev_common.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

dev_err_t svc_shell_parse_line(char *line, char *argv[], uint8_t max_args, uint8_t *argc)
{
    uint8_t count = 0U;

    if ((line == NULL) || (argv == NULL) || (argc == NULL)) return DEV_ERR_NULL_PTR;
    if (max_args == 0U) return DEV_ERR_INVALID_ARG;

    while (*line != '\0') {
        /* Skip whitespace */
        while (*line == ' ' || *line == '\t') line++;
        if (*line == '\0') break;
        if (count >= max_args) return DEV_ERR_OVERFLOW;

#if (SVC_SHELL_CFG_QUOTE_PARSE_ENABLED == DEV_ON)
        if (*line == '"') {
            line++;                         /* skip opening quote */
            argv[count++] = line;            /* arg starts after quote */
            while (*line != '\0' && *line != '"') line++;
            if (*line == '\0') return DEV_ERR_PARSE;  /* unterminated quote */
            *line = '\0';                    /* close arg */
            line++;                          /* skip closing quote */
            continue;
        }
#endif

        argv[count++] = line;
        while (*line != '\0' && *line != ' ' && *line != '\t') line++;
        if (*line != '\0') { *line = '\0'; line++; }
    }
    *argc = count;
    return DEV_OK;
}

bool svc_shell_arg_is_equal(const char *arg, const char *expected)
    { return (arg && expected) ? (strcmp(arg, expected) == 0) : false; }

dev_err_t svc_shell_arg_to_i32(const char *arg, int32_t *value)
{
    char *end;
    if (!arg || !value) return DEV_ERR_NULL_PTR;
    if (*arg == '\0') return DEV_ERR_PARSE;
    errno = 0;
    long v = strtol(arg, &end, 10);
    if (*end != '\0' || end == arg) return DEV_ERR_PARSE;
    if (errno == ERANGE || v > INT32_MAX || v < INT32_MIN) return DEV_ERR_OUT_OF_RANGE;
    *value = (int32_t)v;
    return DEV_OK;
}

dev_err_t svc_shell_arg_to_u32(const char *arg, uint32_t *value)
{
    char *end;
    if (!arg || !value) return DEV_ERR_NULL_PTR;
    if (*arg == '\0') return DEV_ERR_PARSE;
    /* Reject leading minus sign for unsigned */
    if (*arg == '-') return DEV_ERR_PARSE;
    errno = 0;
    unsigned long v = strtoul(arg, &end, 10);
    if (*end != '\0' || end == arg) return DEV_ERR_PARSE;
    if (errno == ERANGE || v > UINT32_MAX) return DEV_ERR_OUT_OF_RANGE;
    *value = (uint32_t)v;
    return DEV_OK;
}

dev_err_t svc_shell_arg_to_u16(const char *arg, uint16_t *value)
{
    uint32_t v; dev_err_t e = svc_shell_arg_to_u32(arg, &v);
    if (e != DEV_OK) return e;
    if (v > 0xFFFFU) return DEV_ERR_OUT_OF_RANGE;
    *value = (uint16_t)v; return DEV_OK;
}

dev_err_t svc_shell_arg_to_u8(const char *arg, uint8_t *value)
{
    uint32_t v; dev_err_t e = svc_shell_arg_to_u32(arg, &v);
    if (e != DEV_OK) return e;
    if (v > 0xFFU) return DEV_ERR_OUT_OF_RANGE;
    *value = (uint8_t)v; return DEV_OK;
}

dev_err_t svc_shell_arg_to_bool(const char *arg, bool *value)
{
    if (!arg || !value) return DEV_ERR_NULL_PTR;
    if (svc_shell_arg_is_equal(arg, "true") || svc_shell_arg_is_equal(arg, "1")
     || svc_shell_arg_is_equal(arg, "on")  || svc_shell_arg_is_equal(arg, "yes"))
        { *value = true; return DEV_OK; }
    if (svc_shell_arg_is_equal(arg, "false") || svc_shell_arg_is_equal(arg, "0")
     || svc_shell_arg_is_equal(arg, "off")   || svc_shell_arg_is_equal(arg, "no"))
        { *value = false; return DEV_OK; }
    return DEV_ERR_PARSE;
}

dev_err_t svc_shell_arg_to_hex_u32(const char *arg, uint32_t *value)
{
    char *end;
    if (!arg || !value) return DEV_ERR_NULL_PTR;
    if (*arg == '\0') return DEV_ERR_PARSE;
    errno = 0;
    unsigned long v = strtoul(arg, &end, 16);
    if (*end != '\0' || end == arg) return DEV_ERR_PARSE;
    if (errno == ERANGE || v > UINT32_MAX) return DEV_ERR_OUT_OF_RANGE;
    *value = (uint32_t)v;
    return DEV_OK;
}
