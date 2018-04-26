#ifndef abInclude_abMath_abVec
#define abInclude_abMath_abVec
#include "../abCommon.h"

/*
 * 4-component SIMD vector.
 *
 * Comparisons and bit operations return values of the same type as the inputs.
 * For instance, abVec4_Or takes abVec4 arguments and returns abVec4, while abVec4s32_Or takes and returns abVec4s32.
 * While logically they both should return abVec4u32, this is to make usage of the FP SIMD unit or integer SIMD unit more explicit.
 */

#if defined(abPlatform_CPU_x86)

/*
 * SSE2 implementation.
 */

#include <emmintrin.h>

typedef __m128 abVec4;
typedef __m128i abVec4s32;
typedef __m128i abVec4u32;

#if defined(abPlatform_Compiler_MSVC)
#define abVec4s32_ConstInit(x, y, z, w) { .m128i_i32 = { (x), (y), (z), (w) } }
#define abVec4u32_ConstInit(x, y, z, w) { .m128i_u32 = { (x), (y), (z), (w) } }
#define abVec4_AsS32 _mm_castps_si128
#define abVec4s32_AsF32 _mm_castsi128_ps
#elif defined(abPlatform_Compiler_GNU)
#define abVec4s32_ConstInit(x, y, z, w) { \
	(uint64_t) ((uint32_t) (x)) | ((uint64_t) ((uint32_t) (y)) << 32u), \
	(uint64_t) ((uint32_t) (z)) | ((uint64_t) ((uint32_t) (w)) << 32u) \
}
#define abVec4u32_ConstInit(x, y, z, w) abVec4s32_ConstInit((x), (y), (z), (w))
#define abVec4_AsS32(v) ((abVec4s32) (v))
#define abVec4s32_AsF32(v) ((abVec4) (v))
#else
#error No vector casts and abVec4u32/abVec4s32 static initializer known for the current compiler.
#endif
#define abVec4_AsU32(v) abVec4_AsS32((v))
#define abVec4s32_AsU32(v) (v)
#define abVec4u32_AsF32(v) abVec4s32_AsF32((v))
#define abVec4u32_AsS32(v) (v)

#define abVec4_LoadAligned _mm_load_ps
#define abVec4s32_LoadAligned(p) _mm_load_si128((__m128i const *) (p))
#define abVec4u32_LoadAligned(p) abVec4s32_LoadAligned((p))
#define abVec4_StoreAligned _mm_store_ps
#define abVec4s32_StoreAligned(p, v) _mm_store_si128((__m128i *) (p), (v))
#define abVec4u32_StoreAligned(p, v) abVec4s32_StoreAligned((p), (v))
#define abVec4_LoadUnaligned _mm_loadu_ps
#define abVec4s32_LoadUnaligned(p) _mm_loadu_si128((__m128i const *) (p))
#define abVec4u32_LoadUnaligned(p) abVec4s32_LoadUnaligned((p))
#define abVec4_StoreUnaligned _mm_storeu_ps
#define abVec4s32_StoreUnaligned(p, v) _mm_storeu_si128((__m128i *) (p), (v))
#define abVec4u32_StoreUnaligned(p, v) abVec4s32_StoreUnaligned((p), (v))
#define abVec4_LoadX4 _mm_set1_ps
#define abVec4s32_LoadX4 _mm_set1_epi32
#define abVec4u32_LoadX4(i) abVec4s32_LoadX4((int32_t) (i))
abForceInline float abVec4_GetX(abVec4 v) { float x; _mm_store_ss(&x, v); return x; }
abForceInline int32_t abVec4s32_GetX(abVec4s32 v) { int32_t x; _mm_store_ss((float *) &x, abVec4s32_AsF32(v)); return x; }
abForceInline uint32_t abVec4u32_GetX(abVec4u32 v) { uint32_t x; _mm_store_ss((float *) &x, abVec4u32_AsF32(v)); return x; }
#define abVec4_Zero _mm_setzero_ps()
#define abVec4s32_Zero _mm_setzero_si128()
#define abVec4u32_Zero abVec4s32_Zero

#define abVec4i_SSE_MakeSwizzle(name, x, y, z, w) \
	abForceInline abVec4 abVec4_##name(abVec4 v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE((w), (z), (y), (x))); } \
	abForceInline abVec4s32 abVec4s32_##name(abVec4s32 v) { return _mm_shuffle_epi32(v, _MM_SHUFFLE((w), (z), (y), (x))); } \
	abForceInline abVec4u32 abVec4u32_##name(abVec4u32 v) { return _mm_shuffle_epi32(v, _MM_SHUFFLE((w), (z), (y), (x))); }
abVec4i_SSE_MakeSwizzle(YXWZ, 1, 0, 3, 2)
abVec4i_SSE_MakeSwizzle(ZWXY, 2, 3, 0, 1)

