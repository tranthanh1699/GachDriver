#ifndef DEV_ASSERT_H
#define DEV_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_error.h"
#include "dev_compiler.h"

typedef enum {
    DEV_ASSERT_BACKEND_NONE = 0,
    DEV_ASSERT_BACKEND_UART,
    DEV_ASSERT_BACKEND_TEXT_BUFFER,
    DEV_ASSERT_BACKEND_BREAKPOINT,
    DEV_ASSERT_BACKEND_RESET,
    DEV_ASSERT_BACKEND_USER_HOOK
} dev_assert_backend_t;

typedef enum {
    DEV_ASSERT_TYPE_ASSERT = 0,
    DEV_ASSERT_TYPE_CHECK,
    DEV_ASSERT_TYPE_ERROR
} dev_assert_type_t;

typedef struct {
    const char       *file;
    uint32_t          line;
    dev_assert_type_t type;
    dev_err_t         error;
} dev_assert_info_t;

typedef void (*dev_assert_user_hook_t)(const dev_assert_info_t *info);
typedef void (*dev_assert_output_hook_t)(const char *text);
typedef void (*dev_assert_reset_hook_t)(void);

typedef struct {
    dev_assert_backend_t     backend;
    dev_assert_output_hook_t output_hook;
    dev_assert_user_hook_t   user_hook;
    dev_assert_reset_hook_t  reset_hook;
    char                    *text_buffer;
    uint16_t                 text_buffer_size;
} dev_assert_config_t;

void dev_assert_init(const dev_assert_config_t *config);

void dev_assert_report(const char *file,
                       uint32_t line,
                       dev_assert_type_t type,
                       dev_err_t error);

/* Assert/check macros — do { } while (false), args evaluated once */

#define DEV_CHECK_RET(condition, error_code)                         \
    do {                                                             \
        if (!(condition)) {                                          \
            dev_err_t _dev_check_err = (error_code);                 \
            dev_assert_report(__FILE__, (uint32_t)__LINE__,          \
                              DEV_ASSERT_TYPE_CHECK, _dev_check_err); \
            return _dev_check_err;                                    \
        }                                                             \
    } while (false)

#define DEV_CHECK_PTR_RET(pointer) \
    DEV_CHECK_RET(((pointer) != NULL), DEV_ERR_NULL_PTR)

#define DEV_CHECK_OK_RET(expression)                                 \
    do {                                                             \
        dev_err_t _err = (expression);                               \
        if (_err != DEV_OK) {                                        \
            dev_assert_report(__FILE__, (uint32_t)__LINE__,          \
                              DEV_ASSERT_TYPE_CHECK, _err);          \
            return _err;                                              \
        }                                                             \
    } while (false)

#define DEV_ASSERT(condition)                                        \
    do {                                                             \
        if (!(condition)) {                                          \
            dev_assert_report(__FILE__, (uint32_t)__LINE__,          \
                              DEV_ASSERT_TYPE_ASSERT, DEV_ERR_FAIL); \
            /* dev_assert_report MUST NOT return for ASSERT type.    */ \
            /* Compiler safeguard in case of misconfiguration:       */ \
            for (;;) {}                                              \
        }                                                             \
    } while (false)

#ifdef __cplusplus
}
#endif

#endif /* DEV_ASSERT_H */
