#ifndef abInclude_Common
#define abInclude_Common

/*
 * Common top-level definitions - platform, most general-purpose macros.
 */

// CPU architecture.

#if defined(_M_AMD64) || defined(__x86_64__)
#define abPlatform_CPU_x86 1
#define abPlatform_CPU_x86_64 1
#elif defined(_M_IX86) || defined(__i386__)
#define abPlatform_CPU_x86 1
#define abPlatform_CPU_x86_32 1
#else
#error Unsupported target CPU.
#endif

#if defined(abPlatform_CPU_x86_64)
#define abPlatform_CPU_64Bit 1
#else
#define abPlatform_CPU_32Bit 1
#endif

// Operating system.

#if defined(_WIN32)
#define abPlatform_OS_Windows 1
#define abPlatform_OS_WindowsDesktop 1
#else
#error Unsupported target OS.
#endif

// Compiler.

#if defined(_MSC_VER)
#define abPlatform_Compiler_MSVC 1
#elif defined(__GNUC__)
#define abPlatform_Compiler_GNU 1
#else
#error Unsupported compiler.
#endif

// Debugging.

#ifdef _DEBUG
#define abPlatform_Debug 1
#endif

// Force inlining.

#if defined(abPlatform_Compiler_MSVC)
#define abForceInline __forceinline
#elif defined(abPlatform_Compiler_GNU)
#define abForceInline __attribute__((always_inline))
#else
#error No abForceInline known for the current compiler.
#endif

// Stack allocation.

#if defined(abPlatform_OS_Windows)
#include <malloc.h>
#define abStackAlloc _alloca
#else
#include <alloca.h>
#define abStackAlloc alloca
#endif

#include <stdint.h> // uint32_t...
#include <string.h> // memcpy, memmove, memset.

#endif
