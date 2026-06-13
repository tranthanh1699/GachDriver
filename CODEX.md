# CODEX.md

## 1. Purpose

This file defines coding and architecture rules for AI-assisted code generation in this embedded driver abstraction project.

Codex shall use this file when generating, modifying, reviewing, or refactoring code.

The project provides hardware-independent driver APIs so application code can run across different platforms such as STM32, ESP32, nRF52, and future MCUs without depending directly on vendor HALs or SDKs.

---

## 2. Project Intent

Application code shall use project-owned components:

```text
dev_common
dev_gpio
dev_spi
dev_i2c
dev_uart
dev_timer
dev_adc
dev_pwm
```

Application code shall not directly use:

```text
STM32 HAL / LL
ESP-IDF driver APIs
Nordic nrfx APIs
vendor register headers
vendor SDK-specific types
```

Vendor-specific code shall be isolated in port layers.

---

## 3. Mandatory Architecture

Use this dependency direction only:

```text
application
  -> dev_* public API
  -> dev_* common implementation
  -> dev_* port interface
  -> vendor HAL / SDK / registers
  -> hardware
```

Do not create reverse dependencies.

Do not include vendor headers in public driver headers.

Do not expose vendor types through public APIs.

---

## 4. Component Structure

Recommended structure:

```text
drivers/
  dev_common/
    include/
      dev_common.h
      dev_types.h
      dev_error.h
      dev_assert.h
      dev_log.h
      dev_compiler.h
      dev_version.h
    src/
      dev_common.c
      dev_assert.c
      dev_log.c

  dev_gpio/
    include/
      dev_gpio.h
      dev_gpio_types.h
      dev_gpio_cfg.h
      dev_gpio_port.h
    src/
      dev_gpio.c
    port/
      stm32/
        dev_gpio_port_stm32.c
      esp32/
        dev_gpio_port_esp32.c
      nrf52/
        dev_gpio_port_nrf52.c
      mock/
        dev_gpio_port_mock.c

boards/
  board_<target>/
    dev_board_cfg.h
    dev_gpio_board_cfg.h
    dev_gpio_board_cfg.c

application/
  app/
```

---

## 5. Naming Rules

All components shall use the `dev_` prefix.

Examples:

```text
dev_common
dev_gpio
dev_uart
dev_spi
dev_i2c
dev_timer
```

Public symbols shall use the `Dev_` prefix.

Example:

```c
Dev_ReturnType Dev_Gpio_Init(const Dev_GpioConfigType * config);
```

Private helper functions shall be `static`.

Example:

```c
static Dev_ReturnType dev_gpio_validate_channel(Dev_GpioChannelId channel);
```

File names shall be lower-case and component-prefixed.

Example:

```text
dev_gpio.c
dev_gpio.h
dev_gpio_types.h
dev_gpio_port_stm32.c
```

---

## 6. dev_common Rules

`dev_common` is the base component shared by all drivers.

It shall provide:

- common project types
- common return values
- error codes
- assert/check macros
- configurable error reporting backend
- compiler abstraction
- version information
- optional logging abstraction
- safety utilities

`dev_common` shall not depend directly on `dev_gpio`, `dev_uart`, `dev_spi`, or any other hardware driver.

If `dev_common` needs to print logs through UART, it shall use a user-provided output callback or low-level hook. It shall not directly call `Dev_Uart_Write`.

---

## 7. Common Error Type

Use a project-level error type.

Recommended definition:

```c
typedef enum
{
    DEV_E_OK = 0,
    DEV_E_NOT_OK,
    DEV_E_INVALID_PARAM,
    DEV_E_NULL_PTR,
    DEV_E_NOT_INITIALIZED,
    DEV_E_ALREADY_INITIALIZED,
    DEV_E_BUSY,
    DEV_E_TIMEOUT,
    DEV_E_UNSUPPORTED,
    DEV_E_OUT_OF_RANGE,
    DEV_E_HW_FAILURE
} Dev_ReturnType;
```

Do not return vendor HAL status values from public APIs.

---

## 8. Assert and Error Report Mechanism

The project shall provide its own assert/check mechanism, inspired by ESP-style check macros, but fully project-owned and portable.

The mechanism shall support:

- null pointer checks
- parameter validation
- initialization state checks
- error propagation
- optional UART output
- optional text buffer output
- optional breakpoint
- optional reset
- optional user hook
- optional silent mode

Recommended backend type:

```c
typedef enum
{
    DEV_ASSERT_BACKEND_NONE = 0,
    DEV_ASSERT_BACKEND_UART,
    DEV_ASSERT_BACKEND_TEXT_BUFFER,
    DEV_ASSERT_BACKEND_BREAKPOINT,
    DEV_ASSERT_BACKEND_RESET,
    DEV_ASSERT_BACKEND_USER_HOOK
} Dev_AssertBackend;
```

Recommended assert information type:

