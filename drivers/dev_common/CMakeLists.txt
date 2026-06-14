add_library(dev_common STATIC
    src/dev_common.c
    src/dev_assert.c
)

target_include_directories(dev_common PUBLIC include)
