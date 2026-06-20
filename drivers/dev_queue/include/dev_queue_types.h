#ifndef DEV_QUEUE_TYPES_H
#define DEV_QUEUE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef uint16_t dev_queue_size_t;
typedef uint16_t dev_queue_item_size_t;

typedef struct
{
    uint8_t               *buffer;
    dev_queue_item_size_t  item_size;
    dev_queue_size_t       capacity;
    dev_queue_size_t       head;
    dev_queue_size_t       tail;
    dev_queue_size_t       count;
    bool                   initialized;
} dev_queue_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_QUEUE_TYPES_H */
