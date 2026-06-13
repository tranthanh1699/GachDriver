#ifndef DEV_COMPILER_H
#define DEV_COMPILER_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_UNUSED(x)                ((void)(x))
#define DEV_ARRAY_SIZE(a)            (sizeof(a) / sizeof((a)[0]))
#define DEV_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)

#if defined(__GNUC__) || defined(__clang__)
  #define DEV_WEAK                   __attribute__((weak))
  #define DEV_PACKED                 __attribute__((packed))
  #define DEV_ALIGNED(n)             __attribute__((aligned(n)))
  #define DEV_NORETURN               __attribute__((noreturn))
  #define DEV_SECTION(s)             __attribute__((section(s)))
  #define DEV_BREAKPOINT()           __builtin_trap()
#else
  #error "Unsupported compiler"
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEV_COMPILER_H */
