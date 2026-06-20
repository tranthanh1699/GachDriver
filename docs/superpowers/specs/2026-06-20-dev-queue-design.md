# dev_queue Design Specification

## Overview

`dev_queue` is a lightweight generic static FIFO queue for embedded C projects. It stores fixed-size items by value in caller-provided static storage, with no dynamic allocation, no vendor HAL dependency, and no RTOS dependency.

## Architecture

```
Application / Services / Drivers
         |
         v
    dev_queue
         |
         v
    dev_common (types, errors, assert)
```

Single-file circular FIFO. No port layer needed — pure logic, no hardware interaction.

## Core Types

```c
typedef uint16_t dev_queue_size_t;       // item count type
typedef uint16_t dev_queue_item_size_t;  // per-item byte size

typedef struct {
    uint8_t *buffer;
    dev_queue_item_size_t item_size;
    dev_queue_size_t capacity;
    dev_queue_size_t head;
    dev_queue_size_t tail;
    dev_queue_size_t count;
    bool initialized;
} dev_queue_t;
```

## Public API (required)

- `dev_queue_init` / `dev_queue_deinit` / `dev_queue_reset`
- `dev_queue_is_initialized` / `dev_queue_is_empty` / `dev_queue_is_full`
- `dev_queue_get_count` / `dev_queue_get_capacity` / `dev_queue_get_free_count` / `dev_queue_get_item_size`
- `dev_queue_push` / `dev_queue_pop` / `dev_queue_peek`

## Optional APIs (config-gated)

- `dev_queue_push_overwrite` (`DEV_QUEUE_CFG_OVERWRITE_API_ENABLED`)
- `dev_queue_push_many` / `dev_queue_pop_many` (`DEV_QUEUE_CFG_MANY_API_ENABLED`)
- Critical section hooks (`DEV_QUEUE_CFG_CRITICAL_SECTION_ENABLED`)
- Clear-on-reset (`DEV_QUEUE_CFG_CLEAR_ON_RESET_ENABLED`)

## Static Helpers

```c
#define DEV_QUEUE_DEFINE(name, type, capacity) ...
#define DEV_QUEUE_INIT(name, type, capacity) ...
```

## Error Mapping

| Condition | Error |
|-----------|-------|
| Null pointer | `DEV_ERR_NULL_PTR` |
| Zero item_size/capacity | `DEV_ERR_INVALID_ARG` |
| Push when full | `DEV_ERR_OVERFLOW` |
| Pop/peek when empty | `DEV_ERR_EMPTY` |
| Not initialized | `DEV_ERR_NOT_INITIALIZED` |
| Double init | `DEV_ERR_ALREADY_INITIALIZED` |

All error codes already exist in `dev_error.h`.

## Files to Create

- `drivers/dev_queue/include/dev_queue.h`
- `drivers/dev_queue/include/dev_queue_types.h`
- `drivers/dev_queue/include/dev_queue_cfg.h`
- `drivers/dev_queue/src/dev_queue.c`
- `docs/dev_queue/README.md`

## Files to Update

- `README.md` — add dev_queue to component table

## Design Decisions

1. **No port layer.** dev_queue is pure logic, no hardware interaction.
2. **No RTOS awareness.** Caller manages thread safety. Optional critical-section hooks behind config for future.
3. **By-value copy.** Uses an internal byte-copy helper (not memcpy from string.h, per MISRA preference for bounded explicit copy).
4. **head/tail/count** rather than head/tail-only. Avoids wasting a slot and simplifies full/empty detection.
5. **`dev_queue_size_t` = uint16_t.** Supports up to 65535 items — sufficient for embedded use.

For the complete detailed specification, see the requirements document provided with the implementation request.
