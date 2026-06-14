cmake_minimum_required(VERSION 3.16)
project(crc_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_common ${CMAKE_BINARY_DIR}/dev_common)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_crc ${CMAKE_BINARY_DIR}/dev_crc)

add_executable(crc_test_host test_crc.c)
target_link_libraries(crc_test_host PRIVATE dev_common dev_crc)
target_compile_options(crc_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
