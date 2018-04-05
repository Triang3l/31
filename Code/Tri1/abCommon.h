#ifndef abInclude_abCommon
#define abInclude_abCommon

/*
 * Common top-level definitions - platform, most general-purpose macros.
 * If a header file doesn't include other engine headers, it MUST include abCommon!
 */

#include <float.h> // FLT_MAX...
#include <stdarg.h>
#include <stdint.h> // uint32_t...
#include <string.h> // memcpy, memmove, memset.

/*
 * Currently available build configuration values:
 *
 * GPU abstraction layer backend (mutually exclusive):
 * - abConfig_GPUi_D3D - Direct3D 12.
 */

#define abNull ((void *) 0)

typedef unsigned char abBool; // In case any C++ code needs to interact with C code and for a predictable size.
#define abTrue ((abBool) 1u)
#define abFalse ((abBool) 0u)

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

// Alignment - abAligned must be placed after the struct keyword.

#if defined(abPlatform_Compiler_MSVC)
#define abAligned(alignment) __declspec(align(alignment))
#elif defined(abPlatform_Compiler_GNU)
#define abAligned(alignment) __attribute__((aligned(alignment)))
#else
#error No abAligned known for the current compiler.
#endif

// Force inlining.

#if defined(abPlatform_Compiler_MSVC)
#define abForceInline __forceinline
#elif defined(abPlatform_Compiler_GNU)
#define abForceInline __attribute__((always_inline))
#else
#error No abForceInline known for the current compiler.
#endif

// Static array length.

#define abArrayLength(arr) (sizeof(arr) / (sizeof((arr)[0u])))

// Stack allocation.

#if defined(abPlatform_OS_Windows)
#include <malloc.h>
#define abStackAlloc _alloca
#else
#include <alloca.h>
#define abStackAlloc alloca
#endif

// Common operations on numbers.

#define abMin(a, b) ((a) < (b) ? (a) : (b))
#define abMax(a, b) ((a) > (b) ? (a) : (b))
#define abClamp(value, low, high) (((value) > (high)) ? (high) : (((value) < (low)) ? (low) : (value)))
#define abAlign(value, alignment) (((value) + ((alignment) - 1u)) & ~((alignment) - 1u))

#endif
