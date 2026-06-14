add_library(dev_ringbuf STATIC
    src/dev_ringbuf.c
)

target_include_directories(dev_ringbuf PUBLIC include)
target_link_libraries(dev_ringbuf PUBLIC dev_common)
