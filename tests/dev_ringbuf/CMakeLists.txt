cmake_minimum_required(VERSION 3.16)
project(ringbuf_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_common ${CMAKE_BINARY_DIR}/dev_common)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_ringbuf ${CMAKE_BINARY_DIR}/dev_ringbuf)

add_executable(ringbuf_test_host test_ringbuf.c)
target_link_libraries(ringbuf_test_host PRIVATE dev_common dev_ringbuf)
target_compile_options(ringbuf_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
