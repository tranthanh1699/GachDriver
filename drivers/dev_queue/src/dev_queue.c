#include "dev_queue.h"
#include "dev_assert.h"

/* ── Private helpers ── */

static dev_queue_size_t dev_queue_next_idx(dev_queue_size_t idx,
                                           dev_queue_size_t capacity)
{
    dev_queue_size_t next;
    next = idx + 1U;
    if (next >= capacity)
    {
        next = 0U;
    }
    return next;
}

static void dev_queue_copy_bytes(uint8_t *dst,
                                 const uint8_t *src,
                                 dev_queue_item_size_t length)
{
    dev_queue_item_size_t i;
    for (i = 0U; i < length; i++)
    {
        dst[i] = src[i];
    }
}

static void dev_queue_item_ptr(const dev_queue_t *queue,
                               dev_queue_size_t idx,
                               uint8_t **out_ptr)
{
    *out_ptr = &queue->buffer[(dev_queue_size_t)(idx * queue->item_size)];
}

/* ── Lifecycle ── */

dev_err_t dev_queue_init(dev_queue_t *queue,
                         void *buffer,
                         dev_queue_item_size_t item_size,
                         dev_queue_size_t capacity)
{
    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_PTR_RET(buffer);
    DEV_CHECK_RET((item_size > 0U), DEV_ERR_INVALID_ARG);
    DEV_CHECK_RET((capacity > 0U), DEV_ERR_INVALID_ARG);
    DEV_CHECK_RET((!queue->initialized), DEV_ERR_ALREADY_INITIALIZED);

    queue->buffer       = (uint8_t *)buffer;
    queue->item_size    = item_size;
    queue->capacity     = capacity;
    queue->head         = 0U;
    queue->tail         = 0U;
    queue->count        = 0U;
    queue->initialized  = true;

    return DEV_OK;
}

dev_err_t dev_queue_deinit(dev_queue_t *queue)
{
    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);

    queue->buffer       = NULL;
    queue->item_size    = 0U;
    queue->capacity     = 0U;
    queue->head         = 0U;
    queue->tail         = 0U;
    queue->count        = 0U;
    queue->initialized  = false;

    return DEV_OK;
}

dev_err_t dev_queue_reset(dev_queue_t *queue)
{
    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);

    queue->head  = 0U;
    queue->tail  = 0U;
    queue->count = 0U;

#if (DEV_QUEUE_CFG_CLEAR_ON_RESET_ENABLED == 1U)
    {
        dev_queue_size_t total_bytes;
        dev_queue_size_t i;
        total_bytes = (dev_queue_size_t)(queue->capacity * queue->item_size);
        for (i = 0U; i < total_bytes; i++)
        {
            queue->buffer[i] = 0U;
        }
    }
#endif

    return DEV_OK;
}

/* ── State queries ── */

bool dev_queue_is_initialized(const dev_queue_t *queue)
{
    bool result;
    result = false;
    if (queue != NULL)
    {
        result = queue->initialized;
    }
    return result;
}

bool dev_queue_is_empty(const dev_queue_t *queue)
{
    bool result;
    result = true;
    if ((queue != NULL) && queue->initialized)
    {
        result = (queue->count == 0U);
    }
    return result;
}

bool dev_queue_is_full(const dev_queue_t *queue)
{
    bool result;
    result = false;
    if ((queue != NULL) && queue->initialized)
    {
        result = (queue->count >= queue->capacity);
    }
    return result;
}

dev_queue_size_t dev_queue_get_count(const dev_queue_t *queue)
{
    dev_queue_size_t result;
    result = 0U;
    if ((queue != NULL) && queue->initialized)
    {
        result = queue->count;
    }
    return result;
}

dev_queue_size_t dev_queue_get_capacity(const dev_queue_t *queue)
{
    dev_queue_size_t result;
    result = 0U;
    if ((queue != NULL) && queue->initialized)
    {
        result = queue->capacity;
    }
    return result;
}

dev_queue_size_t dev_queue_get_free_count(const dev_queue_t *queue)
{
    dev_queue_size_t result;
    result = 0U;
    if ((queue != NULL) && queue->initialized)
    {
        result = (dev_queue_size_t)(queue->capacity - queue->count);
    }
    return result;
}

dev_queue_item_size_t dev_queue_get_item_size(const dev_queue_t *queue)
{
    dev_queue_item_size_t result;
    result = 0U;
    if ((queue != NULL) && queue->initialized)
    {
        result = queue->item_size;
    }
    return result;
}

/* ── Core operations ── */

