#ifndef SVC_EEP_H
#define SVC_EEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svc_eep_types.h"
#include "svc_eep_cfg.h"
#include "svc_eep_blocks.h"
#include "dev_error.h"

/* ── Lifecycle ── */

/**
 * @brief Initialize the EEPROM service.
 *
 * Validates the block configuration table, initializes dev_eep,
 * and sets up per-block runtime state. Does NOT read the entire
 * EEPROM. Blocks are loaded on demand via svc_eep_load_block()
 * or svc_eep_read_block().
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_ALREADY_INITIALIZED if already initialized.
 * @return DEV_ERR_CONFIG if block configuration is invalid.
 * @return DEV_ERR_TIMEOUT if EEPROM does not respond.
 */
dev_err_t svc_eep_init(void);

/**
 * @brief Deinitialize the EEPROM service without syncing.
 *
 * Clears all block runtime states. Does not write to EEPROM.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_deinit(void);

/**
 * @brief Shutdown the EEPROM service.
 *
 * Syncs all dirty blocks to EEPROM (if auto-sync is enabled),
 * then deinitializes dev_eep.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_shutdown(void);

/**
 * @brief Check if the EEPROM service is initialized.
 *
 * @return true if initialized, false otherwise.
 */
bool svc_eep_is_initialized(void);

/* ── Block load ── */

/**
 * @brief Load a single block from EEPROM into its RAM mirror.
 *
 * Reads only the configured EEPROM range for this block via
 * dev_eep_read(). Does not read other blocks or the whole EEPROM.
 *
 * After a successful load:
 *   loaded = true
 *   valid  = true
 *   dirty  = false
 *
 * @param block_id Block identifier (from svc_eep_block_id_t enum).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if block_id is invalid.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_TIMEOUT if EEPROM read fails.
 */
dev_err_t svc_eep_load_block(svc_eep_block_id_t block_id);

/* ── Block read ── */

/**
 * @brief Read data from a block's RAM mirror.
 *
 * If the block is not yet loaded, loads it from EEPROM first.
 * The length must equal the configured block size.
 *
 * @param block_id Block identifier.
 * @param data     Output buffer (caller-owned).
 * @param length   Number of bytes to read (must match block size).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if block_id is invalid or length mismatch.
 * @return DEV_ERR_NULL_PTR if data is NULL.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_read_block(svc_eep_block_id_t block_id,
                             void *data,
                             uint16_t length);

/* ── Direct write (immediate EEPROM write) ── */

/**
 * @brief Write data directly to EEPROM by block ID.
 *
 * Writes immediately to physical EEPROM via dev_eep_write().
 * Does not require the mirror to be loaded first.
 * If the block mirror is loaded, updates it and marks it clean.
 *
 * Use this when data must be persisted immediately.
 *
 * @param block_id Block identifier.
 * @param data     Data to write.
 * @param length   Number of bytes to write (must match block size).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if block_id is invalid or length mismatch.
 * @return DEV_ERR_NULL_PTR if data is NULL.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_TIMEOUT if EEPROM write fails.
 */
dev_err_t svc_eep_write_direct(svc_eep_block_id_t block_id,
                               const void *data,
                               uint16_t length);

/* ── Mirror write (RAM only, deferred EEPROM write) ── */

/**
 * @brief Write data to a block's RAM mirror only.
 *
 * Copies data into the block's RAM mirror and marks the block dirty.
 * Does NOT write to physical EEPROM until svc_eep_sync_block() or
 * svc_eep_sync_all() is called.
 *
 * Use this to reduce EEPROM write cycles when data changes frequently.
 *
 * @param block_id Block identifier.
 * @param data     Data to write.
 * @param length   Number of bytes to write (must match block size).
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if block_id is invalid or length mismatch.
 * @return DEV_ERR_NULL_PTR if data is NULL.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_write_mirror(svc_eep_block_id_t block_id,
                               const void *data,
                               uint16_t length);

/* ── Mirror pointer access ── */

/**
 * @brief Get a direct pointer to a block's RAM mirror.
 *
 * Ensures the block is loaded, then returns a pointer to its RAM
 * mirror and the block size. Useful for in-place struct access.
 *
 * If the application modifies data through the returned pointer,
 * it MUST call svc_eep_mark_dirty() afterwards.
 *
 * @param block_id Block identifier.
 * @param ptr      Output pointer to RAM mirror.
 * @param length   Output block size in bytes.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if block_id is invalid.
 * @return DEV_ERR_NULL_PTR if ptr or length is NULL.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_get_mirror_ptr(svc_eep_block_id_t block_id,
                                 void **ptr,
                                 uint16_t *length);

/**
 * @brief Mark a block as dirty without writing data.
 *
 * Use this after modifying a block's RAM mirror through a pointer
 * obtained via svc_eep_get_mirror_ptr().
 *
 * The block must be loaded before marking dirty.
 *
 * @param block_id Block identifier.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if block_id is invalid.
 * @return DEV_ERR_INVALID_STATE if block is not loaded.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t svc_eep_mark_dirty(svc_eep_block_id_t block_id);

/* ── Sync (write dirty mirrors to EEPROM) ── */

/**
 * @brief Sync a single dirty block to EEPROM.
 *
 * If the block is not dirty, returns DEV_OK without writing.
 * Writes the block's RAM mirror to EEPROM via dev_eep_write().
 * Clears the dirty flag after a successful write.
 *
 * @param block_id Block identifier.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_INVALID_ARG if block_id is invalid.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_TIMEOUT if EEPROM write fails.
 */
dev_err_t svc_eep_sync_block(svc_eep_block_id_t block_id);

/**
 * @brief Sync all dirty blocks to EEPROM.
 *
 * Iterates through all configured blocks and syncs only dirty ones.
 * Skips clean blocks. Does not write unused EEPROM areas.
 *
 * @return DEV_OK on success.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_TIMEOUT if any block sync fails.
 */
dev_err_t svc_eep_sync_all(void);

/* ── State queries ── */

/**
 * @brief Check if a specific block is loaded.
 *
 * @param block_id Block identifier.
 *
 * @return true if the block RAM mirror contains valid data.
 */
bool svc_eep_is_block_loaded(svc_eep_block_id_t block_id);

/**
 * @brief Check if a specific block is dirty.
 *
 * @param block_id Block identifier.
 *
 * @return true if the block RAM mirror differs from EEPROM.
 */
bool svc_eep_is_block_dirty(svc_eep_block_id_t block_id);

/**
 * @brief Check if any configured block is dirty.
 *
 * @return true if at least one block is dirty.
 */
bool svc_eep_is_dirty(void);

#ifdef __cplusplus
}
#endif

#endif /* SVC_EEP_H */
