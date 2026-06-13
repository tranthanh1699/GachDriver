cmake_minimum_required(VERSION 3.16)
project(gpio_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(gpio_test_host
    ${CMAKE_SOURCE_DIR}/drivers/dev_common/src/dev_common.c
    ${CMAKE_SOURCE_DIR}/drivers/dev_common/src/dev_assert.c
    ${CMAKE_SOURCE_DIR}/drivers/dev_gpio/src/dev_gpio.c
    ${CMAKE_SOURCE_DIR}/drivers/dev_gpio/port/mock/dev_gpio_port_mock.c
    ${CMAKE_SOURCE_DIR}/boards/board_mock/dev_gpio_board_cfg.c
    test_gpio.c
)

target_include_directories(gpio_test_host PRIVATE
    ${CMAKE_SOURCE_DIR}/drivers/dev_common/include
    ${CMAKE_SOURCE_DIR}/drivers/dev_gpio/include
    ${CMAKE_SOURCE_DIR}/drivers/dev_gpio/port/mock
    ${CMAKE_SOURCE_DIR}/boards/board_mock
)

target_compile_options(gpio_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