#define abVec4_Add _mm_add_ps
#define abVec4s32_Add _mm_add_epi32
#define abVec4u32_Add abVec4s32_Add
#define abVec4_Subtract _mm_sub_ps
#define abVec4s32_Subtract _mm_sub_epi32
#define abVec4u32_Subtract abVec4s32_Subtract
#define abVec4_Negate(v) abVec4_Subtract(abVec4_Zero, (v))
#define abVec4_Multiply _mm_mul_ps
#define abVec4_MultiplyAdd_Separate 1 // Add depends on the result of Multiply, prefer separating them if possible.
#define abVec4_MultiplyAdd(a, m1, m2) abVec4_Add((a), abVec4_Multiply((m1), (m2)))
#define abVec4_MultiplySubtract(a, m1, m2) abVec4_Subtract((a), abVec4_Multiply((m1), (m2)))
#define abVec4_Min _mm_min_ps
#define abVec4_Max _mm_min_ps

#define abVec4_And _mm_and_ps
#define abVec4s32_And _mm_and_si128
#define abVec4u32_And abVec4s32_And
#define abVec4_AndNot _mm_andnot_ps
#define abVec4s32_AndNot _mm_andnot_si128
#define abVec4u32_AndNot abVec4s32_AndNot
#define abVec4_Or _mm_or_ps
#define abVec4s32_Or _mm_or_si128
#define abVec4u32_Or abVec4s32_Or
#define abVec4_Xor _mm_xor_ps
#define abVec4s32_Xor _mm_xor_si128
#define abVec4u32_Xor abVec4s32_Xor

#define abVec4_Equal _mm_cmpeq_ps
#define abVec4s32_Equal _mm_cmpeq_epi32
#define abVec4u32_Equal abVec4s32_Equal
#define abVec4_NotEqual _mm_cmpneq_ps
#define abVec4s32_NotEqual _mm_cmpneq_epi32
#define abVec4u32_NotEqual abVec4s32_NotEqual
// #undef abVec4u32_ComparisonsAvailable
#define abVec4_Less _mm_cmplt_ps
#define abVec4s32_Less _mm_cmplt_epi32
#define abVec4_LessEqual _mm_cmple_ps
#define abVec4_Greater _mm_cmpgt_ps
#define abVec4s32_Greater _mm_cmpgt_epi32
#define abVec4_GreaterEqual _mm_cmpge_ps

#elif defined(abPlatform_CPU_Arm)

/*
 * NEON implementation.
 */

#include <arm_neon.h>

typedef float32x4_t abVec4;
typedef int32x4_t abVec4s32;
typedef uint32x4_t abVec4u32;

#define abVec4s32_ConstInit(x, y, z, w) { (x), (y), (z), (w) }
#define abVec4u32_ConstInit(x, y, z, w) { (x), (y), (z), (w) }

#define abVec4_AsU32 vreinterpretq_s32_f32
#define abVec4_AsS32 vreinterpretq_u32_f32
#define abVec4s32_AsF32 vreinterpretq_f32_s32
#define abVec4s32_AsU32 vreinterpretq_u32_s32
#define abVec4u32_AsF32 vreinterpretq_f32_u32
#define abVec4u32_AsS32 vreinterpretq_s32_u32

#if defined(abPlatform_Compiler_MSVC)
#define abVec4_LoadAligned(p) vld1q_f32_ex((p), 128u)
#define abVec4s32_LoadAligned(p) vld1q_s32_ex((p), 128u)
#define abVec4u32_LoadAligned(p) vld1q_u32_ex((p), 128u)
#define abVec4_StoreAligned(p, v) vst1q_f32_ex((p), (v), 128u)
#define abVec4s32_StoreAligned(p, v) vst1q_s32_ex((p), (v), 128u)
#define abVec4u32_StoreAligned(p, v) vst1q_u32_ex((p), (v), 128u)
#elif defined(abPlatform_Compiler_GNU)
#define abVec4_LoadAligned(p) vld1q_f32((float const *) __builtin_assume_aligned((p), 16u))
#define abVec4s32_LoadAligned(p) vld1q_s32((int32_t const *) __builtin_assume_aligned((p), 16u))
#define abVec4u32_LoadAligned(p) vld1q_u32((uint32_t const *) __builtin_assume_aligned((p), 16u))
#define abVec4_StoreAligned(p, v) vst1q_f32((float *) __builtin_assume_aligned((p), 16u), (v))
#define abVec4s32_StoreAligned(p, v) vst1q_s32((int32_t *) __builtin_assume_aligned((p), 16u), (v))
#define abVec4u32_StoreAligned(p, v) vst1q_u32((uint32_t *) __builtin_assume_aligned((p), 16u), (v))
#else
#error No explicitly aligned vld1q and vst1q known for the current compiler.
#endif
#define abVec4_LoadUnaligned vld1q_f32
#define abVec4s32_LoadUnaligned vld1q_s32
#define abVec4u32_LoadUnaligned vld1q_u32
#define abVec4_StoreUnaligned vst1q_f32
#define abVec4s32_StoreUnaligned vst1q_s32
#define abVec4u32_StoreUnaligned vst1q_u32
#define abVec4_LoadX4 vdupq_n_f32
#define abVec4s32_LoadX4 vdupq_n_s32
#define abVec4u32_LoadX4 vdupq_n_u32
#define abVec4_GetX(v) vgetq_lane_f32((v), 0u)
#define abVec4s32_GetX(v) vgetq_lane_s32((v), 0u)
#define abVec4u32_GetX(v) vgetq_lane_u32((v), 0u)
#define abVec4_Zero abVec4_LoadX4(0.0f)
#define abVec4s32_Zero abVec4s32_LoadX4(0)
#define abVec4u32_Zero abVec4u32_LoadX4(0u)