```c
typedef enum
{
    DEV_ASSERT_TYPE_ASSERT = 0,
    DEV_ASSERT_TYPE_CHECK,
    DEV_ASSERT_TYPE_ERROR
} Dev_AssertType;

typedef struct
{
    const char * file;
    uint32_t line;
    Dev_AssertType type;
    Dev_ReturnType error;
} Dev_AssertInfo;
```

Recommended hooks:

```c
typedef void (*Dev_AssertUserHook)(const Dev_AssertInfo * info);
typedef void (*Dev_AssertOutputHook)(const char * text);
```

---

## 9. Assert Macro Rules

Macros are allowed for simple error-checking patterns.

Example:

```c
#define DEV_CHECK_RET(condition, error_code)             \
    do                                                   \
    {                                                    \
        if ((condition) == false)                        \
        {                                                \
            Dev_Assert_Report(__FILE__, __LINE__,        \
                              DEV_ASSERT_TYPE_CHECK,     \
                              (error_code));             \
            return (error_code);                         \
        }                                                \
    } while (false)

#define DEV_CHECK_PTR_RET(pointer)                       \
    DEV_CHECK_RET(((pointer) != NULL), DEV_E_NULL_PTR)
```

Rules:

1. Macro names that return from a function shall contain `_RET`.
2. Macros shall not evaluate an argument multiple times.
3. Macros shall not hide complex logic.
4. Macros shall not allocate memory.
5. Macros shall not call vendor HAL directly.
6. Macros shall be documented.
7. Use inline functions instead of macros when safer.

---

## 10. Public API Rules

Public APIs must be hardware-independent.

Public APIs must not expose:

- vendor HAL handle types
- vendor status types
- vendor peripheral structures
- raw register addresses
- raw MCU pin names
- chip-specific interrupt names
- chip-specific clock names
- RTOS-specific types

Bad:

```c
HAL_StatusTypeDef Dev_Gpio_Write(GPIO_TypeDef * port, uint16_t pin);
```

Good:

```c
Dev_ReturnType Dev_Gpio_WriteChannel(Dev_GpioChannelId channel,
                                     Dev_GpioLevel level);
```

---

## 11. Configuration Rules

Configuration shall be explicit and centralized.

Use static configuration where possible.

Board-specific configuration shall live under:

```text
boards/board_<target>/
```

Driver-generic configuration shall live inside the corresponding `dev_*` component.

Do not hardcode board pins inside common driver logic.

Example:

```c
typedef struct
{
    Dev_GpioChannelId channel;
    Dev_GpioDirection direction;
    Dev_GpioPullMode pull;
    Dev_GpioLevel default_level;
    Dev_GpioPortPin port_pin;
} Dev_GpioChannelConfig;
```

---

## 12. dev_gpio Minimum API

`dev_gpio` shall provide logical GPIO access.

Recommended APIs:

```c
Dev_ReturnType Dev_Gpio_Init(const Dev_GpioConfigType * config);
Dev_ReturnType Dev_Gpio_DeInit(void);
Dev_ReturnType Dev_Gpio_WriteChannel(Dev_GpioChannelId channel,
                                     Dev_GpioLevel level);
Dev_ReturnType Dev_Gpio_ReadChannel(Dev_GpioChannelId channel,
                                    Dev_GpioLevel * level);
Dev_ReturnType Dev_Gpio_ToggleChannel(Dev_GpioChannelId channel);
```

Recommended logical channels:

```c
typedef enum
{
    DEV_GPIO_LED_STATUS = 0,
    DEV_GPIO_BUTTON_USER,
    DEV_GPIO_CAN_STB,
    DEV_GPIO_SPI_CS,
    DEV_GPIO_CHANNEL_COUNT
} Dev_GpioChannelId;
```

Application code shall use logical channel IDs instead of raw hardware pins.

---

## 13. Port Layer Rules

Port files are the only place where vendor-specific code is allowed.

Examples:

```text
dev_gpio_port_stm32.c
dev_gpio_port_esp32.c
dev_gpio_port_nrf52.c
```

Port layer responsibilities:

- include vendor HAL or SDK headers
- convert project config to hardware config
- call vendor APIs
- map vendor errors to `Dev_ReturnType`
- isolate target-specific limitations
- document unsupported features

Common driver files must not call vendor HAL directly.

---

## 14. MISRA-C Oriented Rules

Generated C code shall be MISRA-C oriented.

Avoid:

1. dynamic memory allocation
2. recursion
3. unbounded loops
4. magic numbers
5. implicit narrowing conversions
6. unchecked pointer usage
7. uninitialized variables
8. ignored return values
9. complex function-like macros
10. direct register access outside port files
11. vendor types in public headers
12. hidden control flow
13. unnecessary global variables
14. undefined behavior
15. implementation-defined behavior without documentation

Use:

```c
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
```

---

## 15. Safety Rules

Generated code shall be safe by default.

Each driver shall:

