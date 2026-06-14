#ifndef DEV_UART_TYPES_H
#define DEV_UART_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_types.h"

typedef uint8_t  dev_uart_id_t;
typedef uint32_t dev_uart_baudrate_t;
typedef uint32_t dev_uart_timeout_t;

typedef enum {
    DEV_UART_DATA_BITS_8 = 0,
    DEV_UART_DATA_BITS_9
} dev_uart_data_bits_t;

typedef enum {
    DEV_UART_STOP_BITS_1 = 0,
    DEV_UART_STOP_BITS_2
} dev_uart_stop_bits_t;

typedef enum {
    DEV_UART_PARITY_NONE = 0,
    DEV_UART_PARITY_EVEN,
    DEV_UART_PARITY_ODD
} dev_uart_parity_t;

typedef enum {
    DEV_UART_FLOW_CONTROL_NONE = 0,
    DEV_UART_FLOW_CONTROL_RTS,
    DEV_UART_FLOW_CONTROL_CTS,
    DEV_UART_FLOW_CONTROL_RTS_CTS
} dev_uart_flow_control_t;

#ifdef __cplusplus
}
#endif

#endif /* DEV_UART_TYPES_H */
