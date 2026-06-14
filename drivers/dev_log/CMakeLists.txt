add_library(dev_log STATIC
    src/dev_log.c
)

target_include_directories(dev_log PUBLIC include)

target_link_libraries(dev_log PUBLIC 
    dev_common 
    dev_uart
)
