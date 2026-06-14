if(NOT DEFINED DEV_UART_PORT)
    set(DEV_UART_PORT "mock" CACHE STRING "UART port: mock, stm32, esp32, nrf52")
endif()
message(STATUS "dev_uart: using port '${DEV_UART_PORT}'")

add_library(dev_uart STATIC
    src/dev_uart.c
    port/${DEV_UART_PORT}/dev_uart_port_${DEV_UART_PORT}.c
)

target_include_directories(dev_uart
    PUBLIC  include
    PRIVATE port/${DEV_UART_PORT}
)

target_link_libraries(dev_uart PUBLIC dev_common dev_ringbuf)

if(DEV_UART_PORT STREQUAL "stm32")
    target_link_libraries(dev_uart PRIVATE stm32cubemx)
endif()
