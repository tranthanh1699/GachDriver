if(NOT DEFINED DEV_ADC_PORT)
    set(DEV_ADC_PORT "mock" CACHE STRING "ADC port: mock, stm32, esp32")
endif()
message(STATUS "dev_adc: using port '${DEV_ADC_PORT}'")

add_library(dev_adc STATIC
    src/dev_adc.c
    port/${DEV_ADC_PORT}/dev_adc_port_${DEV_ADC_PORT}.c
)

target_include_directories(dev_adc
    PUBLIC  include
    PRIVATE port/${DEV_ADC_PORT}
)

target_link_libraries(dev_adc PUBLIC dev_common)

if(DEV_ADC_PORT STREQUAL "stm32")
    target_link_libraries(dev_adc PRIVATE stm32cubemx)
endif()
