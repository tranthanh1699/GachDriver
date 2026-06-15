add_library(dev_shell STATIC
    src/dev_shell.c
    src/dev_shell_parser.c
    src/dev_shell_commands.c
)

target_include_directories(dev_shell PUBLIC include)
target_link_libraries(dev_shell PUBLIC dev_common dev_uart)
