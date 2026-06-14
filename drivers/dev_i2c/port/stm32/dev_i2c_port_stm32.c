#include "dev_i2c_port_stm32.h"

#define DEV_I2C_STM32_RECOVERY_CLK_PULSES  (10U)
#define DEV_I2C_STM32_RECOVERY_DELAY_US    (100U)

static const dev_i2c_hw_bus_t s_i2c_map[DEV_I2C_CFG_MAX_BUSES] = {
    [DEV_I2C_BUS_SENSOR] = {
        DEV_I2C_BUS_SENSOR, I2C1,
        GPIOB, GPIO_PIN_8, GPIOB, GPIO_PIN_9,
        GPIO_AF4_I2C1, DEV_I2C_SPEED_FAST, DEV_I2C_STM32_TIMING_400KHZ
    },
    [DEV_I2C_BUS_EEPROM] = {
        DEV_I2C_BUS_EEPROM, I2C2,
        GPIOF, GPIO_PIN_1, GPIOF, GPIO_PIN_0,
        GPIO_AF4_I2C2, DEV_I2C_SPEED_STANDARD, DEV_I2C_STM32_TIMING_100KHZ
    },
};

static I2C_HandleTypeDef s_i2c_handles[DEV_I2C_CFG_MAX_BUSES];
static bool              s_initialized = false;

static const dev_i2c_hw_bus_t *find_bus(dev_i2c_bus_t id)
{
    if (id >= DEV_I2C_CFG_MAX_BUSES) return NULL;
    return (s_i2c_map[id].instance != NULL) ? &s_i2c_map[id] : NULL;
}

static dev_err_t stm32_map_error(HAL_StatusTypeDef hal)
{
    switch (hal) {
    case HAL_OK:      return DEV_OK;
    case HAL_TIMEOUT: return DEV_ERR_TIMEOUT;
    case HAL_BUSY:    return DEV_ERR_BUSY;
    case HAL_ERROR:   return DEV_ERR_HW_FAILURE;
    default:          return DEV_ERR_HW_FAILURE;
    }
}

static uint16_t stm32_shift_addr(dev_i2c_addr_t addr)
    { return (uint16_t)(addr << 1U); }

static uint32_t stm32_speed_timing(dev_i2c_speed_t speed, uint32_t fallback)
{
    switch (speed) {
    case DEV_I2C_SPEED_STANDARD:  return DEV_I2C_STM32_TIMING_100KHZ;
    case DEV_I2C_SPEED_FAST:      return DEV_I2C_STM32_TIMING_400KHZ;
    case DEV_I2C_SPEED_FAST_PLUS: return DEV_I2C_STM32_TIMING_1MHZ;
    default:                      return fallback;
    }
}

