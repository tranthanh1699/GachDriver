#include "svc_eep.h"
#include "dev_eep.h"
#include "dev_assert.h"
#include <string.h>

/* ── Internal state ── */

static svc_eep_block_state_t s_svc_eep_block_state[SVC_EEP_BLOCK_COUNT];
static bool s_initialized = false;

/* ── Internal helpers: forward declarations ── */

static const svc_eep_block_cfg_t *svc_eep_find_block_cfg(svc_eep_block_id_t block_id);
static dev_err_t svc_eep_validate_config_table(void);

/* ── Internal helper implementations ── */

static const svc_eep_block_cfg_t *svc_eep_find_block_cfg(svc_eep_block_id_t block_id)
{
    uint8_t i;

    for (i = 0U; i < SVC_EEP_BLOCK_COUNT; i++)
    {
        if (s_svc_eep_block_cfg[i].block_id == (uint8_t)block_id)
        {
            return &s_svc_eep_block_cfg[i];
        }
    }
    return NULL;
}

static dev_err_t svc_eep_validate_config_table(void)
{
    dev_eep_info_t eep_info;
    dev_err_t result;
    uint8_t i;
    uint8_t j;
    uint32_t block_end;
    uint32_t other_end;

    /* Get total EEPROM size from dev_eep for bounds checking */
    result = dev_eep_get_info(DEV_EEP_MAIN, &eep_info);
    if (result != DEV_OK)
    {
        return result;
    }

    for (i = 0U; i < SVC_EEP_BLOCK_COUNT; i++)
    {
        const svc_eep_block_cfg_t *cfg = &s_svc_eep_block_cfg[i];

        /* Block ID must match its array index */
        if (cfg->block_id != i)
        {
            return DEV_ERR_CONFIG;
        }

        /* Block size must be non-zero */
        if (cfg->block_size == 0U)
        {
            return DEV_ERR_CONFIG;
        }

        /* Mirror pointer must be valid (static allocation) */
#if (SVC_EEP_CFG_DYNAMIC_MIRROR_ENABLED == DEV_OFF)
        if (cfg->mirror == NULL)
        {
            return DEV_ERR_CONFIG;
        }
#endif

        /* EEPROM offset + size must not exceed total EEPROM size */
        block_end = cfg->eep_offset + (uint32_t)cfg->block_size;
        if ((block_end < cfg->eep_offset) || (block_end > eep_info.total_size))
        {
            return DEV_ERR_CONFIG;
        }
    }

    /* Check for overlapping blocks */
    for (i = 0U; i < SVC_EEP_BLOCK_COUNT; i++)
    {
        const svc_eep_block_cfg_t *cfg = &s_svc_eep_block_cfg[i];
        block_end = cfg->eep_offset + (uint32_t)cfg->block_size;

        for (j = (uint8_t)(i + 1U); j < SVC_EEP_BLOCK_COUNT; j++)
        {
            const svc_eep_block_cfg_t *other = &s_svc_eep_block_cfg[j];
            other_end = other->eep_offset + (uint32_t)other->block_size;

            if (!((block_end <= other->eep_offset) || (other_end <= cfg->eep_offset)))
            {
                return DEV_ERR_CONFIG;
            }
        }
    }

    return DEV_OK;
}

/* ── Lifecycle ── */

dev_err_t svc_eep_init(void)
{
    dev_err_t result;
    uint8_t i;

    if (s_initialized)
    {
        return DEV_ERR_ALREADY_INITIALIZED;
    }

    /* Validate block configuration */
    result = svc_eep_validate_config_table();
    if (result != DEV_OK)
    {
        return result;
    }

    /* Initialize the underlying EEPROM device driver */
    result = dev_eep_init(DEV_EEP_MAIN);
    if (result != DEV_OK)
    {
        return result;
    }

    /* Initialize all block runtime states */
    for (i = 0U; i < SVC_EEP_BLOCK_COUNT; i++)
    {
        s_svc_eep_block_state[i].loaded = false;
        s_svc_eep_block_state[i].dirty  = false;
        s_svc_eep_block_state[i].valid  = false;
    }

    s_initialized = true;
    return DEV_OK;
}

dev_err_t svc_eep_deinit(void)
{
    uint8_t i;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    /* Clear block states */
    for (i = 0U; i < SVC_EEP_BLOCK_COUNT; i++)
    {
        s_svc_eep_block_state[i].loaded = false;
        s_svc_eep_block_state[i].dirty  = false;
        s_svc_eep_block_state[i].valid  = false;
    }

    /* Deinitialize dev_eep without syncing */
    (void)dev_eep_deinit(DEV_EEP_MAIN);

    s_initialized = false;
    return DEV_OK;
}

