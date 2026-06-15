#include "dev_shell.h"
#include "dev_common.h"
#include <string.h>

#define BS  (0x08U)
#define CR  (0x0DU)
#define LF  (0x0AU)
#define SPC (0x20U)
#define DEL (0x7FU)
#define TAB (0x09U)

typedef struct {
    dev_uart_id_t uart_id;
    char          line[DEV_SHELL_CFG_MAX_LINE_LENGTH];
    uint16_t      line_len;
    bool          initialized;
} dev_shell_state_t;

static dev_shell_state_t g_state;

bool dev_shell_is_initialized(void) { return g_state.initialized; }

dev_err_t dev_shell_init(dev_uart_id_t uart_id)
{
    if (g_state.initialized) return DEV_ERR_ALREADY_INITIALIZED;
    if (uart_id >= DEV_UART_CFG_MAX_INSTANCES) return DEV_ERR_INVALID_ARG;

    g_state.uart_id     = uart_id;
    g_state.line_len    = 0U;
    g_state.initialized = true;

    /* Ensure UART is initialized */
    if (!dev_uart_is_initialized()) {
        dev_err_t ue = dev_uart_init();
        if (ue != DEV_OK) { g_state.initialized = false; return ue; }
    }
    memset(g_state.line, 0, sizeof(g_state.line));

    /* Validate command table */
    if (g_dev_shell_command_count > DEV_SHELL_CFG_MAX_COMMANDS) return DEV_ERR_CONFIG;

    /* Detect duplicates */
    for (uint16_t i = 0U; i < g_dev_shell_command_count; i++) {
        if (!g_dev_shell_commands[i].name || !g_dev_shell_commands[i].function)
            return DEV_ERR_CONFIG;
        for (uint16_t j = i + 1U; j < g_dev_shell_command_count; j++)
            if (strcmp(g_dev_shell_commands[i].name, g_dev_shell_commands[j].name) == 0)
                return DEV_ERR_CONFIG;
    }

#if (DEV_SHELL_CFG_PROMPT_ENABLED == 1U)
    dev_shell_print_prompt();
#endif
    return DEV_OK;
}

dev_err_t dev_shell_deinit(void)
{
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    g_state.initialized = false;
    return DEV_OK;
}

dev_err_t dev_shell_print_prompt(void)
{
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    return dev_shell_write(DEV_SHELL_CFG_DEFAULT_PROMPT " ");
}

dev_err_t dev_shell_write(const char *text)
{
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!text) return DEV_ERR_NULL_PTR;
    size_t len = strlen(text);
    if (len == 0U) return DEV_OK;
    return dev_uart_write(g_state.uart_id, (const uint8_t *)text,
                          (uint16_t)len, DEV_UART_TIMEOUT_DEFAULT_MS);
}

dev_err_t dev_shell_write_line(const char *text)
{
    dev_err_t e = dev_shell_write(text);
    if (e != DEV_OK) return e;
#if (DEV_SHELL_CFG_CRLF_ENABLED == 1U)
    return dev_shell_write("\r\n");
#else
    return dev_shell_write("\n");
#endif
}

dev_err_t dev_shell_write_data(const uint8_t *data, uint16_t length)
{
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!data) return DEV_ERR_NULL_PTR;
    if (length == 0U) return DEV_OK;
    return dev_uart_write(g_state.uart_id, data, length, DEV_UART_TIMEOUT_DEFAULT_MS);
}

dev_err_t dev_shell_find_command(const char *name, const dev_shell_cmd_t **command)
{
    if (!name || !command) return DEV_ERR_NULL_PTR;
    for (uint16_t i = 0U; i < g_dev_shell_command_count; i++) {
        if (strcmp(g_dev_shell_commands[i].name, name) == 0) {
            *command = &g_dev_shell_commands[i];
            return DEV_OK;
        }
    }
    return DEV_ERR_NOT_FOUND;
}

dev_err_t dev_shell_execute_line(char *line)
{
    char    *argv[DEV_SHELL_CFG_MAX_ARGS];
    uint8_t  argc = 0U;
    const dev_shell_cmd_t *cmd = NULL;
    dev_err_t e;

    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!line) return DEV_ERR_NULL_PTR;

    e = dev_shell_parse_line(line, argv, DEV_SHELL_CFG_MAX_ARGS, &argc);
    if (e != DEV_OK) return e;
    if (argc == 0U) return DEV_OK;

    e = dev_shell_find_command(argv[0], &cmd);
    if (e != DEV_OK) {
        dev_shell_write("Unknown command: ");
        dev_shell_write_line(argv[0]);
        return e;
    }

    return cmd->function(argc, argv);
}

dev_err_t dev_shell_handle(void)
{
    uint8_t  byte;
    uint16_t n;
    dev_err_t e;

    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;

    /* Process all available bytes */
    for (;;) {
        e = dev_uart_read(g_state.uart_id, &byte, 1U, &n, DEV_UART_TIMEOUT_NO_WAIT);
        if (e == DEV_ERR_EMPTY || n == 0U) break;
        if (e != DEV_OK) return e;

        /* Handle Enter */
        if (byte == CR || byte == LF) {
#if (DEV_SHELL_CFG_CRLF_ENABLED == 1U)
            dev_shell_write("\r\n");
#else
            dev_shell_write("\n");
#endif
            if (g_state.line_len > 0U) {
                g_state.line[g_state.line_len] = '\0';
                e = dev_shell_execute_line(g_state.line);
                (void)e;
                g_state.line_len = 0U;
            }
#if (DEV_SHELL_CFG_PROMPT_ENABLED == 1U)
            dev_shell_print_prompt();
#endif
            continue;
        }

        /* Handle Backspace */
        if (byte == BS || byte == DEL) {
#if (DEV_SHELL_CFG_BACKSPACE_ENABLED == 1U)
            if (g_state.line_len > 0U) {
                g_state.line_len--;
#if (DEV_SHELL_CFG_ECHO_ENABLED == 1U)
                dev_shell_write("\b \b");
#endif
            }
#endif
            continue;
        }

        /* Skip non-printable */
        if (byte < SPC && byte != TAB) continue;

        /* Store char */
        if (g_state.line_len >= (DEV_SHELL_CFG_MAX_LINE_LENGTH - 1U)) {
            dev_shell_write_line("");
            dev_shell_write_line("Line too long, cleared.");
            g_state.line_len = 0U;
            dev_shell_print_prompt();
            continue;
        }

        g_state.line[g_state.line_len++] = (char)byte;
#if (DEV_SHELL_CFG_ECHO_ENABLED == 1U)
        dev_shell_write_data(&byte, 1U);
#endif
    }
    return DEV_OK;
}
