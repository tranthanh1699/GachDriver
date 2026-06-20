cmake_minimum_required(VERSION 3.16)
project(queue_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_common ${CMAKE_BINARY_DIR}/dev_common)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_queue ${CMAKE_BINARY_DIR}/dev_queue)

add_executable(queue_test_host test_queue.c)
target_link_libraries(queue_test_host PRIVATE dev_common dev_queue)
target_compile_options(queue_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
