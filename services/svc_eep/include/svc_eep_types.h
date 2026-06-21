#ifndef SVC_EEP_TYPES_H
#define SVC_EEP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"
#include "dev_eep_types.h"

typedef uint8_t  svc_eep_id_t;
typedef uint32_t svc_eep_addr_t;
typedef uint32_t svc_eep_size_t;
typedef uint16_t svc_eep_field_id_t;

typedef struct
{
    svc_eep_field_id_t field_id;
    svc_eep_addr_t     offset;
    svc_eep_size_t     size;
    const char        *name;
} svc_eep_field_t;

/*
 * Service-level device descriptor.
 * Device-level details (I2C bus, I2C addr, page size, etc.) are owned by dev_eep.
 * svc_eep owns only the RAM mirror and dirty map.
 */
typedef struct
{
    svc_eep_id_t  eep_id;
    uint8_t      *mirror;
    svc_eep_size_t mirror_size;
    uint8_t      *dirty_map;
    svc_eep_size_t dirty_map_size;
} svc_eep_device_t;

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_TYPES_H */
