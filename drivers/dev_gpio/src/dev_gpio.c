#include "dev_gpio.h"
#include "dev_gpio_port.h"
#include "dev_common.h"
#include <stddef.h>

/* ── Module state ── */

static bool g_initialized = false;
static const dev_gpio_config_t *g_config = NULL;

/* ── Callback tables (owned by common driver, indexed by config index) ── */

static dev_gpio_isr_callback_t g_callbacks[DEV_GPIO_CFG_MAX_CHANNELS];
static void                   *g_callback_args[DEV_GPIO_CFG_MAX_CHANNELS];
static bool                    g_interrupt_enabled[DEV_GPIO_CFG_MAX_CHANNELS];

/* ── Channel ID → config index lookup ── */

static uint16_t dev_gpio_find_index(dev_gpio_channel_t channel)
{
    uint16_t i;
    if (g_config == NULL) {
        return 0U;
    }
    for (i = 0U; i < g_config->channel_count; i++) {
        if (g_config->channels[i].channel == channel) {
            return i;
        }
    }
    return g_config->channel_count;
}

/* ── Enum validation helpers ── */

static bool dev_gpio_is_valid_direction(dev_gpio_direction_t direction)
{
    return (direction == DEV_GPIO_DIRECTION_INPUT)  ||
           (direction == DEV_GPIO_DIRECTION_OUTPUT) ||
           (direction == DEV_GPIO_DIRECTION_INPUT_OUTPUT);
}

static bool dev_gpio_is_valid_pull(dev_gpio_pull_t pull)
{
    return (pull == DEV_GPIO_PULL_NONE) ||
           (pull == DEV_GPIO_PULL_UP)   ||
           (pull == DEV_GPIO_PULL_DOWN);
}

static bool dev_gpio_is_valid_interrupt(dev_gpio_intr_type_t interrupt)
{
    return (interrupt == DEV_GPIO_INTR_DISABLE)      ||
           (interrupt == DEV_GPIO_INTR_RISING_EDGE)  ||
           (interrupt == DEV_GPIO_INTR_FALLING_EDGE) ||
           (interrupt == DEV_GPIO_INTR_BOTH_EDGES)   ||
           (interrupt == DEV_GPIO_INTR_LOW_LEVEL)    ||
           (interrupt == DEV_GPIO_INTR_HIGH_LEVEL);
}

static bool dev_gpio_is_valid_level(dev_gpio_level_t level)
{
    return (level == DEV_GPIO_LEVEL_LOW) || (level == DEV_GPIO_LEVEL_HIGH);
}

/* ── Duplicate detection ── */

#if (DEV_GPIO_CFG_VALIDATE_DUPLICATES == 1U)
static bool dev_gpio_has_duplicates(void)
{
    uint16_t i;
    uint16_t j;

    for (i = 0U; i < g_config->channel_count; i++) {
        for (j = i + 1U; j < g_config->channel_count; j++) {
            if (g_config->channels[i].channel == g_config->channels[j].channel) {
                return true;
            }
        }
    }
    return false;
}
#endif /* DEV_GPIO_CFG_VALIDATE_DUPLICATES */

/* ── Port error mapping ── */

static dev_err_t dev_gpio_map_port_error(dev_err_t port_err)
{
    if (port_err == DEV_OK) {
        return DEV_OK;
    }
    if (port_err == DEV_ERR_NOT_SUPPORTED) {
        return DEV_ERR_NOT_SUPPORTED;
    }
    return DEV_ERR_HW_FAILURE;
}

/* ── Public API ── */

