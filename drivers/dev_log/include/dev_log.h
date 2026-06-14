#ifndef DEV_LOG_H
#define DEV_LOG_H
#include "stdint.h"
#include "dev_uart.h"
/**
 * @brief This function to log message
 * @param str The format string
 * @param ... The variable arguments
 */

void dev_log_init(uint8_t uart_id);
void dev_log(const char* str, ...); 
void dev_log_hex(const uint8_t* data, uint32_t length); 

// #define CONFIG_LOG_DEFAULT_LEVEL_NONE
#ifndef CONFIG_LOG_DEFAULT_LEVEL_NONE

#define DEV_LOG dev_log

/* Logging macros */
#define CONFIG_LOG_TAG(name, enable)	static const char *const_TAG = #name ; 														    \
										static const bool const_log_enabled = enable;


/* Logging macros */
#define CONFIG_LOG_TAG(name, enable)	static const char *const_TAG = #name ; 														    \
										static const bool const_log_enabled = enable;

#define DEV_LOG_RAW(msg,...)			if (const_log_enabled == true) { 															    \
											DEV_LOG(msg, ##__VA_ARGS__); 															    \
										}
									
#define DEV_LOG_ERR(msg,...)			if (const_log_enabled == true) { 															    \
											DEV_LOG("\033[0;31m---> [%s-%d]: " msg "\033[0m\r\n", const_TAG, __LINE__, ##__VA_ARGS__);  \
										}

#define DEV_LOG_WRN(msg,...)            if (const_log_enabled == true) { 															    \
											DEV_LOG("\033[0;33m---> [%s-%d]: " msg "\033[0m\r\n", const_TAG, __LINE__, ##__VA_ARGS__);  \
										}
										

#define DEV_LOG_INF(msg,...)            if (const_log_enabled == true) { 															    \
											DEV_LOG("\033[0;32m---> [%s-%d]: " msg "\033[0m\r\n", const_TAG, __LINE__, ##__VA_ARGS__);  \
										}

#define DEV_LOG_HEX                     dev_log_hex

#else
#define CONFIG_LOG_TAG(name, enable)	
#define DEV_LOG_BUFF(buff, len) 	
#define DEV_LOG_RAW(...)
#define DEV_LOG_ERR(...)
#define DEV_LOG_WRN(...)
#define DEV_LOG_INF(...)
#define DEV_LOG_HEX(...)
#endif /* CONFIG_LOG_DEFAULT_LEVEL_NONE */



#endif // DEV_LOG_H
