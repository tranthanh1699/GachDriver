#ifndef SVC_EEP_TYPES_H
#define SVC_EEP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_i2c_types.h"

typedef uint8_t  svc_eep_id_t;
typedef uint32_t svc_eep_addr_t;
typedef uint32_t svc_eep_size_t;
typedef uint16_t svc_eep_field_id_t;

typedef enum
{
    SVC_EEP_MEM_ADDR_SIZE_8BIT = 0,
    SVC_EEP_MEM_ADDR_SIZE_16BIT,
    SVC_EEP_MEM_ADDR_SIZE_24BIT,
    SVC_EEP_MEM_ADDR_SIZE_32BIT
} svc_eep_mem_addr_size_t;

typedef struct
{
    svc_eep_field_id_t field_id;
    svc_eep_addr_t     offset;
    svc_eep_size_t     size;
    const char        *name;
} svc_eep_field_t;

typedef struct
{
    svc_eep_id_t            eep_id;
    dev_i2c_bus_t           i2c_bus;
    dev_i2c_addr_t          i2c_addr;
    svc_eep_size_t          total_size;
    svc_eep_size_t          page_size;
    svc_eep_mem_addr_size_t mem_addr_size;
    uint32_t                write_cycle_time_ms;
    uint8_t                *mirror;
    svc_eep_size_t          mirror_size;
    uint8_t                *dirty_map;
    svc_eep_size_t          dirty_map_size;
} svc_eep_device_t;

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_TYPES_H */
