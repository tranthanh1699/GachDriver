#include "svc_shell.h"
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
    char          line[SVC_SHELL_CFG_MAX_LINE_LENGTH];
    uint16_t      line_len;
    bool          initialized;
    bool          last_was_cr;
} svc_shell_state_t;

static svc_shell_state_t g_state;

#if (SVC_SHELL_CFG_RUNTIME_COMMAND_ENABLED == DEV_ON)
static svc_shell_cmd_t s_runtime_commands[SVC_SHELL_CFG_MAX_RUNTIME_COMMANDS];
static uint16_t        s_runtime_command_count;
#endif

bool svc_shell_is_initialized(void) { return g_state.initialized; }

dev_err_t svc_shell_init(dev_uart_id_t uart_id)
{
    if (g_state.initialized) return DEV_ERR_ALREADY_INITIALIZED;
    if (!dev_uart_is_valid(uart_id)) return DEV_ERR_INVALID_ARG;

    g_state.uart_id     = uart_id;
    g_state.line_len    = 0U;
    g_state.initialized = false;
    g_state.last_was_cr = false;
    memset(g_state.line, 0, sizeof(g_state.line));

    if (!dev_uart_is_initialized()) {
        dev_err_t ue = dev_uart_init();
        if (ue != DEV_OK) return ue;
    }

    if (g_svc_shell_command_count > SVC_SHELL_CFG_MAX_COMMANDS) return DEV_ERR_CONFIG;

    for (uint16_t i = 0U; i < g_svc_shell_command_count; i++) {
        const svc_shell_cmd_t *c = &g_svc_shell_commands[i];
        if (!c->name || !c->help || !c->usage || !c->function)
            return DEV_ERR_CONFIG;
        if (strchr(c->name, ' ') != NULL) return DEV_ERR_CONFIG;
        for (uint16_t j = i + 1U; j < g_svc_shell_command_count; j++)
            if (strcmp(c->name, g_svc_shell_commands[j].name) == 0)
                return DEV_ERR_CONFIG;
    }

    g_state.initialized = true;

#if (SVC_SHELL_CFG_RUNTIME_COMMAND_ENABLED == DEV_ON)
    s_runtime_command_count = 0U;
#endif

#if (SVC_SHELL_CFG_PROMPT_ENABLED == DEV_ON)
    (void)svc_shell_print_prompt();
#endif
    return DEV_OK;
}

dev_err_t svc_shell_deinit(void)
{
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    g_state.initialized = false;
    return DEV_OK;
}

dev_err_t svc_shell_print_prompt(void)
{
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    return svc_shell_write(SVC_SHELL_CFG_DEFAULT_PROMPT " ");
}

/*
 * Bounded strlen: scans up to max_len + 1 bytes.
 * Returns true if the string is <= max_len (fits), false if truncated.
 */
static bool svc_shell_strlen_bounded(const char *text, size_t *out_len)
{
    size_t max = SVC_SHELL_CFG_MAX_OUTPUT_LENGTH;
    size_t n   = 0U;
    while (n <= max && text[n] != '\0') { n++; }
    *out_len = (n <= max) ? n : max;
    return (n <= max);
}

dev_err_t svc_shell_write(const char *text)
{
    size_t len;
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!text) return DEV_ERR_NULL_PTR;

    (void)svc_shell_strlen_bounded(text, &len);
    if (len == 0U) return DEV_OK;
    return dev_uart_write(g_state.uart_id, (const uint8_t *)text,
                          (uint16_t)len, DEV_UART_TIMEOUT_DEFAULT_MS);
}

dev_err_t svc_shell_write_line(const char *text)
{
    dev_err_t e = svc_shell_write(text);
    if (e != DEV_OK) return e;
#if (SVC_SHELL_CFG_CRLF_ENABLED == DEV_ON)
    return svc_shell_write("\r\n");
#else
    return svc_shell_write("\n");
#endif
}

dev_err_t svc_shell_write_data(const uint8_t *data, uint16_t length)
{
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!data) return DEV_ERR_NULL_PTR;
    if (length == 0U) return DEV_OK;
    return dev_uart_write(g_state.uart_id, data, length, DEV_UART_TIMEOUT_DEFAULT_MS);
}

