# dev_shell Design Specification

**Status:** Approved. Implementation via writing-plans.

## Architecture

9 files: 5 headers + 3 .c + CMakeLists. Uses internal singleton state. Command table is static. All I/O through dev_uart public API.

## Files

```
drivers/dev_shell/
  include/ dev_shell.h, dev_shell_types.h, dev_shell_cfg.h, dev_shell_parser.h, dev_shell_commands.h
  src/     dev_shell.c, dev_shell_parser.c, dev_shell_commands.c
  CMakeLists.txt
tests/dev_shell/ test_shell.c + CMakeLists.txt
docs/dev_shell/  README.md
```

## Modifications

- dev_error.h: +DEV_ERR_NOT_FOUND, DEV_ERR_PARSE
- root CMakeLists.txt: +add_subdirectory(dev_shell)
- root README.md: +dev_shell row in component table

## Key Types

- dev_shell_cmd_fn_t(argc, argv) -> dev_err_t
- dev_shell_cmd_t { name, help, usage, function }
- g_dev_shell_commands[] + g_dev_shell_command_count (extern)

## Key APIs

- init(uart_id), deinit(), handle(), print_prompt()
- write(text), write_line(text), execute_line(line)
- find_command(name, **cmd), is_initialized()
- parse_line, arg_to_i32/u32/u16/u8/bool/hex, arg_is_equal

## Config

MAX_COMMANDS=32, MAX_ARGS=8, MAX_LINE_LENGTH=128,
ECHO/PROMPT/CRLF/BACKSPACE/QUOTE/RUNTIME_CHECK enabled,
DEFAULT_PROMPT="root:"
