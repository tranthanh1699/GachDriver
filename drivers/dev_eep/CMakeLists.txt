add_library(dev_eep STATIC
    src/dev_eep.c
)

target_include_directories(dev_eep PUBLIC include)

target_link_libraries(dev_eep PUBLIC dev_common dev_i2c)
