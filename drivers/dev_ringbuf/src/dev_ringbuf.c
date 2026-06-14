#include "dev_ringbuf.h"

static size_t next_idx(size_t i, size_t cap) { return ((i + 1U) >= cap) ? 0U : (i + 1U); }

bool dev_ringbuf_is_valid(const dev_ringbuf_t *ring)
{
    return (ring != NULL)
        && (ring->storage != NULL)
        && (ring->capacity >= 2U)
        && (ring->head < ring->capacity)
        && (ring->tail < ring->capacity);
}

dev_err_t dev_ringbuf_init(dev_ringbuf_t *ring, uint8_t *storage, size_t capacity)
{
    DEV_RETURN_ON_FALSE(ring != NULL,    DEV_ERR_NULL_PTR);
    DEV_RETURN_ON_FALSE(storage != NULL, DEV_ERR_NULL_PTR);
    DEV_RETURN_ON_FALSE(capacity >= 2U,  DEV_ERR_INVALID_ARG);

    ring->storage  = storage;
    ring->capacity = capacity;
    ring->head     = 0U;
    ring->tail     = 0U;
    return DEV_OK;
}

/* ── ISR-safe (bool return, no error detail) ── */

bool dev_ringbuf_try_push(dev_ringbuf_t *ring, uint8_t value)
{
    if (!dev_ringbuf_is_valid(ring)) return false;

    size_t head = ring->head;
    size_t tail = ring->tail;
    size_t nxt  = next_idx(head, ring->capacity);

    if (nxt == tail) return false;   /* full */

    ring->storage[head] = value;
    ring->head = nxt;
    return true;
}

bool dev_ringbuf_try_pop(dev_ringbuf_t *ring, uint8_t *out_value)
{
    if (!dev_ringbuf_is_valid(ring)) return false;
    if (out_value == NULL)           return false;

    size_t tail = ring->tail;

    if (tail == ring->head) return false;  /* empty */

    *out_value = ring->storage[tail];
    ring->tail = next_idx(tail, ring->capacity);
    return true;
}

/* ── Non-ISR (dev_err_t return) ── */

dev_err_t dev_ringbuf_write(dev_ringbuf_t *ring, uint8_t value)
{
    if (!dev_ringbuf_is_valid(ring)) return DEV_ERR_INVALID_ARG;

    size_t nxt = next_idx(ring->head, ring->capacity);
    if (nxt == ring->tail) return DEV_ERR_BUSY;   /* full */

    ring->storage[ring->head] = value;
    ring->head = nxt;
    return DEV_OK;
}

dev_err_t dev_ringbuf_pop(dev_ringbuf_t *ring, uint8_t *out_value)
{
    if (!dev_ringbuf_is_valid(ring)) return DEV_ERR_INVALID_ARG;
    DEV_RETURN_ON_FALSE(out_value != NULL, DEV_ERR_NULL_PTR);

    if (ring->tail == ring->head) return DEV_ERR_BUSY;  /* empty */

    *out_value = ring->storage[ring->tail];
    ring->tail = next_idx(ring->tail, ring->capacity);
    return DEV_OK;
}

dev_err_t dev_ringbuf_read(dev_ringbuf_t *ring, uint8_t *data,
                           size_t data_len, size_t *out_read_len)
{
    size_t n = 0U;
    DEV_RETURN_ON_FALSE(dev_ringbuf_is_valid(ring), DEV_ERR_INVALID_ARG);
    DEV_RETURN_ON_FALSE(data != NULL, DEV_ERR_NULL_PTR);
    DEV_RETURN_ON_FALSE(out_read_len != NULL, DEV_ERR_NULL_PTR);

    while (n < data_len) {
        uint8_t b;
        if (!dev_ringbuf_try_pop(ring, &b)) break;
        data[n++] = b;
    }
    *out_read_len = n;
    return DEV_OK;
}

/* ── Query ── */

size_t dev_ringbuf_available(const dev_ringbuf_t *ring)
{
    if (!dev_ringbuf_is_valid(ring)) return 0U;
    size_t h = ring->head, t = ring->tail;
    return (h >= t) ? (h - t) : (ring->capacity - t + h);
}

size_t dev_ringbuf_capacity(const dev_ringbuf_t *ring)
{
    if (!dev_ringbuf_is_valid(ring)) return 0U;
    return ring->capacity - 1U;  /* one slot reserved */
}

size_t dev_ringbuf_free(const dev_ringbuf_t *ring)
{
    if (!dev_ringbuf_is_valid(ring)) return 0U;
    return dev_ringbuf_capacity(ring) - dev_ringbuf_available(ring);
}

dev_err_t dev_ringbuf_flush(dev_ringbuf_t *ring)
{
    DEV_RETURN_ON_FALSE(dev_ringbuf_is_valid(ring), DEV_ERR_INVALID_ARG);

    /*
     * Writes tail = head — caller MUST ensure no concurrent push or pop
     * is in flight (e.g., disable the producer ISR before calling).
     */
    ring->tail = ring->head;
    return DEV_OK;
}
