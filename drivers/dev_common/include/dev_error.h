#ifndef DEV_ERROR_H
#define DEV_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef enum {
    DEV_OK = 0,
    DEV_ERR_FAIL,
    DEV_ERR_INVALID_ARG,
    DEV_ERR_NULL_PTR,
    DEV_ERR_INVALID_STATE,
    DEV_ERR_NOT_INITIALIZED,
    DEV_ERR_ALREADY_INITIALIZED,
    DEV_ERR_NOT_SUPPORTED,
    DEV_ERR_TIMEOUT,
    DEV_ERR_BUSY,
    DEV_ERR_OUT_OF_RANGE,
    DEV_ERR_HW_FAILURE,
    DEV_ERR_CONFIG,
    DEV_ERR_NO_ACK,
    DEV_ERR_BUS,
    DEV_ERR_OVERFLOW,
    DEV_ERR_EMPTY,
    DEV_ERR_NOT_FOUND,
    DEV_ERR_PARSE,
    DEV_ERR_CRC
} dev_err_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_ERROR_H */
