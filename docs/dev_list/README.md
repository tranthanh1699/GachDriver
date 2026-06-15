# dev_list — Fixed-Capacity Singly-Linked List

## 1. Overview

Static-memory singly-linked list. No dynamic allocation — caller provides node pool + data buffer at init time. Each node stores the actual payload size; pop returns exactly that many bytes.

- **Variable-size items** — push 2 bytes, pop gets 2 bytes; push 8 bytes, pop gets 8
- **Overflow-safe init** — `DEV_LIST_MAX_CAPACITY(item_size)` prevents wrap
- **O(1) head**, O(n) tail for pop/remove

---

## 2. Quick Start — How to Use

```c
#include "dev_list.h"

#define CAP  8U
#define ISZ  32U

static dev_list_node_t nodes[CAP];
static uint8_t         pool[CAP * ISZ];
static dev_list_t      list;

int main(void)
{
    uint8_t buf[ISZ];

    dev_list_init(&list, nodes, pool, CAP, ISZ);

    /* Push at tail, pop from head → FIFO queue */
    dev_list_push_tail(&list, "hello", 5);
    dev_list_push_tail(&list, "world", 5);

    while (dev_list_pop_head(&list, buf, ISZ)) {
        /* buf contains "hello", then "world" */
    }

    /* Push at head, pop from head → LIFO stack */
    dev_list_push_head(&list, "first",  5);
    dev_list_push_head(&list, "second", 6);
    dev_list_pop_head(&list, buf, ISZ);   /* "second" */
    dev_list_pop_head(&list, buf, ISZ);   /* "first"  */

    /* Variable-size items */
    dev_list_push_tail(&list, "AB", 2);
    dev_list_push_tail(&list, "CDEFGH", 6);
    dev_list_pop_head(&list, buf, ISZ);   /* 2 bytes */
    dev_list_pop_head(&list, buf, ISZ);   /* 6 bytes */

    return 0;
}
```

---

## 3. Configuration — `dev_list.h`

No separate cfg header needed. All configuration is in the init call:

```c
dev_list_init(&list, node_pool, data_pool, capacity, item_size);
```

| Parameter | Meaning | Typical Value |
|-----------|---------|---------------|
| `node_pool` | Static array of `dev_list_node_t` | `dev_list_node_t nodes[8]` |
| `data_pool` | Static byte array for payloads | `uint8_t data[8 * 32]` |
| `capacity` | Max number of elements | `8U` |
| `item_size` | Max bytes per element | `32U` |

If `capacity > SIZE_MAX / item_size`, init returns `false` (overflow guard). This is tested by `DEV_LIST_MAX_CAPACITY(item_size)`.

---

## 4. API Reference

| Function | Description | O() |
|----------|-------------|-----|
| `dev_list_init(list, nodes, data, cap, isz)` | Initialize with static pools | O(n) |
| `dev_list_destroy(list)` | Reset to empty, reuse same pools | O(n) |
| `dev_list_push_head(list, data, data_size)` | Insert at front | O(1) |
| `dev_list_push_tail(list, data, data_size)` | Insert at back | O(1) |
| `dev_list_pop_head(list, out, out_size)` | Remove from front, copy payload | O(1) |
| `dev_list_pop_tail(list, out, out_size)` | Remove from back, copy payload | O(n) |
| `dev_list_remove_head(list)` | Discard head | O(1) |
| `dev_list_remove_tail(list)` | Discard tail | O(n) |
| `dev_list_size(list)` | Current count | O(1) |
| `dev_list_head(list)` | Read-only head pointer | O(1) |
| `dev_list_next(node)` | Read-only next pointer | O(1) |
| `dev_list_node_data(node)` | Read-only payload pointer | O(1) |
| `dev_list_node_data_size(node)` | Stored payload size | O(1) |

### Return values

| Condition | Push/Pop return |
|-----------|-----------------|
| Success | `true` |
| NULL pools, zero cap/size | `false` (init only) |
| NULL data, zero data_size | `false` |
| data_size > item_size | `false` |
| List full (no free nodes) | `false` |
| List empty | `false` |
| out_size < stored data_size | `false` |
| Uninitialized | `false` |

### Iteration

```c
for (const dev_list_node_t *n = dev_list_head(&list); n; n = dev_list_next(n)) {
    const void *payload = dev_list_node_data(n);
    size_t size = dev_list_node_data_size(n);
}
```

---

## 5. Porting — No Changes Needed

`dev_list` is pure C software — it has no hardware dependency and no port layer. Works on any platform with `<string.h>` and `<stddef.h>`.

---

## 6. Build

```cmake
add_subdirectory(drivers/dev_list)
target_link_libraries(${PROJECT_NAME} dev_list)
```

No dependencies — dev_list is self-contained (does not depend on dev_common).

---

## 7. Safety & MISRA

| Rule | Status |
|------|--------|
| No dynamic allocation | ✓ Caller-owned static pools |
| Overflow guard | ✓ `DEV_LIST_MAX_CAPACITY` check in init |
| NULL validation | ✓ All public APIs validate pointers |
| No recursion | ✓ Iterative only |
| No unbounded loops | ✓ Bounded by capacity |
| Per-node data_size | ✓ Pop returns exact stored size |
