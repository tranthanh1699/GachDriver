# dev_list — Fixed-Capacity Singly-Linked List

## 1. Overview

`dev_list` is a static-memory singly-linked list for embedded systems. No dynamic allocation — all memory is caller-owned and provided at init time.

- **Fixed capacity** — pool of pre-allocated nodes + data buffer
- **Variable-size items** — each push stores the actual payload size, pop returns exactly that many bytes
- **Overflow-safe** — `DEV_LIST_MAX_CAPACITY(item_size)` prevents `capacity * item_size` wrap
- **Zero dependencies** — only `<stdbool.h>`, `<stddef.h>`, `<stdint.h>`, `<string.h>`

---

## 2. Quick Start

```c
#include "dev_list.h"

#define CAP  (4U)
#define ISZ  (32U)

static dev_list_node_t nodes[CAP];
static uint8_t         data[CAP * ISZ];
static dev_list_t      list;

int main(void)
{
    dev_list_init(&list, nodes, data, CAP, ISZ);

    dev_list_push_tail(&list, "hello", 5);
    dev_list_push_tail(&list, "world", 5);

    uint8_t buf[ISZ];
    while (dev_list_pop_head(&list, buf, ISZ)) {
        /* process buf */
    }
}
```

---

## 3. Types

### 3.1 Node

```c
typedef struct dev_list_node {
    void                 *data;       /* pointer into data_pool */
    size_t                data_size;  /* actual payload bytes stored */
    struct dev_list_node *next;
} dev_list_node_t;
```

### 3.2 List

```c
typedef struct {
    dev_list_node_t *node_pool;   /* caller-owned node array */
    uint8_t         *data_pool;   /* caller-owned data buffer */
    dev_list_node_t *free_head;   /* internal freelist */
    size_t           capacity;    /* max node count */
    size_t           item_size;   /* max bytes per element */
    dev_list_node_t *head;        /* first node */
    dev_list_node_t *tail;        /* last node */
    size_t           size;        /* current count */
} dev_list_t;
```

All fields are **private-by-convention** — use API functions only.

### 3.3 Capacity macro

```c
#define DEV_LIST_MAX_CAPACITY(item_size) \
    ((item_size) > 0U ? (SIZE_MAX / (item_size)) : 0U)
```

Use this to size your pools safely:

```c
dev_list_node_t nodes[DEV_LIST_MAX_CAPACITY(64U)];   // overflow-safe
uint8_t         data [DEV_LIST_MAX_CAPACITY(64U) * 64U];
```

---

## 4. API Reference

### 4.1 Init / Destroy

```c
bool dev_list_init(dev_list_t *list, dev_list_node_t *node_pool,
                   void *data_pool, size_t capacity, size_t item_size);
```

Returns `false` if: NULL pointers, zero capacity/item_size, or overflow (`capacity > SIZE_MAX / item_size`).

```c
void dev_list_destroy(dev_list_t *list);
```

Resets list to empty — all nodes returned to free pool. Safe to re-init afterwards.

### 4.2 Push

```c
bool dev_list_push_head(dev_list_t *list, const void *data, size_t data_size);
bool dev_list_push_tail(dev_list_t *list, const void *data, size_t data_size);
```

Returns `false` if: uninitialized, NULL data, `data_size == 0`, `data_size > item_size`, or list full.

Allocate → zero-fill → copy data → link into list. O(1) for head push. O(1) for tail push.

### 4.3 Pop

```c
bool dev_list_pop_head(dev_list_t *list, void *out_data, size_t out_size);
bool dev_list_pop_tail(dev_list_t *list, void *out_data, size_t out_size);
```

Copies **exactly the stored `data_size` bytes** (not `item_size`). Returns `false` if: uninitialized, NULL out_data, `out_size < stored data_size`, or empty.

O(1) for head pop. O(n) for tail pop (follows the linked list to find prev).

### 4.4 Remove (discard)

```c
void dev_list_remove_head(dev_list_t *list);   // O(1)
void dev_list_remove_tail(dev_list_t *list);   // O(n)
```

No-op if empty or uninitialized. Returns node to free pool, decrements size.

### 4.5 Query

```c
size_t               dev_list_size(const dev_list_t *list);
const dev_list_node_t *dev_list_head(const dev_list_t *list);
const dev_list_node_t *dev_list_next(const dev_list_node_t *node);
const void           *dev_list_node_data(const dev_list_node_t *node);
size_t                dev_list_node_data_size(const dev_list_node_t *node);
```

---

## 5. Usage Patterns

### 5.1 FIFO queue (head = dequeue, tail = enqueue)

```c
dev_list_push_tail(&list, &item, sizeof(item));   // enqueue
dev_list_pop_head(&list, &item, sizeof(item));    // dequeue
```

### 5.2 LIFO stack (head = push/pop)

```c
dev_list_push_head(&list, &item, sizeof(item));
dev_list_pop_head(&list, &item, sizeof(item));
```

### 5.3 Variable-size messages

```c
dev_list_push_tail(&list, "OK", 2);
dev_list_push_tail(&list, "ERROR_TIMEOUT", 13);

uint8_t buf[32];
size_t len;
if (dev_list_pop_head(&list, buf, sizeof(buf))) {
    len = dev_list_node_data_size(dev_list_head(&list)); // got 2 or 13
}
```

### 5.4 Iteration

```c
for (const dev_list_node_t *n = dev_list_head(&list);
     n != NULL;
     n = dev_list_next(n))
{
    const void   *data = dev_list_node_data(n);
    size_t        size = dev_list_node_data_size(n);
    /* process */
}
```

---

## 6. Build

```cmake
add_subdirectory(drivers/dev_list)
target_link_libraries(${PROJECT_NAME} dev_list)
```

No dependencies — `dev_list` is self-contained (does not depend on `dev_common`).

---

## 7. Safety & MISRA

| Rule | Status |
|------|--------|
| No dynamic allocation | ✓ Caller-owned static pools |
| No recursion | ✓ Iterative only |
| No unbounded loops | ✓ Bounded by capacity |
| No magic numbers | ✓ All constants named |
| Fixed-width types | ✓ `size_t`, `uint8_t` |
| Overflow guard | ✓ `DEV_LIST_MAX_CAPACITY` check in init |
| NULL validation | ✓ All public APIs validate pointers |
