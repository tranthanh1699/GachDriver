#include "dev_uart_port_stm32.h"

#include <stdio.h>

UART_HandleTypeDef huart1;

uint32_t g_fake_uart_clear_ore_count;
uint32_t g_fake_uart_clear_ne_count;
uint32_t g_fake_uart_clear_fe_count;

static uint8_t *g_receive_target;
static uint32_t g_receive_calls;
static HAL_StatusTypeDef g_receive_status = HAL_OK;
static int g_failures;

#define CHECK(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s (%s:%d)\n", message, __FILE__, __LINE__); \
            g_failures++; \
        } \
    } while (0)

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart)
{
    (void)huart;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *data,
                                   uint16_t len, uint32_t timeout)
{
    (void)huart;
    (void)data;
    (void)len;
    (void)timeout;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *data,
                                     uint16_t len)
{
    (void)huart;
    CHECK(len == 1U, "receive length");
    g_receive_calls++;
    g_receive_target = data;
    return g_receive_status;
}

HAL_StatusTypeDef HAL_UART_AbortReceive_IT(UART_HandleTypeDef *huart)
{
    (void)huart;
    return HAL_OK;
}

static void reset_fake(void)
{
    huart1.ErrorCode = HAL_UART_ERROR_NONE;
    g_fake_uart_clear_ore_count = 0U;
    g_fake_uart_clear_ne_count = 0U;
    g_fake_uart_clear_fe_count = 0U;
    g_receive_target = NULL;
    g_receive_calls = 0U;
    g_receive_status = HAL_OK;
}

static void receive_byte(uint8_t value)
{
    CHECK(g_receive_target != NULL, "RX target armed");
    *g_receive_target = value;
    HAL_UART_RxCpltCallback(&huart1);
}

static void test_validation(void)
{
    uint8_t byte;
    uint16_t read_len = 123U;

    CHECK(dev_uart_port_read(DEV_UART_CONSOLE, NULL, 1U, &read_len, 0U)
          == DEV_ERR_NULL_PTR, "null data rejected");
    CHECK(dev_uart_port_read(DEV_UART_CONSOLE, &byte, 1U, NULL, 0U)
          == DEV_ERR_NULL_PTR, "null read length rejected");
    CHECK(dev_uart_port_read((dev_uart_id_t)99U, &byte, 1U, &read_len, 0U)
          == DEV_ERR_INVALID_ARG, "invalid UART rejected");
    CHECK(dev_uart_port_rx_available((dev_uart_id_t)99U) == 0U,
          "invalid UART availability is zero");
}

static void test_rx_and_overflow_accounting(void)
{
    uint8_t byte = 0U;
    uint16_t read_len = 0U;

    CHECK(dev_uart_port_init() == DEV_OK, "port init");
    CHECK(g_receive_calls == 1U, "RX armed at init");

    receive_byte(0x5AU);
    CHECK(dev_uart_port_read(DEV_UART_CONSOLE, &byte, 1U, &read_len, 0U)
          == DEV_OK, "received byte readable");
    CHECK(read_len == 1U && byte == 0x5AU, "received byte value");

    for (uint32_t i = 0U; i < DEV_UART_CONSOLE_RX_BUFFER_SIZE; ++i) {
        receive_byte((uint8_t)i);
    }
    CHECK(dev_uart_port_stm32_rx_dropped(DEV_UART_CONSOLE) == 1U,
          "full ring increments dropped counter");
}

static void test_error_recovery(void)
{
    uint32_t calls_before;

    CHECK(dev_uart_port_deinit() == DEV_OK, "port deinit");
    CHECK(dev_uart_port_init() == DEV_OK, "port reinit");
    calls_before = g_receive_calls;

    huart1.ErrorCode = HAL_UART_ERROR_ORE | HAL_UART_ERROR_NE | HAL_UART_ERROR_FE;
    HAL_UART_ErrorCallback(&huart1);

    CHECK(g_fake_uart_clear_ore_count == 1U, "ORE cleared");
    CHECK(g_fake_uart_clear_ne_count == 1U, "NE cleared");
    CHECK(g_fake_uart_clear_fe_count == 1U, "FE cleared");
    CHECK(huart1.ErrorCode == HAL_UART_ERROR_NONE, "HAL error state cleared");
    CHECK(g_receive_calls == calls_before + 1U, "RX rearmed after error");
}

static void test_nonblocking_error_keeps_current_receive(void)
{
    uint32_t calls_before = g_receive_calls;

    huart1.ErrorCode = HAL_UART_ERROR_NE | HAL_UART_ERROR_FE;
    HAL_UART_ErrorCallback(&huart1);

    CHECK(g_fake_uart_clear_ne_count == 2U, "second NE cleared");
    CHECK(g_fake_uart_clear_fe_count == 2U, "second FE cleared");
    CHECK(g_receive_calls == calls_before,
          "nonblocking error does not start a second receive");
}

int main(void)
{
    reset_fake();
    test_validation();
    test_rx_and_overflow_accounting();
    test_error_recovery();
    test_nonblocking_error_keeps_current_receive();

    printf("STM32 UART port: %s\n", (g_failures == 0) ? "PASS" : "FAIL");
    return (g_failures == 0) ? 0 : 1;
}
