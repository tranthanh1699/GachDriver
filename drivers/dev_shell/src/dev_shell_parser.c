#include "dev_shell_parser.h"
#include "dev_common.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

dev_err_t dev_shell_parse_line(char *line, char *argv[], uint8_t max_args, uint8_t *argc)
{
    uint8_t count = 0U;
    bool    in_quote = false;

    if ((line == NULL) || (argv == NULL) || (argc == NULL)) return DEV_ERR_NULL_PTR;
    if (max_args == 0U) return DEV_ERR_INVALID_ARG;

    while (*line != '\0') {
        while (*line == ' ' || *line == '\t') line++;
        if (*line == '\0') break;
        if (count >= max_args) return DEV_ERR_OVERFLOW;

        argv[count++] = line;
        if (*line == '"') { line++; in_quote = true; }

        while (*line != '\0') {
            if (in_quote && *line == '"') { *line = '\0'; line++; in_quote = false; break; }
            if (!in_quote && (*line == ' ' || *line == '\t')) { *line = '\0'; line++; break; }
            line++;
        }
    }
    *argc = count;
    return (count > 0U) ? DEV_OK : DEV_OK;
}

bool dev_shell_arg_is_equal(const char *arg, const char *expected)
    { return (arg && expected) ? (strcmp(arg, expected) == 0) : false; }

dev_err_t dev_shell_arg_to_i32(const char *arg, int32_t *value)
{
    char *end;
    if (!arg || !value) return DEV_ERR_NULL_PTR;
    long v = strtol(arg, &end, 10);
    if (*end != '\0' || end == arg) return DEV_ERR_PARSE;
    *value = (int32_t)v;
    return DEV_OK;
}

dev_err_t dev_shell_arg_to_u32(const char *arg, uint32_t *value)
{
    char *end;
    if (!arg || !value) return DEV_ERR_NULL_PTR;
    unsigned long v = strtoul(arg, &end, 10);
    if (*end != '\0' || end == arg) return DEV_ERR_PARSE;
    *value = (uint32_t)v;
    return DEV_OK;
}

dev_err_t dev_shell_arg_to_u16(const char *arg, uint16_t *value)
{
    uint32_t v; dev_err_t e = dev_shell_arg_to_u32(arg, &v);
    if (e != DEV_OK) return e;
    if (v > 0xFFFFU) return DEV_ERR_OUT_OF_RANGE;
    *value = (uint16_t)v; return DEV_OK;
}

dev_err_t dev_shell_arg_to_u8(const char *arg, uint8_t *value)
{
    uint32_t v; dev_err_t e = dev_shell_arg_to_u32(arg, &v);
    if (e != DEV_OK) return e;
    if (v > 0xFFU) return DEV_ERR_OUT_OF_RANGE;
    *value = (uint8_t)v; return DEV_OK;
}

dev_err_t dev_shell_arg_to_bool(const char *arg, bool *value)
{
    if (!arg || !value) return DEV_ERR_NULL_PTR;
    if (dev_shell_arg_is_equal(arg, "true") || dev_shell_arg_is_equal(arg, "1")
     || dev_shell_arg_is_equal(arg, "on")  || dev_shell_arg_is_equal(arg, "yes"))
        { *value = true; return DEV_OK; }
    if (dev_shell_arg_is_equal(arg, "false") || dev_shell_arg_is_equal(arg, "0")
     || dev_shell_arg_is_equal(arg, "off")   || dev_shell_arg_is_equal(arg, "no"))
        { *value = false; return DEV_OK; }
    return DEV_ERR_PARSE;
}

dev_err_t dev_shell_arg_to_hex_u32(const char *arg, uint32_t *value)
{
    char *end;
    if (!arg || !value) return DEV_ERR_NULL_PTR;
    unsigned long v = strtoul(arg, &end, 16);
    if (*end != '\0' || end == arg) return DEV_ERR_PARSE;
    *value = (uint32_t)v;
    return DEV_OK;
}
