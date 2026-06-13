#ifndef DEV_GPIO_PORT_MOCK_H
#define DEV_GPIO_PORT_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"

/* Operations that can be targeted for error injection */
typedef enum {
    DEV_GPIO_PORT_MOCK_OP_INIT = 0,
    DEV_GPIO_PORT_MOCK_OP_DEINIT,
    DEV_GPIO_PORT_MOCK_OP_CONFIG_CHANNEL,
    DEV_GPIO_PORT_MOCK_OP_READ,
    DEV_GPIO_PORT_MOCK_OP_WRITE,
    DEV_GPIO_PORT_MOCK_OP_TOGGLE,
    DEV_GPIO_PORT_MOCK_OP_SET_DIRECTION,
    DEV_GPIO_PORT_MOCK_OP_SET_PULL,
    DEV_GPIO_PORT_MOCK_OP_CONFIG_INTERRUPT,
    DEV_GPIO_PORT_MOCK_OP_ENABLE_INTERRUPT,
    DEV_GPIO_PORT_MOCK_OP_DISABLE_INTERRUPT,
    DEV_GPIO_PORT_MOCK_OP_COUNT
} dev_gpio_port_mock_op_t;

/* Global error injection: all subsequent calls fail with this error */
void dev_gpio_port_mock_set_error(dev_err_t error);

/* Per-operation injection: only the specified operation fails */
void dev_gpio_port_mock_set_error_for_op(dev_gpio_port_mock_op_t op, dev_err_t error);

/* Fail-after-N: the Nth call to any port operation fails */
void dev_gpio_port_mock_set_fail_after(uint16_t call_count, dev_err_t error);

/* Clear all injected errors, reset call counter */
void dev_gpio_port_mock_clear_error(void);

/* ISR simulation: invoke the common ISR dispatch */
void dev_gpio_port_mock_trigger_isr(dev_gpio_channel_t channel);

/* State inspection for test assertions */
dev_gpio_level_t     dev_gpio_port_mock_get_level(dev_gpio_channel_t channel);
dev_gpio_direction_t dev_gpio_port_mock_get_direction(dev_gpio_channel_t channel);
dev_gpio_pull_t      dev_gpio_port_mock_get_pull(dev_gpio_channel_t channel);
bool                 dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_MOCK_H */