dev_err_t dev_queue_push(dev_queue_t *queue, const void *item)
{
    uint8_t *dst;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(item);

    if (queue->count >= queue->capacity)
    {
        return DEV_ERR_OVERFLOW;
    }

    dev_queue_item_ptr(queue, queue->tail, &dst);
    dev_queue_copy_bytes(dst, (const uint8_t *)item, queue->item_size);
    queue->tail = dev_queue_next_idx(queue->tail, queue->capacity);
    queue->count = (dev_queue_size_t)(queue->count + 1U);

    return DEV_OK;
}

dev_err_t dev_queue_pop(dev_queue_t *queue, void *item)
{
    uint8_t *src;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(item);

    if (queue->count == 0U)
    {
        return DEV_ERR_EMPTY;
    }

    dev_queue_item_ptr(queue, queue->head, &src);
    dev_queue_copy_bytes((uint8_t *)item, src, queue->item_size);
    queue->head = dev_queue_next_idx(queue->head, queue->capacity);
    queue->count = (dev_queue_size_t)(queue->count - 1U);

    return DEV_OK;
}

#if (DEV_QUEUE_CFG_PEEK_API_ENABLED == 1U)
dev_err_t dev_queue_peek(const dev_queue_t *queue, void *item)
{
    uint8_t *src;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(item);

    if (queue->count == 0U)
    {
        return DEV_ERR_EMPTY;
    }

    dev_queue_item_ptr(queue, queue->head, &src);
    dev_queue_copy_bytes((uint8_t *)item, src, queue->item_size);

    return DEV_OK;
}
#endif

/* ── Optional: overwrite ── */

#if (DEV_QUEUE_CFG_OVERWRITE_API_ENABLED == 1U)
dev_err_t dev_queue_push_overwrite(dev_queue_t *queue, const void *item)
{
    uint8_t *dst;
    dev_err_t result;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(item);

    result = DEV_OK;

    if (queue->count >= queue->capacity)
    {
        /* Drop oldest item to make room */
        queue->head = dev_queue_next_idx(queue->head, queue->capacity);
        queue->count = (dev_queue_size_t)(queue->count - 1U);
        result = DEV_ERR_OVERFLOW;
    }

    dev_queue_item_ptr(queue, queue->tail, &dst);
    dev_queue_copy_bytes(dst, (const uint8_t *)item, queue->item_size);
    queue->tail = dev_queue_next_idx(queue->tail, queue->capacity);
    queue->count = (dev_queue_size_t)(queue->count + 1U);

    return result;
}
#endif

/* ── Optional: bulk operations ── */

#if (DEV_QUEUE_CFG_MANY_API_ENABLED == 1U)
dev_err_t dev_queue_push_many(dev_queue_t *queue,
                              const void *items,
                              dev_queue_size_t item_count)
{
    const uint8_t *src;
    dev_queue_size_t i;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(items);
    DEV_CHECK_RET((item_count > 0U), DEV_ERR_INVALID_ARG);

    if ((dev_queue_size_t)(queue->count + item_count) > queue->capacity)
    {
        return DEV_ERR_OVERFLOW;
    }

    src = (const uint8_t *)items;
    for (i = 0U; i < item_count; i++)
    {
        uint8_t *dst;
        dev_queue_item_ptr(queue, queue->tail, &dst);
        dev_queue_copy_bytes(dst, src, queue->item_size);
        src = &src[queue->item_size];
        queue->tail = dev_queue_next_idx(queue->tail, queue->capacity);
    }
    queue->count = (dev_queue_size_t)(queue->count + item_count);

    return DEV_OK;
}

dev_err_t dev_queue_pop_many(dev_queue_t *queue,
                             void *items,
                             dev_queue_size_t item_count,
                             dev_queue_size_t *actual_count)
{
    uint8_t *dst;
    dev_queue_size_t i;
    dev_queue_size_t popped;

    DEV_CHECK_PTR_RET(queue);
    DEV_CHECK_RET(queue->initialized, DEV_ERR_NOT_INITIALIZED);
    DEV_CHECK_PTR_RET(items);
    DEV_CHECK_PTR_RET(actual_count);
    DEV_CHECK_RET((item_count > 0U), DEV_ERR_INVALID_ARG);

    popped = 0U;
    dst = (uint8_t *)items;

    for (i = 0U; i < item_count; i++)
    {
        uint8_t *src;
        if (queue->count == 0U)
        {
            break;
        }
        dev_queue_item_ptr(queue, queue->head, &src);
        dev_queue_copy_bytes(dst, src, queue->item_size);
        dst = &dst[queue->item_size];
        queue->head = dev_queue_next_idx(queue->head, queue->capacity);
        queue->count = (dev_queue_size_t)(queue->count - 1U);
        popped = (dev_queue_size_t)(popped + 1U);
    }

    *actual_count = popped;

    if (popped == 0U)
    {
        return DEV_ERR_EMPTY;
    }

    return DEV_OK;
}
#endif
