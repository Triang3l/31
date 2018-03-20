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
#if defined(abPlatform_Compiler_MSVC)
#define abVec4u32_ConstInit(x, y, z, w) { .m128i_u32 = { (x), (y), (z), (w) } }
#define abVec4s32_ConstInit(x, y, z, w) { .m128i_i32 = { (x), (y), (z), (w) } }
#elif defined(abPlatform_Compiler_GNU)
#define abVec4u32_ConstInit(x, y, z, w) { \
	(uint64_t) ((uint32_t) (x)) | ((uint64_t) ((uint32_t) (y)) << 32u), \
	(uint64_t) ((uint32_t) (z)) | ((uint64_t) ((uint32_t) (w)) << 32u) \
}
#define abVec4s32_ConstInit(x, y, z, w) abVec4u32_ConstInit(x, y, z, w)
#else
#error No abVec4u32 and abVec4s32 static initializer known for the current compiler.
#endif

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

#if defined(abPlatform_CPU_x86)
#define abVec4_LoadAligned _mm_load_ps
#define abVec4_StoreAligned _mm_store_ps
#define abVec4_LoadUnaligned _mm_loadu_ps
#define abVec4_StoreUnaligned _mm_storeu_ps
#define abVec4_LoadX4 _mm_set1_ps
#define abVec4_Zero _mm_setzero_ps

abForceInline abVec4 abVec4_YXWZ(abVec4 v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1)); }
abForceInline abVec4 abVec4_ZWXY(abVec4 v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 0, 3, 2)); }

#define abVec4_Add _mm_add_ps
#define abVec4_Subtract _mm_sub_ps
#define abVec4_Negate(v) abVec4_Subtract(abVec4_Zero, (v))
#define abVec4_Multiply _mm_mul_ps
#define abVec4_MultiplyAdd(a, m1, m2) abVec4_Add((a), abVec4_Multiply((m1), (m2)))

#elif defined(abPlatform_CPU_Arm)
#if defined(abPlatform_Compiler_MSVC)
#define abVec4_LoadAligned(p) vld1q_f32_ex((p), 128u)
#define abVec4_StoreAligned(p, v) vst1q_f32_ex((p), (v), 128u)
#elif defined(abPlatform_Compiler_GNU)
#define abVec4_LoadAligned(p) vld1q_f32((const float *) __builtin_assume_aligned((p), 16u))
#define abVec4_StoreAligned(p, v) vst1q_f32((float *) __builtin_assume_aligned((p), 16u), (v))
#else
#error No explicitly aligned vld1q and vst1q known for the current compiler.
#endif
#define abVec4_LoadUnaligned vld1q_f32
#define abVec4_StoreUnaligned vst1q_f32
#define abVec4_LoadX4 vdupq_n_f32
#define abVec4_Zero abVec4_LoadX4(0.0f)

#define abVec4_YXWZ vrev64q_f32
abForceInline abVec4 abVec4_ZWXY(abVec4 v) { return vextq_f32(v, v, 2u); }

#define abVec4_Add vaddq_f32
#define abVec4_Subtract vsubq_f32
#define abVec4_Negate vnegq_f32
#define abVec4_Multiply vmulq_f32
#define abVec4_MultiplyAdd vmlaq_f32
#endif

abForceInline abVec4 abVec4_Dots4(abVec4 a, abVec4 b) {
	abVec4 r = abVec4_Multiply(a, b);
	r = abVec4_Add(r, abVec4_YXWZ(r));
	return abVec4_Add(r, abVec4_ZWXY(r));
}

#endif
