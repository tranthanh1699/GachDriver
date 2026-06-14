cmake_minimum_required(VERSION 3.16)
project(i2c_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)
set(DEV_I2C_PORT "mock" CACHE STRING "" FORCE)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_common ${CMAKE_BINARY_DIR}/dev_common)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_i2c ${CMAKE_BINARY_DIR}/dev_i2c)

add_executable(i2c_test_host test_i2c.c)
target_include_directories(i2c_test_host PRIVATE ${PROJECT_ROOT}/drivers/dev_i2c/port/mock)
target_link_libraries(i2c_test_host PRIVATE dev_common dev_i2c)
target_compile_options(i2c_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
