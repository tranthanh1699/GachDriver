#ifndef DEV_EEP_H
#define DEV_EEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_eep_types.h"
#include "dev_eep_cfg.h"
#include "dev_error.h"

/**
 * @brief Initialize an EEPROM device.
 *
 * Verifies the device is present on the I2C bus via ACK polling.
 * Must be called after dev_i2c_init().
 *
 * @param eep_id Logical EEPROM device ID.
 *
 * @return DEV_OK if initialized successfully.
 * @return DEV_ERR_INVALID_ARG if eep_id is invalid.
 * @return DEV_ERR_ALREADY_INITIALIZED if already initialized.
 * @return DEV_ERR_TIMEOUT if device does not respond.
 * @return DEV_ERR_NOT_INITIALIZED if dev_i2c is not initialized.
 */
dev_err_t dev_eep_init(dev_eep_id_t eep_id);

/**
 * @brief Deinitialize an EEPROM device.
 *
 * Clears internal state. Does not write anything to the EEPROM.
 *
 * @param eep_id Logical EEPROM device ID.
 *
 * @return DEV_OK if deinitialized successfully.
 * @return DEV_ERR_INVALID_ARG if eep_id is invalid.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t dev_eep_deinit(dev_eep_id_t eep_id);

/**
 * @brief Read raw bytes from an EEPROM device.
 *
 * Reads are not page-constrained — the full requested length is read
 * in a single I2C transaction regardless of page boundaries.
 *
 * @param eep_id    Logical EEPROM device ID.
 * @param address   Byte address within the EEPROM.
 * @param data      Output buffer.
 * @param length    Number of bytes to read.
 *
 * @return DEV_OK if successful.
 * @return DEV_ERR_INVALID_ARG if eep_id is invalid or length is zero.
 * @return DEV_ERR_NULL_PTR if data is NULL.
 * @return DEV_ERR_OUT_OF_RANGE if address or address+length exceeds device size.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 */
dev_err_t dev_eep_read(dev_eep_id_t eep_id,
                       uint32_t address,
                       uint8_t *data,
                       uint32_t length);

/**
 * @brief Write raw bytes to an EEPROM device.
 *
 * This function handles page boundary splitting internally.
 * For writes spanning multiple pages, each page is written
 * separately with a write-cycle wait (ACK polling or delay).
 *
 * @param eep_id    Logical EEPROM device ID.
 * @param address   Byte address within the EEPROM.
 * @param data      Data buffer to write.
 * @param length    Number of bytes to write.
 *
 * @return DEV_OK if successful.
 * @return DEV_ERR_INVALID_ARG if eep_id is invalid or length is zero.
 * @return DEV_ERR_NULL_PTR if data is NULL.
 * @return DEV_ERR_OUT_OF_RANGE if address or address+length exceeds device size.
 * @return DEV_ERR_CONFIG if page_size is zero.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_TIMEOUT if ACK polling times out during write cycle.
 */
dev_err_t dev_eep_write(dev_eep_id_t eep_id,
                        uint32_t address,
                        const uint8_t *data,
                        uint32_t length);

/**
 * @brief Check if an EEPROM device is present and ready.
 *
 * Performs an I2C probe to verify the device responds.
 *
 * @param eep_id Logical EEPROM device ID.
 *
 * @return DEV_OK if device responds.
 * @return DEV_ERR_INVALID_ARG if eep_id is invalid.
 * @return DEV_ERR_NOT_INITIALIZED if not initialized.
 * @return DEV_ERR_TIMEOUT if device does not respond.
 */
dev_err_t dev_eep_is_ready(dev_eep_id_t eep_id);

/**
 * @brief Get information about an EEPROM device.
 *
 * @param eep_id Logical EEPROM device ID.
 * @param info   Output structure (total_size, page_size, mem_addr_size).
 *
 * @return DEV_OK if successful.
 * @return DEV_ERR_INVALID_ARG if eep_id is invalid.
 * @return DEV_ERR_NULL_PTR if info is NULL.
 */
dev_err_t dev_eep_get_info(dev_eep_id_t eep_id,
                           dev_eep_info_t *info);

/**
 * @brief Inject a fault that makes the next dev_eep_deinit() fail.
 *
 * Intended for testing deinitialization error paths.  When enabled,
 * the next call to dev_eep_deinit() returns DEV_ERR_FAIL regardless
 * of the actual device state.  The fault is automatically cleared
 * after one use so that subsequent calls behave normally.
 *
 * @param enable true to arm the fault, false to disarm without triggering.
 */
void dev_eep_set_deinit_fault(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* DEV_EEP_H */
