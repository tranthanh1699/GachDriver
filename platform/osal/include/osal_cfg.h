#ifndef OSAL_CFG_H
#define OSAL_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#define OSAL_CFG_RUNTIME_CHECK_ENABLED      (1U)

#define OSAL_CFG_BAREMETAL_ENABLED          (1U)
#define OSAL_CFG_FREERTOS_ENABLED           (0U)

#define OSAL_CFG_DEFAULT_TICK_RATE_HZ       (1000U)

#if ((OSAL_CFG_BAREMETAL_ENABLED + OSAL_CFG_FREERTOS_ENABLED) != 1U)
#error "osal_cfg: exactly one OSAL backend must be enabled"
#endif

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CFG_H */
