add_library(dev_crc STATIC
    src/dev_crc.c
)

target_include_directories(dev_crc PUBLIC include)
target_link_libraries(dev_crc PUBLIC dev_common)
