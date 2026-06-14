cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME GachDriver)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# ── Driver modules (each owns its own CMakeLists.txt) ──
set(DEV_GPIO_PORT "stm32" CACHE STRING "GPIO port: mock, stm32")
set(DEV_I2C_PORT  "stm32" CACHE STRING "I2C port: mock, stm32, esp32")
set(DEV_UART_PORT "stm32" CACHE STRING "UART port: mock, stm32, esp32, nrf52")
add_subdirectory(drivers/dev_common)
add_subdirectory(drivers/dev_gpio)
add_subdirectory(drivers/dev_i2c)
add_subdirectory(drivers/dev_list)
add_subdirectory(drivers/dev_crc)
add_subdirectory(drivers/dev_ringbuf)
add_subdirectory(drivers/dev_uart)

target_link_libraries(${CMAKE_PROJECT_NAME}
    dev_common
    dev_gpio
    dev_i2c
    dev_list
    dev_crc
    dev_ringbuf
    dev_uart
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