- validate input parameters
- validate initialization state
- return explicit error codes
- avoid undefined behavior
- use deterministic control flow
- keep hardware ownership clear
- avoid hidden side effects
- document unsupported features
- isolate unsafe operations
- avoid dynamic allocation
- avoid blocking forever

Do not create APIs that fail silently.

---

## 16. No Magic Numbers

Numeric values shall use named constants, enums, or configuration macros.

Bad:

```c
timeout = 1000U;
mask = 0x80U;
```

Good:

```c
#define DEV_GPIO_DEFAULT_TIMEOUT_MS      (1000U)
#define DEV_GPIO_PIN_7_MASK              (0x80U)
```

Prefer enums for logical values.

Example:

```c
typedef enum
{
    DEV_GPIO_LEVEL_LOW = 0,
    DEV_GPIO_LEVEL_HIGH = 1
} Dev_GpioLevel;
```

---

## 17. Initialization Rules

Each driver shall have a clear lifecycle:

```text
Uninitialized -> Initialized -> Running
```

Before initialization, APIs shall return `DEV_E_NOT_INITIALIZED` or behave in a documented safe way.

Do not allow undefined behavior before initialization.

---

## 18. Concurrency and RTOS Rules

Generic driver code shall not depend directly on an RTOS.

Forbidden in common driver code:

```c
xSemaphoreTake(...)
k_mutex_lock(...)
tx_mutex_get(...)
```

If locking is required, introduce a project-owned abstraction such as:

```text
dev_osal
```

Each API shall document whether it is:

- reentrant
- non-reentrant
- ISR-safe
- task-safe
- blocking
- non-blocking

---

## 19. Logging Rules

Logging shall be optional and configurable.

`dev_common` may provide a logging abstraction, but it shall not directly depend on hardware drivers.

Allowed design:

```c
typedef void (*Dev_LogOutputHook)(const char * text);
```

The user may route logs to:

- UART
- text buffer
- semihosting
- RTT
- SWO
- custom callback
- no output

Do not hardcode logging to one backend.

---

## 20. Testing Rules

Every driver shall be testable without real hardware.

Support:

- mock port layer
- host-based unit tests
- configuration validation tests
- error path tests
- boundary tests

Common driver logic shall be testable with fake ports.

Example:

```text
dev_gpio_port_mock.c
```

---

## 21. Documentation Rules

Every public API shall include documentation.

Documentation shall describe:

- purpose
- parameters
- return value
- initialization requirement
- reentrancy
- safety behavior
- error cases

Example:

```c
/**
 * @brief Write logic level to a configured GPIO channel.
 *
 * @param channel Logical GPIO channel ID.
 * @param level Requested output level.
 *
 * @return DEV_E_OK if successful.
 * @return DEV_E_INVALID_PARAM if input is invalid.
 * @return DEV_E_NOT_INITIALIZED if GPIO driver is not initialized.
 */
Dev_ReturnType Dev_Gpio_WriteChannel(Dev_GpioChannelId channel,
                                     Dev_GpioLevel level);
```

---

## 22. Clean Code Rules

Prefer:

- small functions
- clear names
- explicit types
- single responsibility
- deterministic control flow
- explicit error handling
- consistent formatting
- documented assumptions

Avoid:

- hidden side effects
- clever macros
- deeply nested logic
- copy-paste porting code
- global state without ownership
- unclear abbreviations

---

## 23. Bring-Up Checklist

When adding a new hardware target:

1. Create `boards/board_<target>/`.
2. Add board-level configuration.
3. Add required `dev_*_port_<target>.c` files.
4. Keep vendor includes inside port files only.
5. Map vendor errors to `Dev_ReturnType`.
6. Validate all configured pins and peripherals.
7. Build common driver tests.
8. Build target firmware.
9. Test initialization.
10. Test each public API.
11. Test error paths.
12. Document unsupported features.
13. Add bring-up notes.

---

## 24. Codex Code Generation Rules

When generating or modifying code:

1. Follow this file strictly.
2. Preserve the `dev_` component architecture.
3. Do not introduce vendor dependencies into public APIs.
4. Do not use dynamic allocation.
5. Do not use magic numbers.
6. Prefer static configuration.
7. Keep code MISRA-C oriented.
8. Keep code portable across STM32, ESP32, nRF52, and future MCUs.
9. Use `dev_common` for shared types, errors, assert, and logging.
10. Keep `dev_common` independent from hardware drivers.
11. Put hardware-specific behavior in the port layer.
12. Document assumptions and unsupported features.
13. Prefer incremental, safe changes.
14. Do not rewrite architecture casually.

---

## 25. Definition of Done

A change is complete only when:

- public APIs are hardware-independent
- vendor code is isolated in the port layer
- no magic numbers are introduced
- return values are handled
- configuration is explicit
- initialization behavior is safe
- error handling is deterministic
- code is readable
- tests or test strategy are possible
- MISRA-C oriented rules are respected
- safety assumptions are documented
