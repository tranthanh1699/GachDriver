# dev_shell — Lightweight UART Command Shell

## 1. Overview

Polling-based command shell over UART. Internal singleton state — no shell handle needed.

- **init**: `dev_shell_init(DEV_UART_CONSOLE)` — auto-initializes `dev_uart`
- **main loop**: `dev_shell_handle()` — reads UART, echoes, executes commands
- **commands**: static table in `dev_shell_commands.c`
- **parser**: built-in arg splitting + int/hex/bool conversion

## 2. Quick Start

```c
#include "dev_shell.h"

int main(void) {
    dev_shell_init(DEV_UART_CONSOLE);
    for (;;) dev_shell_handle();
}
```

## 3. Adding a Command

1. Write a callback:
```c
static dev_err_t cmd_led(uint8_t argc, char *argv[]) {
    if (argc != 2U) { dev_shell_write_line("Usage: led <0|1>"); return DEV_ERR_INVALID_ARG; }
    uint8_t v; dev_shell_arg_to_u8(argv[1], &v);
    return dev_gpio_write(DEV_GPIO_LED_GREEN, v ? DEV_GPIO_LEVEL_HIGH : DEV_GPIO_LEVEL_LOW);
}
```

2. Add to table in `dev_shell_commands.c`:
```c
{ "led", "Set LED state", "led <0|1>", cmd_led },
```

## 4. API

| Function | Purpose |
|----------|---------|
| `init(uart_id)` | Initialize shell + UART |
| `deinit()` | Deinitialize |
| `handle()` | Process one iteration (call in loop) |
| `write(text)` | Write string to shell UART |
| `write_line(text)` | Write + newline |
| `write_data(data, len)` | Write raw bytes |
| `execute_line(line)` | Parse and execute a command line |
| `find_command(name, **cmd)` | Look up command by name |
| `is_initialized()` | Check state |
| `print_prompt()` | Print `root:` prompt |

## 5. Config (`dev_shell_cfg.h`)

```c
#define DEV_SHELL_CFG_MAX_LINE_LENGTH   (128U)
#define DEV_SHELL_CFG_MAX_ARGS          (8U)
#define DEV_SHELL_CFG_MAX_COMMANDS       (32U)
#define DEV_SHELL_CFG_DEFAULT_PROMPT    "root:"
```

## 6. Parser

```c
dev_shell_parse_line(line, argv, max_args, &argc);
dev_shell_arg_to_i32(arg, &value);
dev_shell_arg_to_u32/u16/u8(arg, &value);
dev_shell_arg_to_hex_u32(arg, &value);
dev_shell_arg_to_bool(arg, &value);   // true/false/1/0/on/off/yes/no
dev_shell_arg_is_equal(arg, "expected");
```

## 7. Build

```cmake
add_subdirectory(drivers/dev_shell)
target_link_libraries(${PROJECT_NAME} dev_shell)
```

18/18 tests pass.
