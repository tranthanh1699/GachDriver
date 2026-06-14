#ifndef DEV_GPIO_PORT_MOCK_H
#define DEV_GPIO_PORT_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_port.h"

void dev_gpio_port_mock_set_error(dev_err_t error);
void dev_gpio_port_mock_clear_error(void);
void dev_gpio_port_mock_trigger_isr(dev_gpio_pin_t pin);

dev_gpio_level_t dev_gpio_port_mock_get_level(dev_gpio_pin_t pin);
bool             dev_gpio_port_mock_is_output(dev_gpio_pin_t pin);
bool             dev_gpio_port_mock_is_interrupt_enabled(dev_gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_MOCK_H */
