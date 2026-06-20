add_library(svc_sm STATIC
    src/svc_sm.c
    src/svc_sm_modules.c
    src/svc_sm_app.c
)

target_include_directories(svc_sm PUBLIC include)
target_link_libraries(svc_sm PUBLIC dev_common svc_eep svc_shell dev_uart)
