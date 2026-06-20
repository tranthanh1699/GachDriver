add_library(osal STATIC
    src/osal.c
    port/baremetal/osal_port_baremetal.c
)

target_include_directories(osal PUBLIC
    include
    port/baremetal
)

target_link_libraries(osal PUBLIC
    dev_common
    stm32cubemx
)