#define abVec4_YXWZ vrev64q_f32
#define abVec4s32_YXWZ vrev64q_s32
#define abVec4u32_YXWZ vrev64q_u32
abForceInline abVec4 abVec4_ZWXY(abVec4 v) { return vextq_f32(v, v, 2u); }
abForceInline abVec4s32 abVec4s32_ZWXY(abVec4s32 v) { return vextq_s32(v, v, 2u); }
abForceInline abVec4u32 abVec4u32_ZWXY(abVec4u32 v) { return vextq_u32(v, v, 2u); }

#define abVec4_Add vaddq_f32
#define abVec4s32_Add vaddq_s32
#define abVec4u32_Add vaddq_u32
#define abVec4_Subtract vsubq_f32
#define abVec4s32_Subtract vsubq_s32
#define abVec4u32_Subtract vsubq_u32
#define abVec4_Negate vnegq_f32
#define abVec4_Multiply vmulq_f32
// #undef abVec4_MultiplyAdd_Separate
#define abVec4_MultiplyAdd vmlaq_f32
#define abVec4_MultiplySubtract vmlsq_f32
#define abVec4_Min vminq_f32
#define abVec4_Max vmaxq_f32

#define abVec4s32_And vandq_s32
#define abVec4u32_And vandq_u32
#define abVec4_And(a, b) abVec4u32_AsF32(abVec4u32_And(abVec4_AsU32(a), abVec4_AsU32(b)))
#define abVec4s32_AndNot vbicq_s32
#define abVec4u32_AndNot vbicq_u32
#define abVec4_AndNot(a, b) abVec4u32_AsF32(abVec4u32_AndNot(abVec4_AsU32(a), abVec4_AsU32(b)))
#define abVec4s32_Or vorrq_s32
#define abVec4u32_Or vorrq_u32
#define abVec4_Or(a, b) abVec4u32_AsF32(abVec4u32_Or(abVec4_AsU32(a), abVec4_AsU32(b)))
#define abVec4s32_Xor veorq_s32
#define abVec4u32_Xor veorq_u32
#define abVec4_Xor(a, b) abVec4u32_AsF32(abVec4u32_Xor(abVec4_AsU32(a), abVec4_AsU32(b)))

#define abVec4_Equal(a, b) abVec4u32_AsF32(vceqq_f32((a), (b)))
#define abVec4s32_Equal(a, b) abVec4u32_AsS32(vceqq_s32((a), (b)))
#define abVec4u32_Equal vceqq_u32
#define abVec4_NotEqual(a, b) abVec4u32_AsF32(vmvnq_u32(vceqq_f32((a), (b))))
#define abVec4s32_NotEqual(a, b) abVec4u32_AsS32(vmvnq_u32(vceqq_s32((a), (b))))
#define abVec4u32_NotEqual(a, b) vmvnq_u32(vceqq_u32((a), (b)))
#define abVec4u32_ComparisonAvailable 1
#define abVec4_Less(a, b) abVec4u32_AsF32(vcltq_f32((a), (b)))
#define abVec4s32_Less(a, b) abVec4u32_AsS32(vcltq_s32((a), (b)))
#define abVec4u32_Less vcltq_u32
#define abVec4_LessEqual(a, b) abVec4u32_AsF32(vcleq_f32((a), (b)))
#define abVec4_Greater(a, b) abVec4u32_AsF32(vcgtq_f32((a), (b)))
#define abVec4s32_Greater(a, b) abVec4u32_AsS32(vcgtq_s32((a), (b)))
#define abVec4u32_Greater vcgtq_u32
#define abVec4_GreaterEqual(a, b) abVec4u32_AsF32(vcgeq_f32((a), (b)))

#else
#error No SIMD abVec4 for the target CPU.
#endif

abForceInline abVec4 abVec4_Dots4(abVec4 a, abVec4 b) {
	abVec4 r = abVec4_Multiply(a, b);
	r = abVec4_Add(r, abVec4_YXWZ(r));
	return abVec4_Add(r, abVec4_ZWXY(r));
}

#endif
