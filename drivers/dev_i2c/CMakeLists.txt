if(NOT DEFINED DEV_I2C_PORT)
    set(DEV_I2C_PORT "mock" CACHE STRING "I2C port: mock, stm32, esp32")
endif()
message(STATUS "dev_i2c: using port '${DEV_I2C_PORT}'")

add_library(dev_i2c STATIC
    src/dev_i2c.c
    port/${DEV_I2C_PORT}/dev_i2c_port_${DEV_I2C_PORT}.c
)

target_include_directories(dev_i2c
    PUBLIC  include
    PRIVATE port/${DEV_I2C_PORT}
)

target_link_libraries(dev_i2c PUBLIC dev_common)

if(DEV_I2C_PORT STREQUAL "stm32")
    target_link_libraries(dev_i2c PRIVATE stm32cubemx)
endif()
