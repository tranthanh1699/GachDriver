# GPIO port selection — set before including this file, or defaults to mock
if(NOT DEFINED DEV_GPIO_PORT)
    set(DEV_GPIO_PORT "mock" CACHE STRING "GPIO port target: mock, stm32, esp32, nrf52")
endif()
message(STATUS "dev_gpio: using port '${DEV_GPIO_PORT}'")

add_library(dev_gpio STATIC
    src/dev_gpio.c
    port/${DEV_GPIO_PORT}/dev_gpio_port_${DEV_GPIO_PORT}.c
)

target_include_directories(dev_gpio
    PUBLIC  include
    PRIVATE port/${DEV_GPIO_PORT}
)

target_link_libraries(dev_gpio PUBLIC dev_common)
