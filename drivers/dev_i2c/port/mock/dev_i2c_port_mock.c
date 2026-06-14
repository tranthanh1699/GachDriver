#include "dev_i2c_port_mock.h"
#include "dev_compiler.h"
#include <string.h>

#define MOCK_MEM_MAX       (256U)
#define MOCK_MAX_DEVICES    (4U)

typedef struct {
    dev_i2c_addr_t addr;
    uint8_t        mem[MOCK_MEM_MAX];
    uint16_t       mem_size;
    bool           attached;
} mock_dev_t;

static mock_dev_t s_devs[DEV_I2C_CFG_MAX_BUSES][MOCK_MAX_DEVICES];
static dev_err_t s_error = DEV_OK;

static mock_dev_t *find_dev(dev_i2c_bus_t bus, dev_i2c_addr_t addr)
{
    if (bus >= DEV_I2C_CFG_MAX_BUSES) return NULL;
    for (uint8_t i = 0U; i < MOCK_MAX_DEVICES; i++)
        if (s_devs[bus][i].attached && s_devs[bus][i].addr == addr)
            return &s_devs[bus][i];
    return NULL;
}

void dev_i2c_port_mock_reset(void)
{
    s_error = DEV_OK;
    for (uint8_t b = 0U; b < DEV_I2C_CFG_MAX_BUSES; b++)
        for (uint8_t i = 0U; i < MOCK_MAX_DEVICES; i++)
            s_devs[b][i].attached = false;
}

void dev_i2c_port_mock_set_error(dev_err_t e)       { s_error = e; }
void dev_i2c_port_mock_clear_error(void)             { s_error = DEV_OK; }

void dev_i2c_port_mock_attach_device(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                     uint8_t *mem, uint16_t mem_size)
{
    if (bus >= DEV_I2C_CFG_MAX_BUSES) return;
    for (uint8_t i = 0U; i < MOCK_MAX_DEVICES; i++) {
        if (!s_devs[bus][i].attached) {
            s_devs[bus][i].addr     = addr;
            s_devs[bus][i].mem_size = (mem_size > MOCK_MEM_MAX) ? MOCK_MEM_MAX : mem_size;
            s_devs[bus][i].attached = true;
            if (mem != NULL) memcpy(s_devs[bus][i].mem, mem, s_devs[bus][i].mem_size);
            return;
        }
    }
}

void dev_i2c_port_mock_detach_device(dev_i2c_bus_t bus, dev_i2c_addr_t addr)
{
    mock_dev_t *d = find_dev(bus, addr);
    if (d != NULL) d->attached = false;
}

/* ── Port API ── */

dev_err_t dev_i2c_port_init(void)                          { return s_error; }
dev_err_t dev_i2c_port_deinit(void)                        { return s_error; }
dev_err_t dev_i2c_port_set_speed(dev_i2c_bus_t b, dev_i2c_speed_t s)
    { DEV_UNUSED(b); if (s == DEV_I2C_SPEED_HIGH) return DEV_ERR_NOT_SUPPORTED; return s_error; }

dev_err_t dev_i2c_port_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                             const uint8_t *data, uint16_t len, dev_i2c_timeout_t to)
{
    DEV_UNUSED(to);
    if (s_error != DEV_OK) return s_error;
    mock_dev_t *d = find_dev(bus, addr);
    if (d == NULL) return DEV_ERR_NO_ACK;
    if (len > 0U) memcpy(d->mem, data, (len > d->mem_size) ? d->mem_size : len);
    return DEV_OK;
}

dev_err_t dev_i2c_port_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                            uint8_t *data, uint16_t len, dev_i2c_timeout_t to)
{
    DEV_UNUSED(to);
    if (s_error != DEV_OK) return s_error;
    mock_dev_t *d = find_dev(bus, addr);
    if (d == NULL) return DEV_ERR_NO_ACK;
    if (len > 0U) memcpy(data, d->mem, (len > d->mem_size) ? d->mem_size : len);
    return DEV_OK;
}

dev_err_t dev_i2c_port_write_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                  const uint8_t *wd, uint16_t wl,
                                  uint8_t *rd, uint16_t rl, dev_i2c_timeout_t to)
{
    dev_err_t e = dev_i2c_port_write(bus, addr, wd, wl, to);
    if (e != DEV_OK) return e;
    return dev_i2c_port_read(bus, addr, rd, rl, to);
}

dev_err_t dev_i2c_port_mem_write(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                 uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_size,
                                 const uint8_t *data, uint16_t len, dev_i2c_timeout_t to)
{
    DEV_UNUSED(mem_size);
    if (s_error != DEV_OK) return s_error;
    mock_dev_t *d = find_dev(bus, addr);
    if (d == NULL) return DEV_ERR_NO_ACK;
    if (mem_addr >= d->mem_size) return DEV_ERR_OUT_OF_RANGE;
    uint16_t cap = (uint16_t)(d->mem_size - mem_addr);
    if (len > cap) len = cap;
    memcpy(&d->mem[mem_addr], data, len);
    DEV_UNUSED(to); return DEV_OK;
}

dev_err_t dev_i2c_port_mem_read(dev_i2c_bus_t bus, dev_i2c_addr_t addr,
                                uint16_t mem_addr, dev_i2c_mem_addr_size_t mem_size,
                                uint8_t *data, uint16_t len, dev_i2c_timeout_t to)
{
    DEV_UNUSED(mem_size);
    if (s_error != DEV_OK) return s_error;
    mock_dev_t *d = find_dev(bus, addr);
    if (d == NULL) return DEV_ERR_NO_ACK;
    if (mem_addr >= d->mem_size) return DEV_ERR_OUT_OF_RANGE;
    uint16_t cap = (uint16_t)(d->mem_size - mem_addr);
    if (len > cap) len = cap;
    memcpy(data, &d->mem[mem_addr], len);
    DEV_UNUSED(to); return DEV_OK;
}

dev_err_t dev_i2c_port_probe(dev_i2c_bus_t bus, dev_i2c_addr_t addr, dev_i2c_timeout_t to)
{
    DEV_UNUSED(to);
    if (s_error != DEV_OK) return s_error;
    return (find_dev(bus, addr) != NULL) ? DEV_OK : DEV_ERR_NO_ACK;
}

dev_err_t dev_i2c_port_recover_bus(dev_i2c_bus_t bus)
    { DEV_UNUSED(bus); return DEV_ERR_NOT_SUPPORTED; }