dev_err_t svc_shell_find_command(const char *name, const svc_shell_cmd_t **command)
{
    if (!name || !command) return DEV_ERR_NULL_PTR;
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;

    /* Search static table first */
    for (uint16_t i = 0U; i < g_svc_shell_command_count; i++) {
        if (strcmp(g_svc_shell_commands[i].name, name) == 0) {
            *command = &g_svc_shell_commands[i];
            return DEV_OK;
        }
    }

    /* Search runtime table second */
#if (SVC_SHELL_CFG_RUNTIME_COMMAND_ENABLED == DEV_ON)
    for (uint16_t i = 0U; i < s_runtime_command_count; i++) {
        if (strcmp(s_runtime_commands[i].name, name) == 0) {
            *command = &s_runtime_commands[i];
            return DEV_OK;
        }
    }
#endif

    return DEV_ERR_NOT_FOUND;
}

/* ── Runtime command registration ── */

#if (SVC_SHELL_CFG_RUNTIME_COMMAND_ENABLED == DEV_ON)

static bool svc_shell_is_valid_cmd_name(const char *name)
{
    size_t i;
    if (!name || name[0] == '\0') return false;
    for (i = 0U; name[i] != '\0'; i++) {
        unsigned char c = (unsigned char)name[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return false;
    }
    return true;
}

static bool svc_shell_cmd_exists_static(const char *name)
{
    for (uint16_t i = 0U; i < g_svc_shell_command_count; i++)
        if (strcmp(g_svc_shell_commands[i].name, name) == 0) return true;
    return false;
}

static bool svc_shell_cmd_exists_runtime(const char *name)
{
    for (uint16_t i = 0U; i < s_runtime_command_count; i++)
        if (strcmp(s_runtime_commands[i].name, name) == 0) return true;
    return false;
}

dev_err_t svc_shell_register_command(const svc_shell_cmd_t *cmd)
{
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!cmd) return DEV_ERR_NULL_PTR;
    if (!cmd->name) return DEV_ERR_NULL_PTR;
    if (!cmd->help || !cmd->usage || !cmd->function) return DEV_ERR_NULL_PTR;
    if (!svc_shell_is_valid_cmd_name(cmd->name)) return DEV_ERR_INVALID_ARG;

    if (svc_shell_cmd_exists_static(cmd->name))  return DEV_ERR_CONFIG;
    if (svc_shell_cmd_exists_runtime(cmd->name)) return DEV_ERR_CONFIG;

    if (s_runtime_command_count >= SVC_SHELL_CFG_MAX_RUNTIME_COMMANDS)
        return DEV_ERR_OVERFLOW;

    s_runtime_commands[s_runtime_command_count] = *cmd;
    s_runtime_command_count++;
    return DEV_OK;
}

dev_err_t svc_shell_unregister_command(const char *name)
{
    uint16_t i;
    bool found = false;

    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!name) return DEV_ERR_NULL_PTR;

    if (svc_shell_cmd_exists_static(name)) return DEV_ERR_CONFIG;

    for (i = 0U; i < s_runtime_command_count; i++) {
        if (strcmp(s_runtime_commands[i].name, name) == 0) { found = true; break; }
    }
    if (!found) return DEV_ERR_NOT_FOUND;

    /* Compact: shift remaining entries left, clear stale slot */
    for (; i < s_runtime_command_count - 1U; i++)
        s_runtime_commands[i] = s_runtime_commands[i + 1U];
    memset(&s_runtime_commands[s_runtime_command_count - 1U], 0,
           sizeof(s_runtime_commands[0]));
    s_runtime_command_count--;
    return DEV_OK;
}

dev_err_t svc_shell_unregister_all_runtime_commands(void)
{
    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    s_runtime_command_count = 0U;
    return DEV_OK;
}

uint16_t svc_shell_get_runtime_command_count(void) { return s_runtime_command_count; }

/* Private: used by help command to enumerate runtime entries */
const svc_shell_cmd_t *svc_shell_private_get_runtime_cmd(uint16_t index)
    { return (index < s_runtime_command_count) ? &s_runtime_commands[index] : NULL; }

