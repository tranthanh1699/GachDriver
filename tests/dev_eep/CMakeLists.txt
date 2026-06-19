cmake_minimum_required(VERSION 3.16)
project(eep_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)

# Mock I2C port
set(DEV_I2C_PORT "mock" CACHE STRING "I2C port for testing")

add_subdirectory(${PROJECT_ROOT}/drivers/dev_common ${CMAKE_BINARY_DIR}/dev_common)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_crc ${CMAKE_BINARY_DIR}/dev_crc)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_i2c ${CMAKE_BINARY_DIR}/dev_i2c)

# Compile dev_eep as part of the test (no separate library yet)
add_executable(eep_test_host
    test_eep.c
    ${PROJECT_ROOT}/drivers/dev_eep/src/dev_eep.c
    ${PROJECT_ROOT}/drivers/dev_eep/src/dev_eep_layout.c
)

target_include_directories(eep_test_host PRIVATE
    ${PROJECT_ROOT}/drivers/dev_eep/include
    ${PROJECT_ROOT}/drivers/dev_i2c/port/mock
)

target_link_libraries(eep_test_host PRIVATE dev_common dev_crc dev_i2c)
target_compile_options(eep_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