dev_err_t dev_gpio_init(const dev_gpio_config_t *config)
{
    uint16_t i;
    dev_err_t err;

    /* Step 1: check already initialized */
    if (g_initialized) {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    /* Step 2-3: validate pointers */
    if (config == NULL) {
        return DEV_ERR_NULL_PTR;
    }
    if (config->channels == NULL) {
        return DEV_ERR_NULL_PTR;
    }

    /* Step 4-5: validate channel_count */
    if (config->channel_count == 0U) {
        return DEV_ERR_INVALID_ARG;
    }
    if (config->channel_count > DEV_GPIO_CFG_MAX_CHANNELS) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Steps 6-9: validate each channel's enum fields + interrupt compiled-out check */
    for (i = 0U; i < config->channel_count; i++) {
        if (!dev_gpio_is_valid_direction(config->channels[i].direction)) {
            return DEV_ERR_INVALID_ARG;
        }
        if (!dev_gpio_is_valid_pull(config->channels[i].pull)) {
            return DEV_ERR_INVALID_ARG;
        }
        if (!dev_gpio_is_valid_interrupt(config->channels[i].interrupt)) {
            return DEV_ERR_INVALID_ARG;
        }
#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U)
        if (config->channels[i].interrupt != DEV_GPIO_INTR_DISABLE) {
            return DEV_ERR_NOT_SUPPORTED;
        }
#endif
    }

    /* Step 10: duplicate detection */
#if (DEV_GPIO_CFG_VALIDATE_DUPLICATES == 1U)
    g_config = config; /* temporarily set so dev_gpio_has_duplicates can scan */
    if (dev_gpio_has_duplicates()) {
        g_config = NULL;
        return DEV_ERR_CONFIG;
    }
#endif

    g_config = config;

    /* Step 11: port init */
    err = dev_gpio_port_init(config);
    if (err != DEV_OK) {
        g_config = NULL;
        return dev_gpio_map_port_error(err);
    }

    /* Step 12-13: configure each channel, cleanup on failure */
    for (i = 0U; i < config->channel_count; i++) {
        err = dev_gpio_port_config_channel(&config->channels[i]);
        if (err != DEV_OK) {
            (void)dev_gpio_port_deinit();
            g_config = NULL;
            return dev_gpio_map_port_error(err);
        }
    }

    /* Step 14: initialize callback tables */
    for (i = 0U; i < config->channel_count; i++) {
        g_callbacks[i]         = config->channels[i].callback;
        g_callback_args[i]     = config->channels[i].callback_arg;
        g_interrupt_enabled[i] = false;
    }

    /* Step 15-16: success */
    g_initialized = true;
    return DEV_OK;
}

dev_err_t dev_gpio_deinit(void)
{
    uint16_t i;
    dev_err_t port_err;
    dev_err_t result = DEV_OK;

    /* Step 1 */
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Step 2: best-effort disable all interrupts */
    for (i = 0U; i < g_config->channel_count; i++) {
        port_err = dev_gpio_port_disable_interrupt(g_config->channels[i].channel);
        if (port_err != DEV_OK) {
            dev_assert_report(__FILE__, (uint32_t)__LINE__,
                              DEV_ASSERT_TYPE_ERROR, port_err);
        }
    }

    /* Step 3-4: clear callbacks and interrupt state */
    for (i = 0U; i < DEV_GPIO_CFG_MAX_CHANNELS; i++) {
        g_callbacks[i]         = NULL;
        g_callback_args[i]     = NULL;
        g_interrupt_enabled[i] = false;
    }

    /* Step 5-6-7-8: port deinit, force UNINITIALIZED */
    port_err = dev_gpio_port_deinit();
    g_config      = NULL;
    g_initialized = false;

    if (port_err != DEV_OK) {
        result = dev_gpio_map_port_error(port_err);
    }

    return result;
}

dev_err_t dev_gpio_read(dev_gpio_channel_t channel,
                        dev_gpio_level_t *level)
{
    dev_gpio_level_t temp;
    uint16_t idx;
    dev_err_t err;

    /* Step 1 */
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Step 2 */
    if (level == NULL) {
        return DEV_ERR_NULL_PTR;
    }

    /* Step 3 */
    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 4-5: read into local temp, write *level only on success */
    err = dev_gpio_port_read(channel, &temp);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    *level = temp;
    return DEV_OK;
}

dev_err_t dev_gpio_write(dev_gpio_channel_t channel,
                         dev_gpio_level_t level)
{
    uint16_t idx;
    dev_err_t err;

    /* Step 1 */
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Step 2 */
    if (!dev_gpio_is_valid_level(level)) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 3 */
    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 4 */
    if (g_config->channels[idx].direction == DEV_GPIO_DIRECTION_INPUT) {
        return DEV_ERR_INVALID_STATE;
    }

    /* Step 5-6 */
    err = dev_gpio_port_write(channel, level);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_toggle(dev_gpio_channel_t channel)
{
    uint16_t idx;
    dev_err_t err;

    /* Step 1 */
    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Step 2 */
    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Step 3 */
    if (g_config->channels[idx].direction == DEV_GPIO_DIRECTION_INPUT) {
        return DEV_ERR_INVALID_STATE;
    }

    /* Step 4-5 */
    err = dev_gpio_port_toggle(channel);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_set_direction(dev_gpio_channel_t channel,
                                 dev_gpio_direction_t direction)
{
    uint16_t idx;
    dev_err_t err;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (!dev_gpio_is_valid_direction(direction)) {
        return DEV_ERR_INVALID_ARG;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    err = dev_gpio_port_set_direction(channel, direction);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    /* Update internal direction tracking */
    ((dev_gpio_channel_config_t *)&g_config->channels[idx])->direction = direction;

    return DEV_OK;
}

dev_err_t dev_gpio_set_pull(dev_gpio_channel_t channel,
                            dev_gpio_pull_t pull)
{
    uint16_t idx;
    dev_err_t err;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (!dev_gpio_is_valid_pull(pull)) {
        return DEV_ERR_INVALID_ARG;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    err = dev_gpio_port_set_pull(channel, pull);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

#if (DEV_GPIO_CFG_INTERRUPT_ENABLED == 1U)

dev_err_t dev_gpio_config_interrupt(dev_gpio_channel_t channel,
                                    dev_gpio_intr_type_t interrupt)
{
    uint16_t idx;
    dev_err_t err;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (!dev_gpio_is_valid_interrupt(interrupt)) {
        return DEV_ERR_INVALID_ARG;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    err = dev_gpio_port_config_interrupt(channel, interrupt);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_register_callback(dev_gpio_channel_t channel,
                                     dev_gpio_isr_callback_t callback,
                                     void *user_arg)
{
    uint16_t idx;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    g_callbacks[idx]     = callback;
    g_callback_args[idx] = user_arg;

    return DEV_OK;
}

dev_err_t dev_gpio_enable_interrupt(dev_gpio_channel_t channel)
{
    uint16_t idx;
    dev_err_t err;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Mark enabled BEFORE port call to avoid dropping immediate IRQs */
    g_interrupt_enabled[idx] = true;

    err = dev_gpio_port_enable_interrupt(channel);
    if (err != DEV_OK) {
        /* Roll back on failure */
        g_interrupt_enabled[idx] = false;
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

dev_err_t dev_gpio_disable_interrupt(dev_gpio_channel_t channel)
{
    uint16_t idx;
    dev_err_t err;

    if (!g_initialized) {
        return DEV_ERR_NOT_INITIALIZED;
    }

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return DEV_ERR_INVALID_ARG;
    }

    /* Mark disabled BEFORE port call */
    g_interrupt_enabled[idx] = false;

    err = dev_gpio_port_disable_interrupt(channel);
    if (err != DEV_OK) {
        return dev_gpio_map_port_error(err);
    }

    return DEV_OK;
}

void dev_gpio_dispatch_isr(dev_gpio_channel_t channel)
{
    uint16_t idx;
    dev_gpio_isr_callback_t cb;
    void *arg;

    idx = dev_gpio_find_index(channel);
    if (idx >= g_config->channel_count) {
        return;
    }

    if (!g_interrupt_enabled[idx]) {
        return;
    }

    cb  = g_callbacks[idx];
    arg = g_callback_args[idx];

    if (cb != NULL) {
        cb(channel, arg);
    }
}

#else /* DEV_GPIO_CFG_INTERRUPT_ENABLED == 0U */

dev_err_t dev_gpio_config_interrupt(dev_gpio_channel_t channel,
                                    dev_gpio_intr_type_t interrupt)
{
    DEV_UNUSED(channel);
    DEV_UNUSED(interrupt);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_register_callback(dev_gpio_channel_t channel,
                                     dev_gpio_isr_callback_t callback,
                                     void *user_arg)
{
    DEV_UNUSED(channel);
    DEV_UNUSED(callback);
    DEV_UNUSED(user_arg);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_enable_interrupt(dev_gpio_channel_t channel)
{
    DEV_UNUSED(channel);
    return DEV_ERR_NOT_SUPPORTED;
}

dev_err_t dev_gpio_disable_interrupt(dev_gpio_channel_t channel)
{
    DEV_UNUSED(channel);
    return DEV_ERR_NOT_SUPPORTED;
}

void dev_gpio_dispatch_isr(dev_gpio_channel_t channel)
{
    DEV_UNUSED(channel);
    /* No-op when interrupts are compiled out */
}

#endif /* DEV_GPIO_CFG_INTERRUPT_ENABLED */

bool dev_gpio_is_initialized(void)
{
    return g_initialized;
}
