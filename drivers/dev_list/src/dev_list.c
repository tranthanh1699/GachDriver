#include "dev_list.h"
#include <string.h>

static bool dev_list_is_initialized(const dev_list_t *list)
{
    return (list != NULL)
        && (list->node_pool != NULL)
        && (list->data_pool != NULL)
        && (list->capacity > 0U)
        && (list->item_size > 0U);
}

static dev_list_node_t *dev_list_acquire_node(dev_list_t *list)
{
    dev_list_node_t *node;

    if ((list == NULL) || (list->free_head == NULL)) {
        return NULL;
    }

    node = list->free_head;
    list->free_head = node->next;
    node->next = NULL;
    node->data_size = 0U;

    return node;
}

static void dev_list_release_node(dev_list_t *list, dev_list_node_t *node)
{
    if ((list == NULL) || (node == NULL)) {
        return;
    }

    node->next = list->free_head;
    list->free_head = node;
}

bool dev_list_init(dev_list_t      *list,
                   dev_list_node_t *node_pool,
                   void            *data_pool,
                   size_t           capacity,
                   size_t           item_size)
{
    size_t  index;
    uint8_t *data_bytes;

    if (list == NULL)                         { return false; }
    if ((node_pool == NULL) || (data_pool == NULL)) { return false; }
    if ((capacity == 0U) || (item_size == 0U))      { return false; }

    /* Overflow guard: capacity * item_size must not wrap */
    if (capacity > DEV_LIST_MAX_CAPACITY(item_size)) { return false; }

    list->node_pool = node_pool;
    list->data_pool = (uint8_t *)data_pool;
    list->capacity  = capacity;
    list->item_size = item_size;
    list->head      = NULL;
    list->tail      = NULL;
    list->size      = 0U;

    data_bytes = list->data_pool;

    for (index = 0U; index < capacity; index++) {
        node_pool[index].data      = (void *)&data_bytes[index * item_size];
        node_pool[index].data_size = 0U;
        node_pool[index].next      = ((index + 1U) < capacity)
                                     ? &node_pool[index + 1U] : NULL;
    }

    list->free_head = &node_pool[0];
    return true;
}

void dev_list_destroy(dev_list_t *list)
{
    if (!dev_list_is_initialized(list)) { return; }

    (void)dev_list_init(list,
                        list->node_pool,
                        list->data_pool,
                        list->capacity,
                        list->item_size);
}

static bool dev_list_push_impl(dev_list_t   *list,
                               const void   *data,
                               size_t        data_size,
                               bool          to_head)
{
    dev_list_node_t *node;

    if (!dev_list_is_initialized(list))                         { return false; }
    if ((data == NULL) || (data_size == 0U))                    { return false; }
    if (data_size > list->item_size)                            { return false; }

    node = dev_list_acquire_node(list);
    if (node == NULL)                                           { return false; }

    (void)memset(node->data, 0, list->item_size);
    (void)memcpy(node->data, data, data_size);
    node->data_size = data_size;

    if (to_head) {
        node->next  = list->head;
        list->head  = node;
        if (list->tail == NULL) { list->tail = node; }
    } else {
        if (list->tail != NULL) { list->tail->next = node; }
        list->tail = node;
        if (list->head == NULL) { list->head = node; }
    }

    list->size++;
    return true;
}

bool dev_list_push_head(dev_list_t *list, const void *data, size_t data_size)
{
    return dev_list_push_impl(list, data, data_size, true);
}

bool dev_list_push_tail(dev_list_t *list, const void *data, size_t data_size)
{
    return dev_list_push_impl(list, data, data_size, false);
}

static bool dev_list_pop_impl(dev_list_t *list,
                              void       *out_data,
                              size_t      out_size,
                              bool        from_head)
{
    dev_list_node_t *node;
    size_t           copy_size;

    if (!dev_list_is_initialized(list))                   { return false; }
    if (out_data == NULL)                                  { return false; }

    if (from_head) {
        node = list->head;
    } else {
        node = list->tail;
    }

    if (node == NULL)                                      { return false; }

    /* Out buffer must fit the actual stored data */
    if (out_size < node->data_size)                        { return false; }

    copy_size = node->data_size;

    if (from_head) {
        list->head = node->next;
        if (list->head == NULL) { list->tail = NULL; }
    } else {
        if (list->head == list->tail) {
            list->head = NULL;
            list->tail = NULL;
        } else {
            dev_list_node_t *prev = list->head;
            while (prev->next != list->tail) { prev = prev->next; }
            prev->next = NULL;
            list->tail = prev;
        }
    }

    (void)memcpy(out_data, node->data, copy_size);
    dev_list_release_node(list, node);
    list->size--;

    return true;
}

bool dev_list_pop_head(dev_list_t *list, void *out_data, size_t out_size)
{
    return dev_list_pop_impl(list, out_data, out_size, true);
}

bool dev_list_pop_tail(dev_list_t *list, void *out_data, size_t out_size)
{
    return dev_list_pop_impl(list, out_data, out_size, false);
}

static void dev_list_remove_impl(dev_list_t *list, bool from_head)
{
    dev_list_node_t *node;

    if (!dev_list_is_initialized(list)) { return; }

    if (from_head) {
        node = list->head;
    } else {
        node = list->tail;
    }

    if (node == NULL) { return; }

    if (from_head) {
        list->head = node->next;
        if (list->head == NULL) { list->tail = NULL; }
    } else {
        if (list->head == list->tail) {
            list->head = NULL;
            list->tail = NULL;
        } else {
            dev_list_node_t *prev = list->head;
            while (prev->next != list->tail) { prev = prev->next; }
            prev->next = NULL;
            list->tail = prev;
        }
    }

    dev_list_release_node(list, node);
    list->size--;
}

void dev_list_remove_head(dev_list_t *list) { dev_list_remove_impl(list, true); }
void dev_list_remove_tail(dev_list_t *list) { dev_list_remove_impl(list, false); }

size_t dev_list_size(const dev_list_t *list)
{
    return dev_list_is_initialized(list) ? list->size : 0U;
}

const dev_list_node_t *dev_list_head(const dev_list_t *list)
{
    return dev_list_is_initialized(list) ? list->head : NULL;
}

const dev_list_node_t *dev_list_next(const dev_list_node_t *node)
{
    return (node != NULL) ? node->next : NULL;
}

const void *dev_list_node_data(const dev_list_node_t *node)
{
    return (node != NULL) ? node->data : NULL;
}

size_t dev_list_node_data_size(const dev_list_node_t *node)
{
    return (node != NULL) ? node->data_size : 0U;
}
