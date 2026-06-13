cmake_minimum_required(VERSION 3.16)
project(gpio_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)

add_executable(gpio_test_host
    ${PROJECT_ROOT}/drivers/dev_common/src/dev_common.c
    ${PROJECT_ROOT}/drivers/dev_common/src/dev_assert.c
    ${PROJECT_ROOT}/drivers/dev_gpio/src/dev_gpio.c
    ${PROJECT_ROOT}/drivers/dev_gpio/port/mock/dev_gpio_port_mock.c
    ${PROJECT_ROOT}/boards/board_mock/dev_gpio_board_cfg.c
    test_gpio.c
)

target_include_directories(gpio_test_host PRIVATE
    ${PROJECT_ROOT}/drivers/dev_common/include
    ${PROJECT_ROOT}/drivers/dev_gpio/include
    ${PROJECT_ROOT}/drivers/dev_gpio/port/mock
    ${PROJECT_ROOT}/boards/board_mock
)

target_compile_options(gpio_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
