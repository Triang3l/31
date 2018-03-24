#ifndef abInclude_abFeedback
#include "../abCommon.h"
#if defined(abPlatform_OS_Windows)
#include <intrin.h>
#endif

#ifdef _DEBUG
#define abFeedback_DebugBuild 1
#endif

#ifdef abPlatform_Compiler_MSVC
#define abFeedback_StaticAssert static_assert
#else
#define abFeedback_StaticAssert _Static_assert
#endif

#if defined(abPlatform_OS_Windows)
#define abFeedback_Break __debugbreak
#else
#error No abFeedback_Break implementation for the target OS.
#endif

// These are OS-specific.
void abFeedback_DebugMessageForceV(char const * format, va_list arguments);
void abFeedback_CrashV(bool isAssert, char const * functionName, char const * messageFormat, va_list messageArguments);

// Non-OS-specific.
void abFeedback_DebugMessageForce(char const * format, ...);
void abFeedback_Crash(char const * functionName, char const * messageFormat, ...);
void abFeedback_AssertCrash(char const * functionName, char const * messageFormat, ...);
#ifdef abFeedback_DebugBuild
#define abFeedback_DebugMessage(format, ...) abFeedback_DebugMessageForce((format), __VA_ARGS__)
#define abFeedback_Assert(condition, functionName, messageFormat, ...) \
	{ if (!(condition)) abFeedback_AssertCrash((functionName), (messageFormat), __VA_ARGS__); }
#else
#define abFeedback_DebugMessage(format, ...) {}
#define abFeedback_Assert(condition, functionName, messageFormat, ...) {}
#endif

#endif