dev_err_t svc_eep_shutdown(void)
{
    dev_err_t result;
    uint8_t i;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

#if (SVC_EEP_CFG_AUTO_SYNC_ON_SHUTDOWN == DEV_ON)
    result = svc_eep_sync_all();
    if (result != DEV_OK)
    {
        return result;
    }
#endif

    /* Deinitialize dev_eep */
    (void)dev_eep_deinit(DEV_EEP_MAIN);

    /* Clear block states */
    for (i = 0U; i < SVC_EEP_BLOCK_COUNT; i++)
    {
        s_svc_eep_block_state[i].loaded = false;
        s_svc_eep_block_state[i].dirty  = false;
        s_svc_eep_block_state[i].valid  = false;
    }

    s_initialized = false;
    return DEV_OK;
}

bool svc_eep_is_initialized(void)
{
    return s_initialized;
}

/* ── Block load ── */

dev_err_t svc_eep_load_block(svc_eep_block_id_t block_id)
{
    const svc_eep_block_cfg_t *cfg;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (block_id >= SVC_EEP_BLOCK_COUNT)
    {
        return DEV_ERR_INVALID_ARG;
    }

    cfg = svc_eep_find_block_cfg(block_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    /* Read only this block from EEPROM */
    result = dev_eep_read(DEV_EEP_MAIN,
                          cfg->eep_offset,
                          cfg->mirror,
                          (uint32_t)cfg->block_size);
    if (result != DEV_OK)
    {
        return result;
    }

    s_svc_eep_block_state[block_id].loaded = true;
    s_svc_eep_block_state[block_id].valid  = true;
    s_svc_eep_block_state[block_id].dirty  = false;

    return DEV_OK;
}

/* ── Block read ── */

dev_err_t svc_eep_read_block(svc_eep_block_id_t block_id,
                             void *data,
                             uint16_t length)
{
    const svc_eep_block_cfg_t *cfg;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (block_id >= SVC_EEP_BLOCK_COUNT)
    {
        return DEV_ERR_INVALID_ARG;
    }

    DEV_CHECK_PTR_RET(data);

    cfg = svc_eep_find_block_cfg(block_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (length != cfg->block_size)
    {
        return DEV_ERR_INVALID_ARG;
    }

    /* Load block on demand if not already loaded */
    if (!s_svc_eep_block_state[block_id].loaded)
    {
        result = svc_eep_load_block(block_id);
        if (result != DEV_OK)
        {
            return result;
        }
    }

    (void)memcpy(data, cfg->mirror, (size_t)cfg->block_size);

    return DEV_OK;
}

/* ── Direct write (immediate EEPROM write) ── */

dev_err_t svc_eep_write_direct(svc_eep_block_id_t block_id,
                               const void *data,
                               uint16_t length)
{
    const svc_eep_block_cfg_t *cfg;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (block_id >= SVC_EEP_BLOCK_COUNT)
    {
        return DEV_ERR_INVALID_ARG;
    }

    DEV_CHECK_PTR_RET(data);

    cfg = svc_eep_find_block_cfg(block_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (length != cfg->block_size)
    {
        return DEV_ERR_INVALID_ARG;
    }

    /* Write directly to EEPROM */
    result = dev_eep_write(DEV_EEP_MAIN,
                           cfg->eep_offset,
                           (const uint8_t *)data,
                           (uint32_t)cfg->block_size);
    if (result != DEV_OK)
    {
        return result;
    }

    /* If mirror is loaded, update it and mark clean */
    if (s_svc_eep_block_state[block_id].loaded)
    {
        (void)memcpy(cfg->mirror, data, (size_t)cfg->block_size);
        s_svc_eep_block_state[block_id].dirty = false;
        s_svc_eep_block_state[block_id].valid = true;
    }
    else
    {
        /* Mark as loaded with the written data in mirror */
        (void)memcpy(cfg->mirror, data, (size_t)cfg->block_size);
        s_svc_eep_block_state[block_id].loaded = true;
        s_svc_eep_block_state[block_id].valid  = true;
        s_svc_eep_block_state[block_id].dirty  = false;
    }

    return DEV_OK;
}

/* ── Mirror write (RAM only) ── */

dev_err_t svc_eep_write_mirror(svc_eep_block_id_t block_id,
                               const void *data,
                               uint16_t length)
{
    const svc_eep_block_cfg_t *cfg;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (block_id >= SVC_EEP_BLOCK_COUNT)
    {
        return DEV_ERR_INVALID_ARG;
    }

    DEV_CHECK_PTR_RET(data);

    cfg = svc_eep_find_block_cfg(block_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    if (length != cfg->block_size)
    {
        return DEV_ERR_INVALID_ARG;
    }

    /* Copy data to mirror only — do not write EEPROM */
    (void)memcpy(cfg->mirror, data, (size_t)cfg->block_size);

    s_svc_eep_block_state[block_id].loaded = true;
    s_svc_eep_block_state[block_id].valid  = true;
    s_svc_eep_block_state[block_id].dirty  = true;

    return DEV_OK;
}

/* ── Mirror pointer access ── */

dev_err_t svc_eep_get_mirror_ptr(svc_eep_block_id_t block_id,
                                 void **ptr,
                                 uint16_t *length)
{
    const svc_eep_block_cfg_t *cfg;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (block_id >= SVC_EEP_BLOCK_COUNT)
    {
        return DEV_ERR_INVALID_ARG;
    }

    DEV_CHECK_PTR_RET(ptr);
    DEV_CHECK_PTR_RET(length);

    cfg = svc_eep_find_block_cfg(block_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    /* Load block on demand if not already loaded */
    if (!s_svc_eep_block_state[block_id].loaded)
    {
        result = svc_eep_load_block(block_id);
        if (result != DEV_OK)
        {
            return result;
        }
    }

    *ptr    = (void *)cfg->mirror;
    *length = cfg->block_size;

    return DEV_OK;
}

dev_err_t svc_eep_mark_dirty(svc_eep_block_id_t block_id)
{
    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (block_id >= SVC_EEP_BLOCK_COUNT)
    {
        return DEV_ERR_INVALID_ARG;
    }

    if (!s_svc_eep_block_state[block_id].loaded)
    {
        return DEV_ERR_INVALID_STATE;
    }

    s_svc_eep_block_state[block_id].dirty = true;

    return DEV_OK;
}

/* ── Sync ── */

dev_err_t svc_eep_sync_block(svc_eep_block_id_t block_id)
{
    const svc_eep_block_cfg_t *cfg;
    dev_err_t result;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    if (block_id >= SVC_EEP_BLOCK_COUNT)
    {
        return DEV_ERR_INVALID_ARG;
    }

    cfg = svc_eep_find_block_cfg(block_id);
    DEV_CHECK_RET((cfg != NULL), DEV_ERR_INVALID_ARG);

    /* Nothing to sync if not dirty */
    if (!s_svc_eep_block_state[block_id].dirty)
    {
        return DEV_OK;
    }

    /* Write mirror to EEPROM */
    result = dev_eep_write(DEV_EEP_MAIN,
                           cfg->eep_offset,
                           cfg->mirror,
                           (uint32_t)cfg->block_size);
    if (result != DEV_OK)
    {
        return result;
    }

    s_svc_eep_block_state[block_id].dirty = false;

    return DEV_OK;
}

dev_err_t svc_eep_sync_all(void)
{
    dev_err_t result;
    dev_err_t first_error = DEV_OK;
    uint8_t i;

    if (!s_initialized)
    {
        return DEV_ERR_NOT_INITIALIZED;
    }

    for (i = 0U; i < SVC_EEP_BLOCK_COUNT; i++)
    {
        if (s_svc_eep_block_state[i].dirty)
        {
            result = svc_eep_sync_block((svc_eep_block_id_t)i);
            if ((result != DEV_OK) && (first_error == DEV_OK))
            {
                first_error = result;
            }
        }
    }

    return first_error;
}

/* ── State queries ── */

bool svc_eep_is_block_loaded(svc_eep_block_id_t block_id)
{
    if (block_id >= SVC_EEP_BLOCK_COUNT)
    {
        return false;
    }

    return s_svc_eep_block_state[block_id].loaded;
}

bool svc_eep_is_block_dirty(svc_eep_block_id_t block_id)
{
    if (block_id >= SVC_EEP_BLOCK_COUNT)
    {
        return false;
    }

    return s_svc_eep_block_state[block_id].dirty;
}

bool svc_eep_is_dirty(void)
{
    uint8_t i;

    for (i = 0U; i < SVC_EEP_BLOCK_COUNT; i++)
    {
        if (s_svc_eep_block_state[i].dirty)
        {
            return true;
        }
    }

    return false;
}
