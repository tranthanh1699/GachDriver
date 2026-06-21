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

/**
 * @brief Initialize the EEPROM service.
 *
 * Initializes dev_eep, reads all EEPROM data into the RAM mirror,
 * validates magic/version/CRC, and loads defaults if needed.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_ALREADY_INITIALIZED if already initialized.
 * @return DEV_ERR_CONFIG if device configuration is invalid.
 * @return DEV_ERR_TIMEOUT if EEPROM does not respond.
 */
dev_err_t svc_eep_init(void);

/**
 * @brief Shutdown the EEPROM service.
 *
 * Flushes dirty pages to EEPROM (if auto-flush is enabled),
 * updates CRC, and clears initialized state.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_shutdown(void);

/**
 * @brief Deinitialize the EEPROM service without flushing.
 *
 * Clears RAM mirror and dirty map. Does not write to EEPROM.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_deinit(void);

/**
 * @brief Check if the EEPROM service is initialized.
 *
 * @return true if initialized, false otherwise.
 */
bool svc_eep_is_initialized(void);

/* ── Field-based read/write ── */

/**
 * @brief Read from a named field in the RAM mirror.
 *
 * @param field_id Field identifier.
 * @param data     Output buffer.
 * @param length   Number of bytes to read (must be <= field size).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if field_id is invalid or length > field size.
 * @return DEV_ERR_NULL_PTR if data is NULL.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_read_field(svc_eep_field_id_t field_id,
                             void *data,
                             svc_eep_size_t length);

/**
 * @brief Write to a named field in the RAM mirror.
 *
 * Marks affected pages dirty. Does not write to physical EEPROM
 * until flush() or shutdown() is called.
 *
 * @param field_id Field identifier.
 * @param data     Data to write.
 * @param length   Number of bytes to write (must be <= field size).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if field_id is invalid or length > field size.
 * @return DEV_ERR_NULL_PTR if data is NULL.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_write_field(svc_eep_field_id_t field_id,
                              const void *data,
                              svc_eep_size_t length);

/**
 * @brief Get field descriptor by ID.
 *
 * @param field_id Field identifier.
 * @param field    Output pointer to field descriptor.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if field_id is not found.
 * @return DEV_ERR_NULL_PTR if field is NULL.
 */
dev_err_t svc_eep_get_field_info(svc_eep_field_id_t field_id,
                                 const svc_eep_field_t **field);

/* ── Typed read/write ── */

dev_err_t svc_eep_read_u8(svc_eep_field_id_t field_id, uint8_t *value);
dev_err_t svc_eep_write_u8(svc_eep_field_id_t field_id, uint8_t value);

dev_err_t svc_eep_read_u16(svc_eep_field_id_t field_id, uint16_t *value);
dev_err_t svc_eep_write_u16(svc_eep_field_id_t field_id, uint16_t value);

dev_err_t svc_eep_read_u32(svc_eep_field_id_t field_id, uint32_t *value);
dev_err_t svc_eep_write_u32(svc_eep_field_id_t field_id, uint32_t value);

/* ── Flush ── */

/**
 * @brief Write all dirty pages to physical EEPROM.
 *
 * Iterates over the dirty map and writes each dirty page via dev_eep_write().
 * Dirty bits are cleared only after successful writes.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_TIMEOUT if EEPROM write fails.
 */
dev_err_t svc_eep_flush(void);

/* ── Raw read/write (RAM mirror) ── */

/**
 * @brief Read raw bytes from the RAM mirror.
 *
 * Reads from RAM only — does not access the physical EEPROM.
 *
 * @param eep_id Logical EEPROM ID.
 * @param addr   Byte address within the mirror.
 * @param data   Output buffer.
 * @param length Number of bytes to read.
 *
 * @return DEV_OK on success.
 */
dev_err_t svc_eep_read(svc_eep_id_t eep_id,
                       svc_eep_addr_t addr,
                       uint8_t *data,
                       svc_eep_size_t length);

/**
 * @brief Write raw bytes to the RAM mirror.
 *
 * Writes to RAM only — does not access the physical EEPROM.
 * Marks affected pages dirty.
 *
 * @param eep_id Logical EEPROM ID.
 * @param addr   Byte address within the mirror.
 * @param data   Data to write.
 * @param length Number of bytes to write.
 *
 * @return DEV_OK on success.
 */
dev_err_t svc_eep_write(svc_eep_id_t eep_id,
                        svc_eep_addr_t addr,
                        const uint8_t *data,
                        svc_eep_size_t length);

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
