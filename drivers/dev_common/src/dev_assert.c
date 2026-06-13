#include "dev_assert.h"
#include <stdio.h>

#define DEV_ASSERT_MSG_MAX_LEN  128U

static dev_assert_config_t g_dev_assert_config;

static void dev_assert_format_message(const dev_assert_info_t *info, char *buf, uint16_t buf_size)
{
    const char *type_str;

    switch (info->type) {
    case DEV_ASSERT_TYPE_ASSERT:
        type_str = "ASSERT";
        break;
    case DEV_ASSERT_TYPE_CHECK:
        type_str = "CHECK";
        break;
    case DEV_ASSERT_TYPE_ERROR:
        type_str = "ERROR";
        break;
    default:
        type_str = "UNKNOWN";
        break;
    }

    (void)snprintf(buf, buf_size, "[%s] %s:%u: error=%d",
                   type_str, info->file, (unsigned int)info->line, (int)info->error);
}

static void dev_assert_fatal_loop(void)
{
    for (;;) {}
}

void dev_assert_init(const dev_assert_config_t *config)
{
    if (config != NULL) {
        g_dev_assert_config = *config;
    } else {
        g_dev_assert_config.backend     = DEV_ASSERT_BACKEND_NONE;
        g_dev_assert_config.output_hook  = NULL;
        g_dev_assert_config.user_hook    = NULL;
        g_dev_assert_config.reset_hook   = NULL;
        g_dev_assert_config.text_buffer  = NULL;
        g_dev_assert_config.text_buffer_size = 0U;
    }
}

void dev_assert_report(const char *file, uint32_t line,
                       dev_assert_type_t type, dev_err_t error)
{
    dev_assert_info_t info;
    char msg[DEV_ASSERT_MSG_MAX_LEN];

    info.file  = file;
    info.line  = line;
    info.type  = type;
    info.error = error;

    dev_assert_format_message(&info, msg, (uint16_t)DEV_ARRAY_SIZE(msg));

    if (type == DEV_ASSERT_TYPE_ASSERT) {
        /* Fatal — must not return */
        switch (g_dev_assert_config.backend) {
        case DEV_ASSERT_BACKEND_NONE:
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_UART:
            if (g_dev_assert_config.output_hook != NULL) {
                g_dev_assert_config.output_hook(msg);
            }
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_TEXT_BUFFER:
            if ((g_dev_assert_config.text_buffer != NULL) &&
                (g_dev_assert_config.text_buffer_size > 0U)) {
                (void)snprintf(g_dev_assert_config.text_buffer,
                               g_dev_assert_config.text_buffer_size,
                               "%s", msg);
            }
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_BREAKPOINT:
            if (g_dev_assert_config.output_hook != NULL) {
                g_dev_assert_config.output_hook(msg);
            }
            DEV_BREAKPOINT();
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_RESET:
            if (g_dev_assert_config.user_hook != NULL) {
                g_dev_assert_config.user_hook(&info);
            }
            if (g_dev_assert_config.reset_hook != NULL) {
                g_dev_assert_config.reset_hook();
            }
            dev_assert_fatal_loop();
            break;
        case DEV_ASSERT_BACKEND_USER_HOOK:
            if (g_dev_assert_config.user_hook != NULL) {
                g_dev_assert_config.user_hook(&info);
            }
            dev_assert_fatal_loop();
            break;
        default:
            dev_assert_fatal_loop();
            break;
        }
    } else {
        /* Non-fatal — return after reporting */
        switch (g_dev_assert_config.backend) {
        case DEV_ASSERT_BACKEND_NONE:
            break;
        case DEV_ASSERT_BACKEND_UART:
            if (g_dev_assert_config.output_hook != NULL) {
                g_dev_assert_config.output_hook(msg);
            }
            break;
        case DEV_ASSERT_BACKEND_TEXT_BUFFER:
            if ((g_dev_assert_config.text_buffer != NULL) &&
                (g_dev_assert_config.text_buffer_size > 0U)) {
                (void)snprintf(g_dev_assert_config.text_buffer,
                               g_dev_assert_config.text_buffer_size,
                               "%s", msg);
            }
            break;
        case DEV_ASSERT_BACKEND_BREAKPOINT:
            /* No breakpoint for non-fatal checks */
            break;
        case DEV_ASSERT_BACKEND_RESET:
        case DEV_ASSERT_BACKEND_USER_HOOK:
            if (g_dev_assert_config.user_hook != NULL) {
                g_dev_assert_config.user_hook(&info);
            }
            break;
        default:
            break;
        }
    }
}
