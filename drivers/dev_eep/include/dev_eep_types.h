#ifndef DEV_EEP_TYPES_H
#define DEV_EEP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_i2c_types.h"

typedef uint8_t  dev_eep_id_t;
typedef uint32_t dev_eep_addr_t;
typedef uint32_t dev_eep_size_t;
typedef uint16_t dev_eep_field_id_t;

typedef enum
{
    DEV_EEP_MEM_ADDR_SIZE_8BIT = 0,
    DEV_EEP_MEM_ADDR_SIZE_16BIT,
    DEV_EEP_MEM_ADDR_SIZE_24BIT,
    DEV_EEP_MEM_ADDR_SIZE_32BIT
} dev_eep_mem_addr_size_t;

typedef struct
{
    dev_eep_field_id_t field_id;
    dev_eep_addr_t     offset;
    dev_eep_size_t     size;
    const char        *name;
} dev_eep_field_t;

typedef struct
{
    dev_eep_id_t            eep_id;
    dev_i2c_bus_t           i2c_bus;
    dev_i2c_addr_t          i2c_addr;
    dev_eep_size_t          total_size;
    dev_eep_size_t          page_size;
    dev_eep_mem_addr_size_t mem_addr_size;
    uint32_t                write_cycle_time_ms;
    uint8_t                *mirror;
    dev_eep_size_t          mirror_size;
    uint8_t                *dirty_map;
    dev_eep_size_t          dirty_map_size;
} dev_eep_device_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_TYPES_H */
