add_library(svc_shell STATIC
    src/svc_shell.c
    src/svc_shell_parser.c
    src/svc_shell_commands.c
)

target_include_directories(svc_shell PUBLIC include)
target_link_libraries(svc_shell PUBLIC dev_common dev_uart)
