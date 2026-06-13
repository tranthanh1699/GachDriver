#ifndef DEV_GPIO_PORT_H
#define DEV_GPIO_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_gpio_types.h"
#include "dev_gpio_cfg.h"
#include "dev_error.h"

/* ── Port-implemented functions (called by common driver) ── */

dev_err_t dev_gpio_port_init(const dev_gpio_config_t *config);

dev_err_t dev_gpio_port_deinit(void);

dev_err_t dev_gpio_port_config_channel(const dev_gpio_channel_config_t *channel_config);

dev_err_t dev_gpio_port_read(dev_gpio_channel_t channel,
                             dev_gpio_level_t *level);

dev_err_t dev_gpio_port_write(dev_gpio_channel_t channel,
                              dev_gpio_level_t level);

dev_err_t dev_gpio_port_toggle(dev_gpio_channel_t channel);

dev_err_t dev_gpio_port_set_direction(dev_gpio_channel_t channel,
                                      dev_gpio_direction_t direction);

dev_err_t dev_gpio_port_set_pull(dev_gpio_channel_t channel,
                                 dev_gpio_pull_t pull);

dev_err_t dev_gpio_port_config_interrupt(dev_gpio_channel_t channel,
                                         dev_gpio_intr_type_t interrupt);

dev_err_t dev_gpio_port_enable_interrupt(dev_gpio_channel_t channel);

dev_err_t dev_gpio_port_disable_interrupt(dev_gpio_channel_t channel);

/* ── Common-driver service (called by port ISR handlers) ── */

/* PORT-ONLY: called from vendor ISR handlers. Do not call from application code. */
void dev_gpio_dispatch_isr(dev_gpio_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* DEV_GPIO_PORT_H */
