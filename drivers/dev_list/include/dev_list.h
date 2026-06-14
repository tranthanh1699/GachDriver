#ifndef DEV_LIST_H
#define DEV_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Singly-linked list node.
 *
 * Fields are private-by-convention — do not modify directly.
 * Use dev_list_*() API functions for all operations.
 */
typedef struct dev_list_node
{
    void                 *data;       /* pointer into data_pool */
    size_t                data_size;  /* actual payload bytes stored */
    struct dev_list_node *next;
} dev_list_node_t;

/**
 * @brief Fixed-capacity singly-linked list context.
 *
 * Fields are private-by-convention — do not modify directly.
 * Allocate via caller-owned static pools (no dynamic memory).
 */
typedef struct
{
    dev_list_node_t *node_pool;   /* caller-owned node array    */
    uint8_t         *data_pool;   /* caller-owned data buffer   */
    dev_list_node_t *free_head;   /* internal freelist          */
    size_t           capacity;    /* max node count             */
    size_t           item_size;   /* max bytes per element      */
    dev_list_node_t *head;        /* first node                 */
    dev_list_node_t *tail;        /* last node                  */
    size_t           size;        /* current node count         */
} dev_list_t;

/**
 * @brief Maximum capacity for a given item_size (overflow-safe).
 *
 * Use this to size your node_pool and data_pool:
 *   dev_list_node_t nodes[DEV_LIST_MAX_CAPACITY(64U)];
 *   uint8_t         data [DEV_LIST_MAX_CAPACITY(64U) * 64U];
 */
#define DEV_LIST_MAX_CAPACITY(item_size) \
    ((item_size) > 0U ? (SIZE_MAX / (item_size)) : 0U)

/**
 * @brief Initialize list with caller-owned static pools.
 *
 * @param list      List context.
 * @param node_pool Node storage array of size `capacity`.
 * @param data_pool Data storage buffer of size `capacity * item_size`.
 * @param capacity  Maximum number of elements (1 .. DEV_LIST_MAX_CAPACITY).
 * @param item_size Maximum bytes per element (> 0).
 *
 * @return true on success, false on NULL ptr, zero capacity/size, or overflow.
 */
bool dev_list_init(dev_list_t       *list,
                   dev_list_node_t  *node_pool,
                   void             *data_pool,
                   size_t            capacity,
                   size_t            item_size);

/**
 * @brief Reset list to empty state and return all nodes to free pool.
 */
void dev_list_destroy(dev_list_t *list);

/**
 * @brief Push a copy of payload at list head.
 *
 * @param list      List context.
 * @param data      Payload pointer (must not be NULL).
 * @param data_size Payload size in bytes (1 .. item_size).
 *
 * @return true on success, false if uninitialized / NULL data /
 *         data_size == 0 / data_size > item_size / list full.
 */
bool dev_list_push_head(dev_list_t *list, const void *data, size_t data_size);

/**
 * @brief Push a copy of payload at list tail.
 */
bool dev_list_push_tail(dev_list_t *list, const void *data, size_t data_size);

/**
 * @brief Pop head node and copy payload to caller buffer.
 *
 * Copies exactly the stored data_size bytes (not item_size).
 *
 * @param list     List context.
 * @param out_data Caller output buffer.
 * @param out_size Output buffer size in bytes (must be >= stored data_size).
 *
 * @return true on success, false when list empty/invalid or output too small.
 */
bool dev_list_pop_head(dev_list_t *list, void *out_data, size_t out_size);

/**
 * @brief Pop tail node and copy payload to caller buffer.
 */
bool dev_list_pop_tail(dev_list_t *list, void *out_data, size_t out_size);

/**
 * @brief Remove head node (discard payload).
 */
void dev_list_remove_head(dev_list_t *list);

/**
 * @brief Remove tail node (discard payload).
 */
void dev_list_remove_tail(dev_list_t *list);

/**
 * @brief Get current list size.
 */
size_t dev_list_size(const dev_list_t *list);

/**
 * @brief Get read-only pointer to the head node (NULL if empty).
 */
const dev_list_node_t *dev_list_head(const dev_list_t *list);

/**
 * @brief Get read-only pointer to the next node (NULL at end).
 */
const dev_list_node_t *dev_list_next(const dev_list_node_t *node);

/**
 * @brief Get read-only pointer to node payload (NULL if node is NULL).
 */
const void *dev_list_node_data(const dev_list_node_t *node);

/**
 * @brief Get stored data size of a node (0 if node is NULL).
 */
size_t dev_list_node_data_size(const dev_list_node_t *node);

#ifdef __cplusplus
}
#endif

#endif /* DEV_LIST_H */
