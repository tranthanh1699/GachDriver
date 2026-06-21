add_library(svc_eep STATIC
    src/svc_eep.c
    src/svc_eep_layout.c
)

target_include_directories(svc_eep PUBLIC include)
target_link_libraries(svc_eep PUBLIC dev_common dev_eep dev_crc)
