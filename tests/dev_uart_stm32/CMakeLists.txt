cmake_minimum_required(VERSION 3.16)
project(uart_stm32_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)

add_subdirectory(${PROJECT_ROOT}/drivers/dev_common ${CMAKE_BINARY_DIR}/dev_common)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_ringbuf ${CMAKE_BINARY_DIR}/dev_ringbuf)

add_executable(uart_stm32_test_host
    test_uart_stm32.c
    ${PROJECT_ROOT}/drivers/dev_uart/port/stm32/dev_uart_port_stm32.c
)

target_include_directories(uart_stm32_test_host PRIVATE
    fake_hal
    ${PROJECT_ROOT}/drivers/dev_uart/include
    ${PROJECT_ROOT}/drivers/dev_uart/port/stm32
)

target_compile_options(uart_stm32_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
target_link_libraries(uart_stm32_test_host PRIVATE dev_common dev_ringbuf)
