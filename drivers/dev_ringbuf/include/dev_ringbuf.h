#ifndef DEV_RINGBUF_H
#define DEV_RINGBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dev_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fixed-size single-producer/single-consumer (SPSC) byte ring buffer.
 *
 * Concurrency contract:
 *   - One context (ISR or task) may call push/write and update `head`.
 *   - One context (ISR or task) may call pop/read and update `tail`.
 *   - No two contexts may write `head` concurrently.
 *   - No two contexts may write `tail` concurrently.
 *   - flush() writes `tail = head` — caller must ensure no concurrent push/pop.
 *   - On platforms where size_t is wider than the native word size, wrap
 *     push/pop in a critical section or disable interrupts.
 *
 * Usable capacity is `capacity - 1` (one slot reserved to distinguish
 * full from empty). Use dev_ringbuf_capacity() and dev_ringbuf_free().
 */
typedef struct {
    uint8_t        *storage;
    size_t          capacity;
    volatile size_t head;
    volatile size_t tail;
} dev_ringbuf_t;

dev_err_t dev_ringbuf_init(dev_ringbuf_t *ring, uint8_t *storage, size_t capacity);
bool      dev_ringbuf_is_valid(const dev_ringbuf_t *ring);

/* ISR-safe fast path — returns false on full/empty (no error detail) */
bool      dev_ringbuf_try_push(dev_ringbuf_t *ring, uint8_t value);
bool      dev_ringbuf_try_pop(dev_ringbuf_t *ring, uint8_t *out_value);

/* Non-ISR path — returns dev_err_t with error detail */
dev_err_t dev_ringbuf_write(dev_ringbuf_t *ring, uint8_t value);
dev_err_t dev_ringbuf_pop(dev_ringbuf_t *ring, uint8_t *out_value);

/* Bulk read */
dev_err_t dev_ringbuf_read(dev_ringbuf_t *ring, uint8_t *data,
                           size_t data_len, size_t *out_read_len);

/* Query */
size_t    dev_ringbuf_available(const dev_ringbuf_t *ring);
size_t    dev_ringbuf_capacity(const dev_ringbuf_t *ring);
size_t    dev_ringbuf_free(const dev_ringbuf_t *ring);

dev_err_t dev_ringbuf_flush(dev_ringbuf_t *ring);

#ifdef __cplusplus
}
#endif

#endif /* DEV_RINGBUF_H */
