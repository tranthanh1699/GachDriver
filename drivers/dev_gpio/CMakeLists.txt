add_library(dev_gpio STATIC
    src/dev_gpio_cfg.c
    src/dev_gpio.c
)

target_include_directories(dev_gpio PUBLIC include)

target_link_libraries(dev_gpio
    PUBLIC  dev_common
    PRIVATE stm32cubemx
)
