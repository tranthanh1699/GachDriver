# dev_shell — Lightweight UART Command Shell

## 1. Overview

A polling-based command shell that runs over a single UART. Call `dev_shell_handle()` in your main loop — it reads incoming bytes, echoes them, handles backspace, detects Enter, parses the command line, finds the matching command, and calls your callback.

- **No dynamic memory** — all buffers are static
- **No RTOS dependency** — pure polling, non-blocking
- **Single UART** — initialized with one `dev_uart_id_t`
- **Auto-initializes `dev_uart`** — you don't need to call `dev_uart_init()` separately

---

## 2. Quick Start — Minimal Working Setup

### Step 1: Include and init

```c
#include "dev_shell.h"

int main(void)
{
    /* Initialize shell on CONSOLE UART. This also calls dev_uart_init() internally. */
    dev_shell_init(DEV_UART_CONSOLE);

    /* Main loop — call handle() as fast as possible */
    for (;;) {
        dev_shell_handle();
    }
}
```

That's it. The shell is now running on `DEV_UART_CONSOLE`. Connect a terminal (115200 baud) and type `help` followed by Enter.

### Step 2: What you see

```
root: help
Available commands:
  help       Show command list or command details
  explain    Explain shell usage
root: explain
dev_shell is a small UART command shell.
...
root:
```

### Step 3: Understanding the flow

```
User types in terminal  →  UART RX interrupt stores bytes in dev_ringbuf
                            ↓
dev_shell_handle()       →  reads one byte via dev_uart_read()
                            ↓
                            if printable → store in line buffer, echo back
                            if backspace → remove last char
                            if Enter     → parse line, find command, call callback
                            ↓
                            callback receives (argc, argv[])
                            callback writes output via dev_shell_write_line()
```

---

## 3. Registering a New Command — Step by Step

Let's add a `led` command that controls an LED.

### Step 1: Write the callback function

Create a function with exactly this signature:

```c
static dev_err_t my_cmd_led(uint8_t argc, char *argv[])
```

Add it anywhere in your application code (or in `dev_shell_commands.c`):

```c
#include "dev_shell.h"        /* for dev_shell_write_line, dev_shell_arg_to_u8 */
#include "dev_gpio.h"         /* for dev_gpio_write */

static dev_err_t my_cmd_led(uint8_t argc, char *argv[])
{
    uint8_t value;
    dev_err_t err;

    /*
     * argc = number of arguments including the command name.
     * argv[0] = "led" (the command itself)
     * argv[1] = first argument (e.g., "1" or "0")
     */

    /* Validate argument count */
    if (argc != 2U) {
        dev_shell_write_line("Usage: led <0|1>");
        return DEV_ERR_INVALID_ARG;
    }

    /* Parse argv[1] as unsigned 8-bit integer */
    err = dev_shell_arg_to_u8(argv[1], &value);
    if (err != DEV_OK) {
        dev_shell_write_line("Error: expected 0 or 1");
        return err;
    }

    /* Control the LED */
    if (value == 1U) {
        err = dev_gpio_write(DEV_GPIO_LED_GREEN, DEV_GPIO_LEVEL_HIGH);
    } else {
        err = dev_gpio_write(DEV_GPIO_LED_GREEN, DEV_GPIO_LEVEL_LOW);
    }

    if (err == DEV_OK) {
        dev_shell_write_line("OK");
    }

    return err;
}
```

### Step 2: Add to the command table

Open `drivers/dev_shell/src/dev_shell_commands.c`. Add one line to `g_dev_shell_commands[]`:

```c
const dev_shell_cmd_t g_dev_shell_commands[] = {
    { "help",    "Show command list or command details", "help [command]", dev_shell_cmd_help },
    { "explain", "Explain shell usage",                  "explain",        dev_shell_cmd_explain },
    { "led",     "Set LED state",                        "led <0|1>",      my_cmd_led },   /* ← ADD THIS LINE */
};
```

The four fields are:

| Field | Meaning | Example |
|-------|---------|---------|
| `name` | Command name (no spaces, lowercase convention) | `"led"` |
| `help` | One-line description shown by `help` | `"Set LED state"` |
| `usage` | Usage string shown by `help led` | `"led <0|1>"` |
| `function` | Your callback function | `my_cmd_led` |

### Step 3: Rebuild and test

Rebuild, flash, connect terminal:

```
root: led 1
OK
root: led 0
OK
root: led 99
Error: expected 0 or 1
root: help led
Command: led
Usage: led <0|1>
Description: Set LED state
```

That's it. One callback + one table entry = new command.

---

## 4. Registering a Runtime Command

