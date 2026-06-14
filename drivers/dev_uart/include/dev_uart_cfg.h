#ifndef DEV_UART_CFG_H
#define DEV_UART_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_uart_types.h"

#define DEV_UART_CFG_MAX_INSTANCES             (3U)
#define DEV_UART_CFG_RUNTIME_CHECK_ENABLED     (1U)
#define DEV_UART_CFG_RX_BUFFER_ENABLED         (1U)

#define DEV_UART_TIMEOUT_DEFAULT_MS            (100U)
#define DEV_UART_TIMEOUT_NO_WAIT               (0U)
#define DEV_UART_TIMEOUT_FOREVER               (0xFFFFFFFFUL)
#define DEV_UART_CFG_MAX_STRING_LENGTH         (256U)

#define DEV_UART_BAUDRATE_9600                 (9600UL)
#define DEV_UART_BAUDRATE_19200                (19200UL)
#define DEV_UART_BAUDRATE_38400                (38400UL)
#define DEV_UART_BAUDRATE_57600                (57600UL)
#define DEV_UART_BAUDRATE_115200               (115200UL)
#define DEV_UART_BAUDRATE_921600               (921600UL)

#define DEV_UART_CONSOLE                       ((dev_uart_id_t)0U)
#define DEV_UART_GNSS                          ((dev_uart_id_t)1U)
#define DEV_UART_MODEM                         ((dev_uart_id_t)2U)

#define DEV_UART_CONSOLE_RX_BUFFER_SIZE        (256U)
#define DEV_UART_GNSS_RX_BUFFER_SIZE           (512U)
#define DEV_UART_MODEM_RX_BUFFER_SIZE          (1024U)

#ifdef __cplusplus
}
#endif

#endif /* DEV_UART_CFG_H */
