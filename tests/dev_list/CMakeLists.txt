cmake_minimum_required(VERSION 3.16)
project(list_test_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)
add_subdirectory(${PROJECT_ROOT}/drivers/dev_list ${CMAKE_BINARY_DIR}/dev_list)

add_executable(list_test_host test_list.c)
target_link_libraries(list_test_host PRIVATE dev_list)
target_compile_options(list_test_host PRIVATE -Wall -Wextra -Werror -pedantic)
