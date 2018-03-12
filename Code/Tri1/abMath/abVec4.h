#ifndef abInclude_abMath_abVec4
#define abInclude_abMath_abVec4
#include "../abCommon.h"

/*
 * SIMD 4-component vector.
 */

#if defined(abPlatform_CPU_x86)
#include <emmintrin.h>
typedef __m128 abVec4;
typedef __m128i abVec4u32;
typedef __m128i abVec4s32;
#define abVec4u32_ConstInit(x, y, z, w) { \
	(x) & 255u, ((x) >> 8) & 255u, ((x) >> 16) & 255u, ((x) >> 24) & 255u, \
	(y) & 255u, ((y) >> 8) & 255u, ((y) >> 16) & 255u, ((y) >> 24) & 255u, \
	(z) & 255u, ((z) >> 8) & 255u, ((z) >> 16) & 255u, ((z) >> 24) & 255u, \
	(w) & 255u, ((w) >> 8) & 255u, ((w) >> 16) & 255u, ((w) >> 24) & 255u \
}
#define abVec4s32_ConstInit(x, y, z, w) { \
	(x) & 255, ((x) >> 8) & 255, ((x) >> 16) & 255, ((x) >> 24) & 255, \
	(y) & 255, ((y) >> 8) & 255, ((y) >> 16) & 255, ((y) >> 24) & 255, \
	(z) & 255, ((z) >> 8) & 255, ((z) >> 16) & 255, ((z) >> 24) & 255, \
	(w) & 255, ((w) >> 8) & 255, ((w) >> 16) & 255, ((w) >> 24) & 255 \
}

#elif defined(abPlatform_CPU_Arm)
#include <arm_neon.h>
typedef float32x4_t abVec4;
typedef uint32x4_t abVec4u32;
typedef int32x4_t abVec4s32;
#define abVec4u32_ConstInit(x, y, z, w) { (x), (y), (z), (w) }
#define abVec4s32_ConstInit(x, y, z, w) { (x), (y), (z), (w) }
#else
#error No SIMD abVec4 for the target CPU.
#endif

extern abVec4u32 abVec4_SignMask;
extern abVec4u32 abVec4_AbsMask;

#endif
