add_library(dev_queue STATIC
    src/dev_queue.c
)

target_include_directories(dev_queue PUBLIC include)
target_link_libraries(dev_queue PUBLIC dev_common)
