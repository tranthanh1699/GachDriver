#ifndef DEV_EEP_H
#define DEV_EEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_eep_types.h"
#include "dev_eep_cfg.h"
#include "dev_eep_layout.h"
#include "dev_error.h"

/* ── Lifecycle ── */

dev_err_t dev_eep_init(void);
dev_err_t dev_eep_shutdown(void);
dev_err_t dev_eep_deinit(void);
bool      dev_eep_is_initialized(void);

/* ── Raw read/write (RAM mirror) ── */

dev_err_t dev_eep_read(dev_eep_id_t eep_id,
                       dev_eep_addr_t addr,
                       uint8_t *data,
                       dev_eep_size_t length);

dev_err_t dev_eep_write(dev_eep_id_t eep_id,
                        dev_eep_addr_t addr,
                        const uint8_t *data,
                        dev_eep_size_t length);

dev_err_t dev_eep_read_all(dev_eep_id_t eep_id);

dev_err_t dev_eep_write_all(dev_eep_id_t eep_id);

dev_err_t dev_eep_flush(dev_eep_id_t eep_id);

/* ── Field-based read/write ── */

dev_err_t dev_eep_read_field(dev_eep_field_id_t field_id,
                             void *data,
                             dev_eep_size_t length);

dev_err_t dev_eep_write_field(dev_eep_field_id_t field_id,
                              const void *data,
                              dev_eep_size_t length);

dev_err_t dev_eep_get_field_info(dev_eep_field_id_t field_id,
                                 const dev_eep_field_t **field);

/* ── Typed read/write ── */

dev_err_t dev_eep_read_u8(dev_eep_field_id_t field_id, uint8_t *value);
dev_err_t dev_eep_write_u8(dev_eep_field_id_t field_id, uint8_t value);

dev_err_t dev_eep_read_u16(dev_eep_field_id_t field_id, uint16_t *value);
dev_err_t dev_eep_write_u16(dev_eep_field_id_t field_id, uint16_t value);

dev_err_t dev_eep_read_u32(dev_eep_field_id_t field_id, uint32_t *value);
dev_err_t dev_eep_write_u32(dev_eep_field_id_t field_id, uint32_t value);

/* ── Dirty state ── */

bool      dev_eep_is_dirty(dev_eep_id_t eep_id);
dev_err_t dev_eep_mark_dirty(dev_eep_id_t eep_id,
                             dev_eep_addr_t addr,
                             dev_eep_size_t length);
dev_err_t dev_eep_clear_dirty(dev_eep_id_t eep_id);
uint16_t  dev_eep_get_dirty_page_count(dev_eep_id_t eep_id);

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_H */
