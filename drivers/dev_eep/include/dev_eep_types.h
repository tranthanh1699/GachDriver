#ifndef DEV_EEP_TYPES_H
#define DEV_EEP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_i2c_types.h"

/* ── EEPROM logical ID ── */

typedef uint8_t  dev_eep_id_t;

/* ── EEPROM address and size ── */

typedef uint32_t dev_eep_addr_t;
typedef uint32_t dev_eep_size_t;

/* ── Memory address width ── */

typedef enum
{
    DEV_EEP_MEM_ADDR_SIZE_8BIT = 0,
    DEV_EEP_MEM_ADDR_SIZE_16BIT,
    DEV_EEP_MEM_ADDR_SIZE_24BIT,
    DEV_EEP_MEM_ADDR_SIZE_32BIT
} dev_eep_mem_addr_size_t;

/* ── Device configuration (static, read-only) ── */

typedef struct
{
    dev_eep_id_t            eep_id;
    dev_i2c_bus_t           i2c_bus;
    dev_i2c_addr_t          i2c_addr;
    dev_eep_size_t          total_size;
    dev_eep_size_t          page_size;
    dev_eep_mem_addr_size_t mem_addr_size;
    uint32_t                write_cycle_time_ms;
} dev_eep_config_t;

/* ── Device info (queried at runtime) ── */

typedef struct
{
    dev_eep_size_t          total_size;
    dev_eep_size_t          page_size;
    dev_eep_mem_addr_size_t mem_addr_size;
} dev_eep_info_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_TYPES_H */
