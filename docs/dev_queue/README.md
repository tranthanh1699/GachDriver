# dev_queue

## Purpose

`dev_queue` is a lightweight generic static FIFO queue for embedded C projects. It stores fixed-size items by value in caller-provided static storage.

Key properties:
- No dynamic memory allocation
- No vendor HAL dependency
- No RTOS dependency
- Supports any fixed-size type (primitives, structs, enums, pointers)
- Circular FIFO with head/tail/count tracking
- Optional APIs gated by compile-time config macros

## Public APIs

### Lifecycle

| Function | Description |
|----------|-------------|
| `dev_queue_init()` | Initialize queue with user-provided buffer |
| `dev_queue_deinit()` | Deinitialize queue |
| `dev_queue_reset()` | Reset queue to empty state |

### State Queries

| Function | Description |
|----------|-------------|
| `dev_queue_is_initialized()` | Check if queue is initialized |
| `dev_queue_is_empty()` | Check if queue is empty |
| `dev_queue_is_full()` | Check if queue is full |
| `dev_queue_get_count()` | Get current item count |
| `dev_queue_get_capacity()` | Get maximum item capacity |
| `dev_queue_get_free_count()` | Get free slots remaining |
| `dev_queue_get_item_size()` | Get item size in bytes |

### Core Operations

| Function | Description |
|----------|-------------|
| `dev_queue_push()` | Push item (fails if full) |
| `dev_queue_pop()` | Pop oldest item (fails if empty) |
| `dev_queue_peek()` | Read oldest item without removing |

### Optional Operations

| Function | Config Gate | Description |
|----------|------------|-------------|
| `dev_queue_push_overwrite()` | `DEV_QUEUE_CFG_OVERWRITE_API_ENABLED` | Push, overwriting oldest if full |
| `dev_queue_push_many()` | `DEV_QUEUE_CFG_MANY_API_ENABLED` | Push multiple items atomically |
| `dev_queue_pop_many()` | `DEV_QUEUE_CFG_MANY_API_ENABLED` | Pop multiple items |

### Static Helpers

```c
DEV_QUEUE_DEFINE(name, type, capacity)  // Declare queue + storage
DEV_QUEUE_INIT(name, type, capacity)    // Initialize declared queue
```

## Configuration

| Macro | Default | Description |
|-------|---------|-------------|
| `DEV_QUEUE_CFG_RUNTIME_CHECK_ENABLED` | `1U` | Enable runtime validation |
| `DEV_QUEUE_CFG_OVERWRITE_API_ENABLED` | `1U` | Enable push_overwrite |
| `DEV_QUEUE_CFG_PEEK_API_ENABLED` | `1U` | Enable peek |
| `DEV_QUEUE_CFG_MANY_API_ENABLED` | `0U` | Enable bulk push/pop |
| `DEV_QUEUE_CFG_CLEAR_ON_RESET_ENABLED` | `0U` | Zero-fill buffer on reset |
| `DEV_QUEUE_CFG_CRITICAL_SECTION_ENABLED` | `0U` | Future: critical section hooks |

## Error Mapping

| Condition | Error Code |
|-----------|-----------|
| Null pointer argument | `DEV_ERR_NULL_PTR` |
| Zero item_size or capacity | `DEV_ERR_INVALID_ARG` |
| Queue not initialized | `DEV_ERR_NOT_INITIALIZED` |
| Double initialization | `DEV_ERR_ALREADY_INITIALIZED` |
| Push when full | `DEV_ERR_OVERFLOW` |
| Pop/peek when empty | `DEV_ERR_EMPTY` |

## Usage Example

### Queue of structs

```c
typedef struct {
    uint8_t  id;
    uint32_t timestamp;
} app_event_t;

DEV_QUEUE_DEFINE(s_event_queue, app_event_t, 8U);

void app_init(void)
{
    DEV_QUEUE_INIT(s_event_queue, app_event_t, 8U);
}

void app_send_event(uint8_t id)
{
    app_event_t event = { .id = id, .timestamp = 12345U };
    (void)dev_queue_push(&s_event_queue, &event);
}

void app_process_event(void)
{
    app_event_t event;
    if (dev_queue_pop(&s_event_queue, &event) == DEV_OK)
    {
        /* Handle event */
    }
}
```

## Safety Notes

- Not thread-safe. Caller must protect concurrent access.
- Not ISR-safe by default. Wrap calls in critical sections if needed.
- Stores items by value (byte copy). Pointers stored as pointer values only.
- No blocking behavior. All APIs return immediately.
- `dev_queue_t` must be zero-initialized or explicitly initialized before use.

## MISRA-C Notes

- No dynamic memory allocation
- No recursion
- Fixed-width integer types used throughout
- Private helpers are `static`
- No vendor or RTOS headers included
- All public APIs return `dev_err_t` where applicable

## Dependencies

- `dev_common` (types, errors, assert macros)