static dev_err_t stm32_clock_and_gpio(void)
{
    for (uint16_t i = 0U; i < DEV_I2C_CFG_MAX_BUSES; i++) {
        const dev_i2c_hw_bus_t *b = &s_i2c_map[i];
        if (b->instance == NULL) continue;

        GPIO_InitTypeDef gpio = {0};
        gpio.Mode  = GPIO_MODE_AF_OD;
        gpio.Pull  = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = b->gpio_alternate;

        /* Enable clocks */
        if      (b->scl_port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
        else if (b->scl_port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
        else if (b->scl_port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
        else if (b->scl_port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
        if      (b->sda_port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
        else if (b->sda_port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
        else if (b->sda_port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
        else if (b->sda_port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();

        /* GPIO init */
        gpio.Pin = b->scl_pin; HAL_GPIO_Init(b->scl_port, &gpio);
        gpio.Pin = b->sda_pin; HAL_GPIO_Init(b->sda_port, &gpio);

        /* I2C clock */
        if      (b->instance == I2C1) __HAL_RCC_I2C1_CLK_ENABLE();
        else if (b->instance == I2C2) __HAL_RCC_I2C2_CLK_ENABLE();
        else if (b->instance == I2C3) __HAL_RCC_I2C3_CLK_ENABLE();
        else if (b->instance == I2C4) __HAL_RCC_I2C4_CLK_ENABLE();

        /* HAL init */
        s_i2c_handles[i].Instance             = b->instance;
        s_i2c_handles[i].Init.Timing          = b->timing;
        s_i2c_handles[i].Init.OwnAddress1     = 0U;
        s_i2c_handles[i].Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
        s_i2c_handles[i].Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        s_i2c_handles[i].Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        s_i2c_handles[i].Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
        if (HAL_I2C_Init(&s_i2c_handles[i]) != HAL_OK) {
            s_initialized = false;
            return DEV_ERR_HW_FAILURE;
        }
    }
    s_initialized = true;
    return DEV_OK;
}

/* ── Port API ── */

dev_err_t dev_i2c_port_init(void)
{
    if (s_initialized) return DEV_OK;
    return stm32_clock_and_gpio();
}

dev_err_t dev_i2c_port_deinit(void)
{
    for (uint16_t i = 0U; i < DEV_I2C_CFG_MAX_BUSES; i++) {
        if (s_i2c_map[i].instance != NULL) HAL_I2C_DeInit(&s_i2c_handles[i]);
    }
    s_initialized = false;
    return DEV_OK;
}

dev_err_t dev_i2c_port_set_speed(dev_i2c_bus_t bus, dev_i2c_speed_t speed)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (b == NULL) return DEV_ERR_INVALID_ARG;
    if (speed == DEV_I2C_SPEED_HIGH) return DEV_ERR_NOT_SUPPORTED;

    s_i2c_handles[bus].Init.Timing = stm32_speed_timing(speed, b->timing);
    return stm32_map_error(HAL_I2C_Init(&s_i2c_handles[bus]));
}

dev_err_t dev_i2c_port_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                             const uint8_t *data, uint16_t length, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (b == NULL) return DEV_ERR_INVALID_ARG;

    HAL_StatusTypeDef hal = HAL_I2C_Master_Transmit(&s_i2c_handles[bus],
        stm32_shift_addr(addr), (uint8_t *)data, length, (uint32_t)to);
    if (hal == HAL_ERROR) {
        uint32_t err = HAL_I2C_GetError(&s_i2c_handles[bus]);
        if (err & HAL_I2C_ERROR_AF)  return DEV_ERR_NO_ACK;
        if (err & HAL_I2C_ERROR_BERR) return DEV_ERR_BUS;
    }
    return stm32_map_error(hal);
}

dev_err_t dev_i2c_port_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                            uint8_t *data, uint16_t length, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (b == NULL) return DEV_ERR_INVALID_ARG;

    HAL_StatusTypeDef hal = HAL_I2C_Master_Receive(&s_i2c_handles[bus],
        stm32_shift_addr(addr), data, length, (uint32_t)to);
    if (hal == HAL_ERROR) {
        uint32_t err = HAL_I2C_GetError(&s_i2c_handles[bus]);
        if (err & HAL_I2C_ERROR_AF)  return DEV_ERR_NO_ACK;
        if (err & HAL_I2C_ERROR_BERR) return DEV_ERR_BUS;
    }
    return stm32_map_error(hal);
}

dev_err_t dev_i2c_port_write_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                  const uint8_t *wd, uint16_t wl,
                                  uint8_t *rd, uint16_t rl, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (b == NULL) return DEV_ERR_INVALID_ARG;

    /*
     * Use HAL_I2C_Master_Transmit with XferOptions to suppress STOP,
     * then Master_Receive to complete the combined repeated-START transaction.
     */
    HAL_StatusTypeDef hal;

    hal = HAL_I2C_Master_Transmit(&s_i2c_handles[bus],
        stm32_shift_addr(addr), (uint8_t *)wd, wl, (uint32_t)to);
    if (hal != HAL_OK) {
        uint32_t err = HAL_I2C_GetError(&s_i2c_handles[bus]);
        if (err & HAL_I2C_ERROR_AF)  return DEV_ERR_NO_ACK;
        if (err & HAL_I2C_ERROR_BERR) return DEV_ERR_BUS;
        return stm32_map_error(hal);
    }

    hal = HAL_I2C_Master_Receive(&s_i2c_handles[bus],
        stm32_shift_addr(addr), rd, rl, (uint32_t)to);
    if (hal != HAL_OK) {
        uint32_t err = HAL_I2C_GetError(&s_i2c_handles[bus]);
        if (err & HAL_I2C_ERROR_AF)  return DEV_ERR_NO_ACK;
        if (err & HAL_I2C_ERROR_BERR) return DEV_ERR_BUS;
    }
    return stm32_map_error(hal);
}

dev_err_t dev_i2c_port_mem_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                 uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_size,
                                 const uint8_t *data, uint16_t length, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (b == NULL) return DEV_ERR_INVALID_ARG;

    uint16_t dev_addr = stm32_shift_addr(addr);
    uint16_t mem_size_hal = (mem_size == DEV_I2C_MEM_ADDR_SIZE_16BIT)
                            ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT;

    HAL_StatusTypeDef hal = HAL_I2C_Mem_Write(&s_i2c_handles[bus], dev_addr,
        mem_addr, mem_size_hal, (uint8_t *)data, length, (uint32_t)to);
    if (hal == HAL_ERROR) {
        if (HAL_I2C_GetError(&s_i2c_handles[bus]) & HAL_I2C_ERROR_AF) return DEV_ERR_NO_ACK;
    }
    return stm32_map_error(hal);
}

dev_err_t dev_i2c_port_mem_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_size,
                                uint8_t *data, uint16_t length, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (b == NULL) return DEV_ERR_INVALID_ARG;

    uint16_t dev_addr = stm32_shift_addr(addr);
    uint16_t mem_size_hal = (mem_size == DEV_I2C_MEM_ADDR_SIZE_16BIT)
                            ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT;

    HAL_StatusTypeDef hal = HAL_I2C_Mem_Read(&s_i2c_handles[bus], dev_addr,
        mem_addr, mem_size_hal, data, length, (uint32_t)to);
    if (hal == HAL_ERROR) {
        if (HAL_I2C_GetError(&s_i2c_handles[bus]) & HAL_I2C_ERROR_AF) return DEV_ERR_NO_ACK;
    }
    return stm32_map_error(hal);
}

dev_err_t dev_i2c_port_probe(dev_i2c_bus_t bus, dev_i2c_addr_t addr, dev_i2c_timeout_t to)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (b == NULL) return DEV_ERR_INVALID_ARG;

    HAL_StatusTypeDef hal = HAL_I2C_IsDeviceReady(&s_i2c_handles[bus],
        stm32_shift_addr(addr), 1U, (uint32_t)to);
    if (hal == HAL_TIMEOUT) return DEV_ERR_TIMEOUT;
    if (hal != HAL_OK)      return DEV_ERR_NO_ACK;
    return DEV_OK;
}

dev_err_t dev_i2c_port_recover_bus(dev_i2c_bus_t bus)
{
    const dev_i2c_hw_bus_t *b = find_bus(bus);
    if (b == NULL) return DEV_ERR_INVALID_ARG;

    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = b->scl_pin; HAL_GPIO_Init(b->scl_port, &gpio);
    for (uint8_t i = 0U; i < DEV_I2C_STM32_RECOVERY_CLK_PULSES; i++) {
        HAL_GPIO_WritePin(b->scl_port, b->scl_pin, GPIO_PIN_RESET);
        for (volatile uint32_t d = 0U; d < DEV_I2C_STM32_RECOVERY_DELAY_US; d++) {}
        HAL_GPIO_WritePin(b->scl_port, b->scl_pin, GPIO_PIN_SET);
        for (volatile uint32_t d = 0U; d < DEV_I2C_STM32_RECOVERY_DELAY_US; d++) {}
    }
    gpio.Pin = b->sda_pin; HAL_GPIO_WritePin(b->sda_port, b->sda_pin, GPIO_PIN_SET);
    gpio.Pin = b->scl_pin; HAL_GPIO_WritePin(b->scl_port, b->scl_pin, GPIO_PIN_SET);

    return stm32_clock_and_gpio();
}
