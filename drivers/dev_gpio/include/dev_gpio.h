#ifndef DEV_GPIO_H
#define DEV_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "dev_gpio_cfg.h"
#include "dev_error.h"

dev_err_t dev_gpio_init(void);
dev_err_t dev_gpio_deinit(void);
bool     dev_gpio_is_initialized(void);

dev_err_t dev_gpio_input(dev_gpio_pin_t pin);
dev_err_t dev_gpio_input_pullup(dev_gpio_pin_t pin);
dev_err_t dev_gpio_input_pulldown(dev_gpio_pin_t pin);
dev_err_t dev_gpio_output(dev_gpio_pin_t pin);
dev_err_t dev_gpio_output_level(dev_gpio_pin_t pin, dev_gpio_level_t level);
dev_err_t dev_gpio_read(dev_gpio_pin_t pin, dev_gpio_level_t *level);
dev_err_t dev_gpio_write(dev_gpio_pin_t pin, dev_gpio_level_t level);
dev_err_t dev_gpio_toggle(dev_gpio_pin_t pin);
dev_err_t dev_gpio_set_pull(dev_gpio_pin_t pin, dev_gpio_pull_t pull);
dev_err_t dev_gpio_high(dev_gpio_pin_t pin);
dev_err_t dev_gpio_low(dev_gpio_pin_t pin);

dev_err_t dev_gpio_interrupt(dev_gpio_pin_t pin, dev_gpio_intr_t intr,
                             dev_gpio_callback_t callback, void *user_arg);
dev_err_t dev_gpio_interrupt_enable(dev_gpio_pin_t pin);
dev_err_t dev_gpio_interrupt_disable(dev_gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_H */