Runtime commands can be added at runtime without modifying `dev_shell_commands.c`. Useful when a module wants to register its own shell commands during its init.

### Enable the feature

```c
#define DEV_SHELL_CFG_RUNTIME_COMMAND_ENABLED (1U)
#define DEV_SHELL_CFG_MAX_RUNTIME_COMMANDS    (8U)
```

### Registration

```c
#include "dev_shell.h"

static dev_err_t cmd_adc(uint8_t argc, char *argv[]) {
    (void)argc; (void)argv;
    return dev_shell_write_line("ADC command called");
}

static const dev_shell_cmd_t adc_cmd = {
    "adc",              /* command name (must be unique) */
    "Read ADC value",   /* help text */
    "adc <channel>",    /* usage string */
    cmd_adc             /* callback */
};

void app_init(void) {
    dev_shell_init(DEV_UART_CONSOLE);
    dev_shell_register_command(&adc_cmd);   /* register at runtime */
}
```

### Unregistration

```c
dev_shell_unregister_command("adc");              /* remove one */
dev_shell_unregister_all_runtime_commands();      /* remove all */
uint16_t n = dev_shell_get_runtime_command_count(); /* count */
```

### Rules

- Runtime commands are searched AFTER static commands → static has priority
- Duplicate names rejected against both static AND runtime tables
- Static commands CANNOT be unregistered
- Caller must keep command strings + callback alive for registration lifetime
- Runtime table cleared on `dev_shell_init()`

### help command output

```
Built-in commands:
  help       Show command list
  explain    Explain shell usage
  hello      Print Hello, World!

Runtime commands:
  adc        Read ADC value
```

---

## 5. Parser Functions — Converting Arguments

Your callback receives `argc` and `argv[]`. Use these parser helpers to convert string arguments to typed values:

```c
/* String comparison */
bool dev_shell_arg_is_equal(const char *arg, const char *expected);
/* Usage: if (dev_shell_arg_is_equal(argv[1], "on")) { ... } */

/* Integer conversions */
dev_err_t dev_shell_arg_to_i32(const char *arg, int32_t *value);
dev_err_t dev_shell_arg_to_u32(const char *arg, uint32_t *value);
dev_err_t dev_shell_arg_to_u16(const char *arg, uint16_t *value);
dev_err_t dev_shell_arg_to_u8(const char *arg, uint8_t *value);
/* Usage: dev_shell_arg_to_u16(argv[1], &port); */

/* Hexadecimal */
dev_err_t dev_shell_arg_to_hex_u32(const char *arg, uint32_t *value);
/* Usage: dev_shell_arg_to_hex_u32(argv[1], &addr);  // "0xFF" → 255 */

/* Boolean (true/false/1/0/on/off/yes/no) */
dev_err_t dev_shell_arg_to_bool(const char *arg, bool *value);
/* Usage: dev_shell_arg_to_bool(argv[1], &enable); */
```

All conversion functions return `DEV_OK` on success, `DEV_ERR_PARSE` if the string is not a valid number, or `DEV_ERR_NULL_PTR` if arguments are NULL.

### Example: Command that takes multiple argument types

```c
static dev_err_t cmd_sensor(uint8_t argc, char *argv[])
{
    if (argc == 2U && dev_shell_arg_is_equal(argv[1], "read")) {
        uint32_t value;
        /* Read sensor here */
        return DEV_OK;
    }

    if (argc == 3U && dev_shell_arg_is_equal(argv[1], "write")) {
        uint8_t val;
        dev_err_t e = dev_shell_arg_to_u8(argv[2], &val);
        if (e != DEV_OK) { dev_shell_write_line("Usage: sensor write <0-255>"); return e; }
        /* Write sensor here */
        return DEV_OK;
    }

    dev_shell_write_line("Usage: sensor <read|write> [value]");
    return DEV_ERR_INVALID_ARG;
}
```

---

## 6. Configuration — All Settings in One File

All configuration macros are in `drivers/dev_shell/include/dev_shell_cfg.h`. You only need to edit this one file.

### Shell behavior

```c
/* Maximum length of a single command line (including Enter) */
#define DEV_SHELL_CFG_MAX_LINE_LENGTH         (128U)

/* Maximum number of space-separated arguments per command */
#define DEV_SHELL_CFG_MAX_ARGS                (8U)

/* Maximum number of commands in the command table */
#define DEV_SHELL_CFG_MAX_COMMANDS            (32U)

/* Maximum length of formatted output strings (help, explain, etc.) */
#define DEV_SHELL_CFG_MAX_OUTPUT_LENGTH       (256U)
```

### Input features (0U = off, 1U = on)

