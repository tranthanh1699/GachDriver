#ifndef SVC_EEP_H
#define SVC_EEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_eep_types.h"
#include "svc_eep_cfg.h"
#include "svc_eep_layout.h"
#include "dev_error.h"

/* ── Lifecycle ── */

dev_err_t svc_eep_init(void);
dev_err_t svc_eep_shutdown(void);
dev_err_t svc_eep_deinit(void);
bool      svc_eep_is_initialized(void);

/* ── Raw read/write (RAM mirror) ── */

dev_err_t svc_eep_read(svc_eep_id_t eep_id,
                       svc_eep_addr_t addr,
                       uint8_t *data,
                       svc_eep_size_t length);

dev_err_t svc_eep_write(svc_eep_id_t eep_id,
                        svc_eep_addr_t addr,
                        const uint8_t *data,
                        svc_eep_size_t length);

dev_err_t svc_eep_read_all(svc_eep_id_t eep_id);

dev_err_t svc_eep_write_all(svc_eep_id_t eep_id);

dev_err_t svc_eep_flush(svc_eep_id_t eep_id);

/* ── Field-based read/write ── */

dev_err_t svc_eep_read_field(svc_eep_field_id_t field_id,
                             void *data,
                             svc_eep_size_t length);

dev_err_t svc_eep_write_field(svc_eep_field_id_t field_id,
                              const void *data,
                              svc_eep_size_t length);

dev_err_t svc_eep_get_field_info(svc_eep_field_id_t field_id,
                                 const svc_eep_field_t **field);

/* ── Typed read/write ── */

dev_err_t svc_eep_read_u8(svc_eep_field_id_t field_id, uint8_t *value);
dev_err_t svc_eep_write_u8(svc_eep_field_id_t field_id, uint8_t value);

dev_err_t svc_eep_read_u16(svc_eep_field_id_t field_id, uint16_t *value);
dev_err_t svc_eep_write_u16(svc_eep_field_id_t field_id, uint16_t value);

dev_err_t svc_eep_read_u32(svc_eep_field_id_t field_id, uint32_t *value);
dev_err_t svc_eep_write_u32(svc_eep_field_id_t field_id, uint32_t value);

/* ── Dirty state ── */

bool      svc_eep_is_dirty(svc_eep_id_t eep_id);
dev_err_t svc_eep_mark_dirty(svc_eep_id_t eep_id,
                             svc_eep_addr_t addr,
                             svc_eep_size_t length);
dev_err_t svc_eep_clear_dirty(svc_eep_id_t eep_id);
uint16_t  svc_eep_get_dirty_page_count(svc_eep_id_t eep_id);

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_H */
