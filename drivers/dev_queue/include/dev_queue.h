#ifndef DEV_QUEUE_H
#define DEV_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_error.h"
#include "dev_queue_types.h"
#include "dev_queue_cfg.h"

/* ── Lifecycle ── */

dev_err_t dev_queue_init(dev_queue_t *queue,
                         void *buffer,
                         dev_queue_item_size_t item_size,
                         dev_queue_size_t capacity);

dev_err_t dev_queue_deinit(dev_queue_t *queue);

dev_err_t dev_queue_reset(dev_queue_t *queue);

/* ── State queries ── */

bool dev_queue_is_initialized(const dev_queue_t *queue);

bool dev_queue_is_empty(const dev_queue_t *queue);

bool dev_queue_is_full(const dev_queue_t *queue);

dev_queue_size_t dev_queue_get_count(const dev_queue_t *queue);

dev_queue_size_t dev_queue_get_capacity(const dev_queue_t *queue);

dev_queue_size_t dev_queue_get_free_count(const dev_queue_t *queue);

dev_queue_item_size_t dev_queue_get_item_size(const dev_queue_t *queue);

/* ── Core operations ── */

/**
 * @brief Push an item into the queue.
 * @return DEV_OK, DEV_ERR_NOT_INITIALIZED, DEV_ERR_NULL_PTR, DEV_ERR_OVERFLOW
 */
dev_err_t dev_queue_push(dev_queue_t *queue, const void *item);

/**
 * @brief Pop the oldest item from the queue.
 * @return DEV_OK, DEV_ERR_NOT_INITIALIZED, DEV_ERR_NULL_PTR, DEV_ERR_EMPTY
 */
dev_err_t dev_queue_pop(dev_queue_t *queue, void *item);

#if (DEV_QUEUE_CFG_PEEK_API_ENABLED == 1U)
/**
 * @brief Read the oldest item without removing it.
 * @return DEV_OK, DEV_ERR_NOT_INITIALIZED, DEV_ERR_NULL_PTR, DEV_ERR_EMPTY
 */
dev_err_t dev_queue_peek(const dev_queue_t *queue, void *item);
#endif

/* ── Optional: overwrite ── */

#if (DEV_QUEUE_CFG_OVERWRITE_API_ENABLED == 1U)
/**
 * @brief Push an item, overwriting the oldest if full.
 * @return DEV_OK if pushed normally, DEV_ERR_OVERFLOW if overwrite occurred.
 */
dev_err_t dev_queue_push_overwrite(dev_queue_t *queue, const void *item);
#endif

/* ── Optional: bulk operations ── */

#if (DEV_QUEUE_CFG_MANY_API_ENABLED == 1U)
dev_err_t dev_queue_push_many(dev_queue_t *queue,
                              const void *items,
                              dev_queue_size_t item_count);

dev_err_t dev_queue_pop_many(dev_queue_t *queue,
                             void *items,
                             dev_queue_size_t item_count,
                             dev_queue_size_t *actual_count);
#endif

/* ── Static declaration helpers ── */

#define DEV_QUEUE_DEFINE(name, type, capacity)          \
    static type name##_storage[(capacity)];              \
    static dev_queue_t name

#define DEV_QUEUE_INIT(name, type, capacity)                 \
    dev_queue_init(&(name),                                  \
                   (void *)(name##_storage),                 \
                   (dev_queue_item_size_t)sizeof(type),      \
                   (dev_queue_size_t)(capacity))

#ifdef __cplusplus
}
#endif

#endif /* DEV_QUEUE_H */
