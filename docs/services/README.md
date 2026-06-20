# Services Layer

## Overview

Services are reusable modules built on top of drivers. They provide higher-level functionality such as command shells, persistent storage management, and protocol handlers.

Services depend on drivers. Drivers must not depend on services.

## Layer Rules

- Services may depend on drivers (`dev_*` modules).
- Services shall not directly use vendor HAL or SDK headers.
- Services shall not depend on application code.
- Services expose `svc_*` APIs.
- Services may have internal state.
- Services may provide `init`, `deinit`, `handle`, `shutdown`, or `flush` APIs.

## Dependency Direction

```
Application
    │
    v
Services (svc_*)
    │
    v
Drivers (dev_*)
    │
    v
Vendor HAL / SDK
```

## Components

| Component | Description | Documentation |
|-----------|-------------|---------------|
| `svc_shell` | UART command shell | [svc_shell/README.md](svc_shell/README.md) |
| `svc_eep` | I2C EEPROM service with RAM mirror | [svc_eep/README.md](svc_eep/README.md) |

## Dependencies

- `svc_shell` depends on `dev_uart`, `dev_common`.
- `svc_eep` depends on `dev_i2c`, `dev_common`, `dev_crc`.

## Adding a New Service

1. Create `services/svc_<name>/include/` and `services/svc_<name>/src/`.
2. Use the `svc_` prefix for all public symbols.
3. Depend only on `drivers/` modules — never on `app/` or other services without explicit approval.
4. Create `services/svc_<name>/CMakeLists.txt`.
5. Create `docs/services/svc_<name>/README.md`.
6. Update this file and the root `README.md`.