```c
/* Echo typed characters back to terminal */
#define DEV_SHELL_CFG_ECHO_ENABLED            (1U)

/* Print "root:" prompt after Enter */
#define DEV_SHELL_CFG_PROMPT_ENABLED          (1U)

/* Use \r\n for newlines (required for most terminals) */
#define DEV_SHELL_CFG_CRLF_ENABLED            (1U)

/* Handle Backspace key to delete last character */
#define DEV_SHELL_CFG_BACKSPACE_ENABLED       (1U)

/* Support quoted arguments like cmd "hello world" */
#define DEV_SHELL_CFG_QUOTE_PARSE_ENABLED     (1U)

/* Validate init state before each API call */
#define DEV_SHELL_CFG_RUNTIME_CHECK_ENABLED   (1U)
```

### Prompt text

```c
#define DEV_SHELL_CFG_DEFAULT_PROMPT          "root:"
```

Change this to customize the prompt. Example:

```c
#define DEV_SHELL_CFG_DEFAULT_PROMPT          "gach>"
```

Terminal output becomes:

```
gach> help
```

### Customizing for your project

| If you want... | Change this |
|----------------|-------------|
| Longer command lines | Increase `DEV_SHELL_CFG_MAX_LINE_LENGTH` |
| More arguments per command | Increase `DEV_SHELL_CFG_MAX_ARGS` |
| Disable echo (password input) | Set `DEV_SHELL_CFG_ECHO_ENABLED` to `0U` |
| Disable backspace | Set `DEV_SHELL_CFG_BACKSPACE_ENABLED` to `0U` |
| Different prompt | Change `DEV_SHELL_CFG_DEFAULT_PROMPT` |
| LF-only newlines (some terminals) | Set `DEV_SHELL_CFG_CRLF_ENABLED` to `0U` |

---

## 7. Output Functions — Writing from Your Command

Command callbacks should use these functions to send output back to the terminal:

```c
/* Write plain text (no newline added) */
dev_err_t dev_shell_write("Processing...");

/* Write text followed by newline (\r\n or \n depending on CRLF setting) */
dev_err_t dev_shell_write_line("Done.");

/* Write raw binary data of specified length */
dev_err_t dev_shell_write_data(&byte, 1U);
```

Do NOT use `printf()` or `dev_uart_write()` directly in command callbacks — use the shell's output functions. They automatically use the correct UART ID.

---

## 8. Complete API Reference

```c
dev_err_t dev_shell_init(dev_uart_id_t uart_id);       /* Init shell + UART */
dev_err_t dev_shell_deinit(void);                       /* Deinit shell */
dev_err_t dev_shell_handle(void);                       /* Process one iteration */
dev_err_t dev_shell_print_prompt(void);                 /* Print prompt now */
dev_err_t dev_shell_write(const char *text);            /* Write string */
dev_err_t dev_shell_write_line(const char *text);       /* Write string + newline */
dev_err_t dev_shell_write_data(const uint8_t *d, uint16_t len); /* Write raw bytes */
dev_err_t dev_shell_execute_line(char *line);           /* Parse and execute a line */
dev_err_t dev_shell_find_command(const char *name, const dev_shell_cmd_t **cmd); /* Lookup */
bool     dev_shell_is_initialized(void);                /* Check state */
dev_err_t dev_shell_register_command(const dev_shell_cmd_t *cmd);        /* Runtime register */
dev_err_t dev_shell_unregister_command(const char *name);                 /* Runtime unregister */
dev_err_t dev_shell_unregister_all_runtime_commands(void);                /* Clear runtime */
uint16_t dev_shell_get_runtime_command_count(void);                         /* Count runtime */
```

---

## 9. Command Table Validation

At init time, `dev_shell_init()` validates the command table:
- Rejects more than `DEV_SHELL_CFG_MAX_COMMANDS` entries
- Rejects duplicate command names
- Rejects NULL command names
- Rejects NULL function pointers

If validation fails, `dev_shell_init()` returns `DEV_ERR_CONFIG`.

---

## 10. Build

```cmake
add_subdirectory(drivers/dev_shell)
target_link_libraries(${PROJECT_NAME} dev_shell)
```

Dependencies: `dev_common`, `dev_uart`.

---

## 11. Key Design Points

| Rule | Why |
|------|-----|
| No dynamic memory | All buffers are static arrays, MISRA compliant |
| No RTOS calls | Pure polling — works in bare-metal and RTOS |
| No vendor headers in public API | Portable across STM32, ESP32, nRF52 |
| Command table is `const` static | Compile-time registration, no runtime overhead |
| UART auto-init | `dev_shell_init()` calls `dev_uart_init()` for you |
| `dev_shell_handle()` returns quickly | Non-blocking — safe in main loop or low-priority task |
