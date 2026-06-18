#ifndef STM32H7XX_HAL_H
#define STM32H7XX_HAL_H

#include <stdint.h>

#define HAL_UART_MODULE_ENABLED

typedef enum {
    HAL_OK = 0U,
    HAL_ERROR = 1U,
    HAL_BUSY = 2U,
    HAL_TIMEOUT = 3U
} HAL_StatusTypeDef;

typedef struct {
    uint32_t BaudRate;
    uint32_t WordLength;
    uint32_t StopBits;
    uint32_t Parity;
    uint32_t HwFlowCtl;
} UART_InitTypeDef;

typedef struct {
    UART_InitTypeDef Init;
    uint32_t ErrorCode;
} UART_HandleTypeDef;

#define UART_WORDLENGTH_8B  (0x00U)
#define UART_WORDLENGTH_9B  (0x01U)
#define UART_STOPBITS_1     (0x00U)
#define UART_STOPBITS_2     (0x02U)
#define UART_PARITY_NONE    (0x00U)
#define UART_PARITY_EVEN    (0x04U)
#define UART_PARITY_ODD     (0x06U)
#define UART_HWCONTROL_NONE (0x00U)
#define UART_HWCONTROL_RTS  (0x08U)
#define UART_HWCONTROL_CTS  (0x10U)
#define UART_HWCONTROL_RTS_CTS (0x18U)

#define HAL_UART_ERROR_NONE (0x00000000U)
#define HAL_UART_ERROR_PE   (0x00000001U)
#define HAL_UART_ERROR_NE   (0x00000002U)
#define HAL_UART_ERROR_FE   (0x00000004U)
#define HAL_UART_ERROR_ORE  (0x00000008U)

extern uint32_t g_fake_uart_clear_ore_count;
extern uint32_t g_fake_uart_clear_ne_count;
extern uint32_t g_fake_uart_clear_fe_count;

#define __HAL_UART_CLEAR_OREFLAG(huart) \
    do { (void)(huart); g_fake_uart_clear_ore_count++; } while (0)
#define __HAL_UART_CLEAR_NEFLAG(huart) \
    do { (void)(huart); g_fake_uart_clear_ne_count++; } while (0)
#define __HAL_UART_CLEAR_FEFLAG(huart) \
    do { (void)(huart); g_fake_uart_clear_fe_count++; } while (0)

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *data,
                                   uint16_t len, uint32_t timeout);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *data,
                                     uint16_t len);
HAL_StatusTypeDef HAL_UART_AbortReceive_IT(UART_HandleTypeDef *huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

#endif
