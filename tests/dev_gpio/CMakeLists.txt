cmake_minimum_required(VERSION 3.16)
project(gpio_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)

add_subdirectory(${PROJECT_ROOT}/drivers/dev_common ${CMAKE_BINARY_DIR}/dev_common)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_gpio ${CMAKE_BINARY_DIR}/dev_gpio)

add_executable(gpio_test_host test_gpio.c)
target_include_directories(gpio_test_host PRIVATE
    ${PROJECT_ROOT}/drivers/dev_gpio/port/mock
)
target_link_libraries(gpio_test_host PRIVATE dev_common dev_gpio)
target_compile_options(gpio_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