#else /* RUNTIME_COMMAND_ENABLED == DEV_OFF */

dev_err_t svc_shell_register_command(const svc_shell_cmd_t *cmd)
    { DEV_UNUSED(cmd); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t svc_shell_unregister_command(const char *name)
    { DEV_UNUSED(name); return DEV_ERR_NOT_SUPPORTED; }
dev_err_t svc_shell_unregister_all_runtime_commands(void)
    { return DEV_ERR_NOT_SUPPORTED; }
uint16_t svc_shell_get_runtime_command_count(void) { return 0U; }
const svc_shell_cmd_t *svc_shell_private_get_runtime_cmd(uint16_t index)
    { DEV_UNUSED(index); return NULL; }

#endif

dev_err_t svc_shell_execute_line(char *line)
{
    char    *argv[SVC_SHELL_CFG_MAX_ARGS];
    uint8_t  argc = 0U;
    const svc_shell_cmd_t *cmd = NULL;
    dev_err_t e;

    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;
    if (!line) return DEV_ERR_NULL_PTR;

    e = svc_shell_parse_line(line, argv, SVC_SHELL_CFG_MAX_ARGS, &argc);
    if (e != DEV_OK) return e;
    if (argc == 0U) return DEV_OK;

    e = svc_shell_find_command(argv[0], &cmd);
    if (e != DEV_OK) {
        (void)svc_shell_write("Unknown command: ");
        (void)svc_shell_write_line(argv[0]);
        return e;
    }

    return cmd->function(argc, argv);
}

/*
 * Process one iteration of the shell.
 *
 * UI output calls (echo, prompt, newline) use (void) because they are best-effort:
 * a transient UART TX failure should not kill the shell loop. Command execution
 * errors (unknown command, parse failure, callback error) ARE propagated.
 */
dev_err_t svc_shell_handle(void)
{
    uint8_t  byte;
    uint16_t n;
    dev_err_t e;

    if (!g_state.initialized) return DEV_ERR_NOT_INITIALIZED;

    for (;;) {
        e = dev_uart_read(g_state.uart_id, &byte, 1U, &n, DEV_UART_TIMEOUT_NO_WAIT);
        if (e == DEV_ERR_EMPTY || n == 0U) break;
        if (e != DEV_OK) return e;

        if (byte == LF && g_state.last_was_cr) { g_state.last_was_cr = false; continue; }
        g_state.last_was_cr = (byte == CR);

        if (byte == CR || byte == LF) {
#if (SVC_SHELL_CFG_CRLF_ENABLED == DEV_ON)
            (void)svc_shell_write("\r\n");
#else
            (void)svc_shell_write("\n");
#endif
            if (g_state.line_len > 0U) {
                g_state.line[g_state.line_len] = '\0';
                e = svc_shell_execute_line(g_state.line);
                g_state.line_len = 0U;
                if (e != DEV_OK) return e;
            }
#if (SVC_SHELL_CFG_PROMPT_ENABLED == DEV_ON)
            (void)svc_shell_print_prompt();
#endif
            continue;
        }

        if (byte == BS || byte == DEL) {
#if (SVC_SHELL_CFG_BACKSPACE_ENABLED == DEV_ON)
            if (g_state.line_len > 0U) {
                g_state.line_len--;
#if (SVC_SHELL_CFG_ECHO_ENABLED == DEV_ON)
                (void)svc_shell_write("\b \b");
#endif
            }
#endif
            continue;
        }

        if (byte < SPC && byte != TAB) continue;

        if (g_state.line_len >= (SVC_SHELL_CFG_MAX_LINE_LENGTH - 1U)) {
            (void)svc_shell_write_line("");
            (void)svc_shell_write_line("Line too long, cleared.");
            g_state.line_len = 0U;
            (void)svc_shell_print_prompt();
            return DEV_ERR_OVERFLOW;
        }

        g_state.line[g_state.line_len++] = (char)byte;
#if (SVC_SHELL_CFG_ECHO_ENABLED == DEV_ON)
        (void)svc_shell_write_data(&byte, 1U);
#endif
    }
    return DEV_OK;
}
