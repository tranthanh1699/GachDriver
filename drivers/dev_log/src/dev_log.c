#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dev_log.h"

uint8_t g_uart_id = 0;

void dev_log_init(uint8_t uart_id)
{
    g_uart_id = uart_id;
    dev_uart_init();
}
void dev_log(const char* str, ...)
{
    char log_buffer[512];
    va_list args;
    va_start(args, str);
    vsnprintf(log_buffer, sizeof(log_buffer), str, args);
    va_end(args);
    
    uint16_t len = strlen(log_buffer);
    if (len > 0) {
        dev_uart_write(g_uart_id, (const uint8_t *)log_buffer, len, DEV_UART_TIMEOUT_DEFAULT_MS);
    }
}

void dev_log_hex(const uint8_t* data, uint32_t length)
{
    char hex_buf[1024];
    int hex_idx = 0;
    dev_log("Hex Dump - Length: %d\r\n", length);
    
    for (uint32_t i = 0; i < length; i++) {
        if (i % 16 == 0 && hex_idx > 0) {
            hex_buf[hex_idx] = '\0';
            dev_log("%s\r\n", hex_buf);
            hex_idx = 0;
        }
        if (i % 16 == 0) {
            hex_idx += snprintf(&hex_buf[hex_idx], sizeof(hex_buf) - hex_idx, "     ");
        }
        hex_idx += snprintf(&hex_buf[hex_idx], sizeof(hex_buf) - hex_idx, "%02X ", data[i]);
    }
    if (hex_idx > 0) {
        hex_buf[hex_idx] = '\0';
        dev_log("%s\r\n", hex_buf);
    }
}