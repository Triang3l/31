#ifndef abInclude_abMath_abVec
#define abInclude_abMath_abVec
#include "../abCommon.h"

/*
 * 128-bit SIMD vectors.
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
typedef __m128i abVec8s16;
typedef __m128i abVec8u16;
typedef __m128i abVec16s8;
typedef __m128i abVec16u8;

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
#define abVec8s16_AsU16(v) (v)
#define abVec8u16_AsS16(v) (v)
#define abVec16s8_AsU8(v) (v)
#define abVec16u8_AsS8(v) (v)

#define abVec4_LoadAligned _mm_load_ps
#define abVec4s32_LoadAligned(p) _mm_load_si128((__m128i const *) (p))
#define abVec4u32_LoadAligned(p) abVec4s32_LoadAligned((p))
#define abVec8s16_LoadAligned(p) abVec4s32_LoadAligned((p))
#define abVec8u16_LoadAligned(p) abVec4s32_LoadAligned((p))
#define abVec16s8_LoadAligned(p) abVec4s32_LoadAligned((p))
#define abVec16u8_LoadAligned(p) abVec4s32_LoadAligned((p))
#define abVec4_StoreAligned _mm_store_ps
#define abVec4s32_StoreAligned(p, v) _mm_store_si128((__m128i *) (p), (v))
#define abVec4u32_StoreAligned(p, v) abVec4s32_StoreAligned((p), (v))
#define abVec8s16_StoreAligned(p, v) abVec4s32_StoreAligned((p), (v))
#define abVec8u16_StoreAligned(p, v) abVec4s32_StoreAligned((p), (v))
#define abVec16s8_StoreAligned(p, v) abVec4s32_StoreAligned((p), (v))
#define abVec16u8_StoreAligned(p, v) abVec4s32_StoreAligned((p), (v))
#define abVec4_LoadUnaligned _mm_loadu_ps
#define abVec4s32_LoadUnaligned(p) _mm_loadu_si128((__m128i const *) (p))
#define abVec4u32_LoadUnaligned(p) abVec4s32_LoadUnaligned((p))
#define abVec8s16_LoadUnaligned(p) abVec4s32_LoadUnaligned((p))
#define abVec8u16_LoadUnaligned(p) abVec4s32_LoadUnaligned((p))
#define abVec16s8_LoadUnaligned(p) abVec4s32_LoadUnaligned((p))
#define abVec16u8_LoadUnaligned(p) abVec4s32_LoadUnaligned((p))
#define abVec4_StoreUnaligned _mm_storeu_ps
#define abVec4s32_StoreUnaligned(p, v) _mm_storeu_si128((__m128i *) (p), (v))
#define abVec4u32_StoreUnaligned(p, v) abVec4s32_StoreUnaligned((p), (v))
#define abVec8s16_StoreUnaligned(p, v) abVec4s32_StoreUnaligned((p), (v))
#define abVec8u16_StoreUnaligned(p, v) abVec4s32_StoreUnaligned((p), (v))
#define abVec16s8_StoreUnaligned(p, v) abVec4s32_StoreUnaligned((p), (v))
#define abVec16u8_StoreUnaligned(p, v) abVec4s32_StoreUnaligned((p), (v))
#define abVec4_LoadX4 _mm_set1_ps
#define abVec4s32_LoadX4 _mm_set1_epi32
#define abVec4u32_LoadX4(i) abVec4s32_LoadX4((int32_t) (i))
#define abVec8s16_LoadX8 _mm_set1_epi16
#define abVec8u16_LoadX8(i) abVec8s16_LoadX8((int16_t) (i))
#define abVec16s8_LoadX16 _mm_set1_epi8
#define abVec16u8_LoadX16(i) abVec16s8_LoadX16((int8_t) (i))
abForceInline float abVec4_GetX(abVec4 v) { float x; _mm_store_ss(&x, v); return x; }
abForceInline int32_t abVec4s32_GetX(abVec4s32 v) { int32_t x; _mm_store_ss((float *) &x, abVec4s32_AsF32(v)); return x; }
abForceInline uint32_t abVec4u32_GetX(abVec4u32 v) { uint32_t x; _mm_store_ss((float *) &x, abVec4u32_AsF32(v)); return x; }
#define abVec4_Zero _mm_setzero_ps()
#define abVec4s32_Zero _mm_setzero_si128()
#define abVec4u32_Zero abVec4s32_Zero
#define abVec8s16_Zero abVec4s32_Zero
#define abVec8u16_Zero abVec4s32_Zero
#define abVec16s8_Zero abVec4s32_Zero
#define abVec16u8_Zero abVec4s32_Zero

#define abVec4i_SSE_MakeSwizzle(name, x, y, z, w) \
	abForceInline abVec4 abVec4_##name(abVec4 v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE((w), (z), (y), (x))); } \
	abForceInline abVec4s32 abVec4s32_##name(abVec4s32 v) { return _mm_shuffle_epi32(v, _MM_SHUFFLE((w), (z), (y), (x))); } \
	abForceInline abVec4u32 abVec4u32_##name(abVec4u32 v) { return _mm_shuffle_epi32(v, _MM_SHUFFLE((w), (z), (y), (x))); }
abVec4i_SSE_MakeSwizzle(YXWZ, 1u, 0u, 3u, 2u)
abVec4i_SSE_MakeSwizzle(ZWXY, 2u, 3u, 0u, 1u)

#define abVec4_Add _mm_add_ps
#define abVec4s32_Add _mm_add_epi32
#define abVec4u32_Add abVec4s32_Add
#define abVec8s16_Add _mm_add_epi16
#define abVec8u16_Add abVec8s16_Add
#define abVec16s8_Add _mm_add_epi8
#define abVec16u8_Add abVec16s8_Add
#define abVec4_Subtract _mm_sub_ps
#define abVec4s32_Subtract _mm_sub_epi32
#define abVec4u32_Subtract abVec4s32_Subtract
#define abVec8s16_Subtract _mm_sub_epi16
#define abVec8u16_Subtract abVec8s16_Subtract
#define abVec16s8_Subtract _mm_sub_epi8
#define abVec16u8_Subtract abVec16s8_Subtract
#define abVec4_Negate(v) abVec4_Subtract(abVec4_Zero, (v))
#define abVec4_Multiply _mm_mul_ps
#define abVec4_MultiplyAdd_Separate 1 // Add depends on the result of Multiply, prefer separating them if possible.
#define abVec4_MultiplyAdd(a, m1, m2) abVec4_Add((a), abVec4_Multiply((m1), (m2)))
#define abVec4_MultiplySubtract(a, m1, m2) abVec4_Subtract((a), abVec4_Multiply((m1), (m2)))
#define abVec4_Min _mm_min_ps
#define abVec4_Max _mm_max_ps
#define abVec8s16_Min _mm_min_epi16
#define abVec8s16_Max _mm_max_epi16
// #undef abVec8u16_MinMax_Available
// #undef abVec16s8_MinMax_Available
#define abVec16u8_Min _mm_min_epu8
#define abVec16u8_Max _mm_max_epu8

// The Two versions are to break dependencies.
#define abVec4_ReciprocalCoarse _mm_rcp_ps
#define abVec4_ReciprocalFine(v) _mm_div_ps(abVec4_LoadX4(1.0f), (v))
abForceInline abVec4_ReciprocalTwoFine(abVec4 den1, abVec4 den2, abVec4 * result1, abVec4 * result2) {
	abVec4 const ones = abVec4_LoadX4(1.0f);
	*result1 = _mm_div_ps(ones, den1);
	*result2 = _mm_div_ps(ones, den2);
}
#define abVec4_DivideFine _mm_div_ps
abForceInline void abVec4_DivideTwoFine(abVec4 num1, abVec4 num2, abVec4 den1, abVec4 den2, abVec4 * result1, abVec4 * result2) {
	*result1 = _mm_div_ps(num1, den1);
	*result2 = _mm_div_ps(num2, den2);
}
#define abVec4_RSqrtCoarse _mm_rsqrt_ps
abForceInline abVec4 abVec4_RSqrtFine(abVec4 v) {
	abVec4 r = abVec4_RSqrtCoarse(v);
	// -0.5r * (xr^2 - 3). Around 22 bit precision.
	return abVec4_Multiply(abVec4_Multiply(abVec4_LoadX4(-0.5f), r), abVec4_MultiplyAdd(abVec4_LoadX4(-3.0f), v, abVec4_Multiply(r, r)));
}
abForceInline void abVec4_RSqrtTwoFine(abVec4 v1, abVec4 v2, abVec4 * result1, abVec4 * result2) {
	abVec4 const mthrees = abVec4_LoadX4(-3.0f), mhalves = abVec4_LoadX4(-0.5f);
	abVec4 r1 = abVec4_RSqrtCoarse(v1), r2 = abVec4_RSqrtCoarse(v2);
	abVec4 t1 = abVec4_Multiply(r1, r1), t2 = abVec4_Multiply(r2, r2);
	t1 = abVec4_Multiply(t1, v1);
	t2 = abVec4_Multiply(t2, v2);
	t1 = abVec4_Add(t1, mthrees);
	t2 = abVec4_Add(t2, mthrees);
	*result1 = abVec4_Multiply(t1, abVec4_Multiply(r1, mhalves));
	*result2 = abVec4_Multiply(t2, abVec4_Multiply(r2, mhalves));
}
#define abVec4_SqrtFine _mm_sqrt_ps
abForceInline void abVec4_SqrtTwoFine(abVec4 v1, abVec4 v2, abVec4 * result1, abVec4 * result2) {
	*result1 = _mm_sqrt_ps(v1);
	*result2 = _mm_sqrt_ps(v2);
}

#define abVec4_And _mm_and_ps
#define abVec4s32_And _mm_and_si128
#define abVec4u32_And abVec4s32_And
#define abVec8s16_And abVec4s32_And
#define abVec8u16_And abVec4s32_And
#define abVec16s8_And abVec4s32_And
#define abVec16u8_And abVec4s32_And
#define abVec4_AndNot _mm_andnot_ps
#define abVec4s32_AndNot _mm_andnot_si128
#define abVec4u32_AndNot abVec4s32_AndNot
#define abVec8s16_AndNot abVec4s32_AndNot
#define abVec8u16_AndNot abVec4s32_AndNot
#define abVec16s8_AndNot abVec4s32_AndNot
#define abVec16u8_AndNot abVec4s32_AndNot
#define abVec4_Or _mm_or_ps
#define abVec4s32_Or _mm_or_si128
#define abVec4u32_Or abVec4s32_Or
#define abVec8s16_Or abVec4s32_Or
#define abVec8u16_Or abVec4s32_Or
#define abVec16s8_Or abVec4s32_Or
#define abVec16u8_Or abVec4s32_Or
#define abVec4_Xor _mm_xor_ps
#define abVec4s32_Xor _mm_xor_si128
#define abVec4u32_Xor abVec4s32_Xor
#define abVec8s16_Xor abVec4s32_Xor
#define abVec8u16_Xor abVec4s32_Xor
#define abVec16s8_Xor abVec4s32_Xor
#define abVec16u8_Xor abVec4s32_Xor
#define abVec4s32_ShiftLeftConst _mm_slli_epi32
#define abVec4u32_ShiftLeftConst abVec4s32_ShiftLeftConst
#define abVec4s32_ShiftRightConst _mm_srai_epi32
#define abVec4u32_ShiftRightConst _mm_srli_epi32
#define abVec8s16_ShiftLeftConst _mm_slli_epi16
#define abVec8u16_ShiftLeftConst abVec8s16_ShiftLeftConst
#define abVec8s16_ShiftRightConst _mm_srai_epi16
#define abVec8u16_ShiftRightConst _mm_srli_epi16

#define abVec4_Equal _mm_cmpeq_ps
#define abVec4s32_Equal _mm_cmpeq_epi32
#define abVec4u32_Equal abVec4s32_Equal
#define abVec8s16_Equal _mm_cmpeq_epi16
#define abVec8u16_Equal abVec8s16_Equal
#define abVec16s8_Equal _mm_cmpeq_epi8
#define abVec16u8_Equal abVec16s8_Equal
#define abVec4_NotEqual _mm_cmpneq_ps
// #undef abVec4u_Comparison_Available
#define abVec4_Less _mm_cmplt_ps
#define abVec4s32_Less _mm_cmplt_epi32
#define abVec8s16_Less _mm_cmplt_epi16
#define abVec16s8_Less _mm_cmplt_epi8
#define abVec4_LessEqual _mm_cmple_ps
#define abVec4_Greater _mm_cmpgt_ps
#define abVec4s32_Greater _mm_cmpgt_epi32
#define abVec8s16_Greater _mm_cmpgt_epi16
#define abVec16s8_Greater _mm_cmpgt_epi8
#define abVec4_GreaterEqual _mm_cmpge_ps

#define abVec4_ToS32 _mm_cvtps_epi32
#define abVec4s32_ToF32 _mm_cvtepi32_ps
#define abVec4s32_ToS16Saturate _mm_packs_epi32
abForceInline abVec4s32 abVec8s16_ToS32Low(abVec8s16 v) { return abVec4s32_ShiftRightConst(_mm_unpacklo_epi16(v, v), 16u); }
abForceInline abVec4s32 abVec8s16_ToS32High(abVec8s16 v) { return abVec4s32_ShiftRightConst(_mm_unpackhi_epi16(v, v), 16u); }
#define abVec8s16_ToS8Saturate _mm_packs_epi16
#define abVec8u16_ToU32Low(v) _mm_unpacklo_epi16((v), abVec8u16_Zero)
#define abVec8u16_ToU32High(v) _mm_unpackhi_epi16((v), abVec8u16_Zero)
#define abVec8u16_ToU8Saturate _mm_packus_epi16
abForceInline abVec8s16 abVec16s8_ToS16Low(abVec16s8 v) { return abVec8s16_ShiftRightConst(_mm_unpacklo_epi8(v, v), 8u); }
abForceInline abVec8s16 abVec16s8_ToS16High(abVec16s8 v) { return abVec8s16_ShiftRightConst(_mm_unpackhi_epi8(v, v), 8u); }
#define abVec16u8_ToU16Low(v) _mm_unpacklo_epi8((v), abVec16u8_Zero)
#define abVec16u8_ToU16High(v) _mm_unpackhi_epi8((v), abVec16u8_Zero)

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
#define abVec8s16_AsU16 vreinterpretq_u16_s16
#define abVec8u16_AsS16 vreinterpretq_s16_u16
#define abVec16s8_AsU8 vreinterpretq_u8_s8
#define abVec16u8_AsS8 vreinterpretq_s8_u8

#if defined(abPlatform_Compiler_MSVC)
#define abVec4_LoadAligned(p) vld1q_f32_ex((p), 128u)
#define abVec4s32_LoadAligned(p) vld1q_s32_ex((p), 128u)
#define abVec4u32_LoadAligned(p) vld1q_u32_ex((p), 128u)
#define abVec8s16_LoadAligned(p) vld1q_s16_ex((p), 128u)
#define abVec8u16_LoadAligned(p) vld1q_u16_ex((p), 128u)
#define abVec16s8_LoadAligned(p) vld1q_s8_ex((p), 128u)
#define abVec16u8_LoadAligned(p) vld1q_u8_ex((p), 128u)
#define abVec4_StoreAligned(p, v) vst1q_f32_ex((p), (v), 128u)
#define abVec4s32_StoreAligned(p, v) vst1q_s32_ex((p), (v), 128u)
#define abVec4u32_StoreAligned(p, v) vst1q_u32_ex((p), (v), 128u)
#define abVec8s16_StoreAligned(p, v) vst1q_s16_ex((p), (v), 128u)
#define abVec8u16_StoreAligned(p, v) vst1q_u16_ex((p), (v), 128u)
#define abVec16s8_StoreAligned(p, v) vst1q_s8_ex((p), (v), 128u)
#define abVec16u8_StoreAligned(p, v) vst1q_u8_ex((p), (v), 128u)
#elif defined(abPlatform_Compiler_GNU)
#define abVec4_LoadAligned(p) vld1q_f32((float const *) __builtin_assume_aligned((p), 16u))
#define abVec4s32_LoadAligned(p) vld1q_s32((int32_t const *) __builtin_assume_aligned((p), 16u))
#define abVec4u32_LoadAligned(p) vld1q_u32((uint32_t const *) __builtin_assume_aligned((p), 16u))
#define abVec8s16_LoadAligned(p) vld1q_s16((int16_t const *) __builtin_assume_aligned((p), 16u))
#define abVec8u16_LoadAligned(p) vld1q_u16((uint16_t const *) __builtin_assume_aligned((p), 16u))
#define abVec16s8_LoadAligned(p) vld1q_s8((int8_t const *) __builtin_assume_aligned((p), 16u))
#define abVec16u8_LoadAligned(p) vld1q_u8((uint8_t const *) __builtin_assume_aligned((p), 16u))
#define abVec4_StoreAligned(p, v) vst1q_f32((float *) __builtin_assume_aligned((p), 16u), (v))
#define abVec4s32_StoreAligned(p, v) vst1q_s32((int32_t *) __builtin_assume_aligned((p), 16u), (v))
#define abVec4u32_StoreAligned(p, v) vst1q_u32((uint32_t *) __builtin_assume_aligned((p), 16u), (v))
#define abVec8s16_StoreAligned(p, v) vst1q_s16((int16_t *) __builtin_assume_aligned((p), 16u), (v))
#define abVec8u16_StoreAligned(p, v) vst1q_u16((uint16_t *) __builtin_assume_aligned((p), 16u), (v))
#define abVec16s8_StoreAligned(p, v) vst1q_s8((int8_t *) __builtin_assume_aligned((p), 16u), (v))
#define abVec16u8_StoreAligned(p, v) vst1q_u8((uint8_t *) __builtin_assume_aligned((p), 16u), (v))
#else
#error No explicitly aligned vld1q and vst1q known for the current compiler.
#endif
#define abVec4_LoadUnaligned vld1q_f32
#define abVec4s32_LoadUnaligned vld1q_s32
#define abVec4u32_LoadUnaligned vld1q_u32
#define abVec8s16_LoadUnaligned vld1q_s16
#define abVec8u16_LoadUnaligned vld1q_u16
#define abVec16s8_LoadUnaligned vld1q_s8
#define abVec16u8_LoadUnaligned vld1q_u8
#define abVec4_StoreUnaligned vst1q_f32
#define abVec4s32_StoreUnaligned vst1q_s32
#define abVec4u32_StoreUnaligned vst1q_u32
#define abVec8s16_StoreUnaligned vst1q_s16
#define abVec8s16_StoreUnaligned vst1q_u16
#define abVec16s8_StoreUnaligned vst1q_s8
#define abVec16s8_StoreUnaligned vst1q_u8
#define abVec4_LoadX4 vdupq_n_f32
#define abVec4s32_LoadX4 vdupq_n_s32
#define abVec4u32_LoadX4 vdupq_n_u32
#define abVec8s16_LoadX8 vdupq_n_s16
#define abVec8u16_LoadX8 vdupq_n_u16
#define abVec16s8_LoadX16 vdupq_n_s8
#define abVec16u8_LoadX16 vdupq_n_u8
#define abVec4_GetX(v) vgetq_lane_f32((v), 0u)
#define abVec4s32_GetX(v) vgetq_lane_s32((v), 0u)
#define abVec4u32_GetX(v) vgetq_lane_u32((v), 0u)
#define abVec4_Zero abVec4_LoadX4(0.0f)
#define abVec4s32_Zero abVec4s32_LoadX4(0)
#define abVec4u32_Zero abVec4u32_LoadX4(0u)
#define abVec8s16_Zero abVec8s16_LoadX8(0)
#define abVec8u16_Zero abVec8u16_LoadX8(0u)
#define abVec16s8_Zero abVec16s8_LoadX16(0)
#define abVec16u8_Zero abVec16u8_LoadX16(0u)

#define abVec4_YXWZ vrev64q_f32
#define abVec4s32_YXWZ vrev64q_s32
#define abVec4u32_YXWZ vrev64q_u32
abForceInline abVec4 abVec4_ZWXY(abVec4 v) { return vextq_f32(v, v, 2u); }
abForceInline abVec4s32 abVec4s32_ZWXY(abVec4s32 v) { return vextq_s32(v, v, 2u); }
abForceInline abVec4u32 abVec4u32_ZWXY(abVec4u32 v) { return vextq_u32(v, v, 2u); }

#define abVec4_Add vaddq_f32
#define abVec4s32_Add vaddq_s32
#define abVec4u32_Add vaddq_u32
#define abVec8s16_Add vaddq_s16
#define abVec8u16_Add vaddq_u16
#define abVec16s8_Add vaddq_s8
#define abVec16u8_Add vaddq_u8
#define abVec4_Subtract vsubq_f32
#define abVec4s32_Subtract vsubq_s32
#define abVec4u32_Subtract vsubq_u32
#define abVec8s16_Subtract vsubq_s16
#define abVec8u16_Subtract vsubq_u16
#define abVec16s8_Subtract vsubq_s8
#define abVec16u8_Subtract vsubq_u8
#define abVec4_Negate vnegq_f32
#define abVec4_Multiply vmulq_f32
// #undef abVec4_MultiplyAdd_Separate
#define abVec4_MultiplyAdd vmlaq_f32
#define abVec4_MultiplySubtract vmlsq_f32
#define abVec4_Min vminq_f32
#define abVec4_Max vmaxq_f32
#define abVec8s16_Min vminq_s16
#define abVec8s16_Max vmaxq_s16
#define abVec8u16_MinMax_Available 1
#define abVec8u16_Min vminq_u16
#define abVec8u16_Max vmaxq_u16
#define abVec16s8_MinMax_Available 1
#define abVec16s8_Min vminq_s8
#define abVec16s8_Max vmaxq_s8
#define abVec16u8_Min vminq_u8
#define abVec16u8_Max vmaxq_u8

// The Two versions are to break dependencies.
#define abVec4_ReciprocalCoarse vrecpeq_f32
abForceInline abVec4 abVec4_ReciprocalFine(abVec4 v) {
	abVec4 r = abVec4_ReciprocalCoarse(v);
	r = abVec4_Multiply(r, vrecpsq_f32(v, r));
	return abVec4_Multiply(r, vrecpsq_f32(v, r));
}
abForceInline abVec4_ReciprocalTwoFine(abVec4 den1, abVec4 den2, abVec4 * result1, abVec4 * result2) {
	abVec4 r1 = abVec4_ReciprocalCoarse(den1), r2 = abVec4_ReciprocalCoarse(den2);
	abVec4 s1 = vrecpsq_f32(den1, r1), s2 = vrecpsq_f32(den2, r2);
	r1 = abVec4_Multiply(r1, s1);
	r2 = abVec4_Multiply(r2, s2);
	s1 = vrecpsq_f32(den1, r1);
	s2 = vrecpsq_f32(den2, r2);
	*result1 = abVec4_Multiply(r1, s1);
	*result2 = abVec4_Multiply(r2, s2);
}
#define abVec4_DivideFine(num, den) abVec4_Multiply((num), abVec4_ReciprocalFine((den)))
abForceInline void abVec4_DivideTwoFine(abVec4 num1, abVec4 num2, abVec4 den1, abVec4 den2, abVec4 * result1, abVec4 * result2) {
	abVec4 r1, r2;
	abVec4_ReciprocalTwoFine(den1, den2, &r1, &r2);
	*result1 = abVec4_Multiply(num1, r1);
	*result2 = abVec4_Multiply(num2, r2);
}
#define abVec4_RSqrtCoarse vrsqrteq_f32
abForceInline abVec4 abVec4_RSqrtFine(abVec4 v) {
	abVec4 r = abVec4_RSqrtCoarse(v);
	r = abVec4_Multiply(r, vrsqrtsq_f32(abVec4_Multiply(r, r), v));
	return abVec4_Multiply(r, vrsqrtsq_f32(abVec4_Multiply(r, r), v));
}
abForceInline void abVec4_RSqrtTwoFine(abVec4 v1, abVec4 v2, abVec4 * result1, abVec4 * result2) {
	abVec4 r1 = abVec4_RSqrtCoarse(v1), r2 = abVec4_RSqrtCoarse(v2);
	abVec4 rr1 = abVec4_Multiply(r1, r1), rr2 = abVec4_Multiply(r2, r2);
	abVec4 s1 = vrsqrtsq_f32(rr1, v1), s2 = vrsqrtsq_f32(rr2, v2);
	r1 = abVec4_Multiply(r1, s1);
	r2 = abVec4_Multiply(r2, s2);
	rr1 = abVec4_Multiply(r1, r1);
	rr2 = abVec4_Multiply(r2, r2);
	s1 = vrsqrtsq_f32(rr1, v1);
	s2 = vrsqrtsq_f32(rr2, v2);
	*result1 = abVec4_Multiply(r1, s1);
	*result2 = abVec4_Multiply(r2, s2);
}
abForceInline abVec4 abVec4_SqrtFine(abVec4 v) { return abVec4_Multiply(abVec4_RSqrtFine(v), v); }
abForceInline void abVec4_SqrtTwoFine(abVec4 v1, abVec4 v2, abVec4 * result1, abVec4 * result2) {
	abVec4 r1, r2;
	abVec4_RSqrtTwoFine(v1, v2, &r1, &r2);
	*result1 = abVec4_Multiply(r1, v1);
	*result2 = abVec4_Multiply(r2, v2);
}

#define abVec4s32_And vandq_s32
#define abVec4u32_And vandq_u32
#define abVec8s16_And vandq_s16
#define abVec8u16_And vandq_u16
#define abVec16s8_And vandq_s8
#define abVec16u8_And vandq_u8
#define abVec4_And(a, b) abVec4u32_AsF32(abVec4u32_And(abVec4_AsU32((a)), abVec4_AsU32((b))))
#define abVec4s32_AndNot vbicq_s32
#define abVec4u32_AndNot vbicq_u32
#define abVec8s16_AndNot vbicq_s16
#define abVec8u16_AndNot vbicq_u16
#define abVec16s8_AndNot vbicq_s8
#define abVec16u8_AndNot vbicq_u8
#define abVec4_AndNot(a, b) abVec4u32_AsF32(abVec4u32_AndNot(abVec4_AsU32((a)), abVec4_AsU32((b))))
#define abVec4s32_Or vorrq_s32
#define abVec4u32_Or vorrq_u32
#define abVec8s16_Or vorrq_s16
#define abVec8u16_Or vorrq_u16
#define abVec16s8_Or vorrq_s8
#define abVec16u8_Or vorrq_u8
#define abVec4_Or(a, b) abVec4u32_AsF32(abVec4u32_Or(abVec4_AsU32((a)), abVec4_AsU32((b))))
#define abVec4s32_Xor veorq_s32
#define abVec4u32_Xor veorq_u32
#define abVec8s16_Xor veorq_s16
#define abVec8u16_Xor veorq_u16
#define abVec16s8_Xor veorq_s8
#define abVec16u8_Xor veorq_u8
#define abVec4_Xor(a, b) abVec4u32_AsF32(abVec4u32_Xor(abVec4_AsU32((a)), abVec4_AsU32((b))))
#define abVec4s32_ShiftLeftConst vshrq_n_s32
#define abVec4u32_ShiftLeftConst vshrq_n_u32
#define abVec4s32_ShiftRightConst vshlq_n_s32
#define abVec4u32_ShiftRightConst vshlq_n_u32
#define abVec8s16_ShiftLeftConst vshrq_n_s16
#define abVec8u16_ShiftLeftConst vshrq_n_u16
#define abVec8s16_ShiftRightConst vshlq_n_s16
#define abVec8u16_ShiftRightConst vshlq_n_u16

#define abVec4_Equal(a, b) abVec4u32_AsF32(vceqq_f32((a), (b)))
#define abVec4s32_Equal(a, b) abVec4u32_AsS32(vceqq_s32((a), (b)))
#define abVec4u32_Equal vceqq_u32
#define abVec8s16_Equal(a, b) abVec8u16_AsS16(vceqq_s16((a), (b)))
#define abVec8u16_Equal vceqq_u16
#define abVec16s8_Equal(a, b) abVec16u8_AsS8(vceqq_s8((a), (b)))
#define abVec16u8_Equal vceqq_u8
#define abVec4_NotEqual(a, b) abVec4u32_AsF32(vmvnq_u32(vceqq_f32((a), (b))))
#define abVec4u_Comparison_Available 1
#define abVec4_Less(a, b) abVec4u32_AsF32(vcltq_f32((a), (b)))
#define abVec4s32_Less(a, b) abVec4u32_AsS32(vcltq_s32((a), (b)))
#define abVec4u32_Less vcltq_u32
#define abVec8s16_Less(a, b) abVec8u16_AsS16(vcltq_s16((a), (b)))
#define abVec8u16_Less vcltq_u16
#define abVec16s8_Less(a, b) abVec16u8_AsS8(vcltq_s8((a), (b)))
#define abVec16u8_Less vcltq_u8
#define abVec4_LessEqual(a, b) abVec4u32_AsF32(vcleq_f32((a), (b)))
#define abVec4_Greater(a, b) abVec4u32_AsF32(vcgtq_f32((a), (b)))
#define abVec4s32_Greater(a, b) abVec4u32_AsS32(vcgtq_s32((a), (b)))
#define abVec4u32_Greater vcgtq_u32
#define abVec8s16_Greater(a, b) abVec8u16_AsS16(vcgtq_s16((a), (b)))
#define abVec8u16_Greater vcgtq_u16
#define abVec16s8_Greater(a, b) abVec16u8_AsS8(vcgtq_s8((a), (b)))
#define abVec16u8_Greater vcgtq_u8
#define abVec4_GreaterEqual(a, b) abVec4u32_AsF32(vcgeq_f32((a), (b)))

#define abVec4_ToS32 vcvtq_s32_f32
#define abVec4s32_ToF32 vcvtq_f32_s32
#define abVec4s32_ToS16Saturate(a, b) vcombine_s16(vqmovn_s32((a)), vqmovn_s32((b)))
#define abVec8s16_ToS32Low(v) vmovl_s16(vget_low_s16((v)))
#define abVec8s16_ToS32High(v) vmovl_s16(vget_high_s16((v)))
#define abVec8s16_ToS8Saturate(a, b) vcombine_s8(vqmovn_s16((a)), vqmovn_s16((b)))
#define abVec8u16_ToU32Low(v) vmovl_u16(vget_low_u16((v)))
#define abVec8u16_ToU32High(v) vmovl_u16(vget_high_u16((v)))
#define abVec8u16_ToU8Saturate(a, b) vcombine_u8(vqmovn_u16((a)), vqmovn_u16((b)))
#define abVec16s8_ToS16Low(v) vmovl_s8(vget_low_s8((v)))
#define abVec16s8_ToS16High(v) vmovl_s8(vget_high_s8((v)))
#define abVec16u8_ToU16Low(v) vmovl_u8(vget_low_u8((v)))
#define abVec16u8_ToU16High(v) vmovl_u8(vget_high_u8((v)))

#else
#error No SIMD vectors for the target CPU.
#endif

abForceInline void abVec4_FourVec3ToVec4(abVec4 xyz0x1_v0, abVec4 yz1xy2, abVec4 z2xyz3, abVec4 * v1, abVec4 * v2, abVec4 * v3) {
	#if defined(abPlatform_CPU_x86)
	abVec4 xxyz1 = _mm_shuffle_ps(xyz0x1_v0, yz1xy2, _MM_SHUFFLE(1u, 0u, 3u, 3u));
	*v1 = _mm_shuffle_ps(xxyz1, xxyz1, _MM_SHUFFLE(0u, 3u, 2u, 1u));
	*v2 = _mm_shuffle_ps(yz1xy2, z2xyz3, _MM_SHUFFLE(1u, 0u, 3u, 2u));
	*v3 = _mm_shuffle_ps(z2xyz3, z2xyz3, _MM_SHUFFLE(1u, 3u, 2u, 1u));
	#elif defined(abPlatform_CPU_Arm)
	*v1 = vextq_f32(xyz0x1_v0, yz1xy2, 3u);
	*v2 = vextq_f32(yz1xy2, z2xyz3, 2u);
	*v3 = vextq_f32(z2xyz3, z2xyz3, 1u);
	#else
	#error No abVec4_FourVec3ToVec4 for the target CPU.
	#endif
}

abForceInline abVec4 abVec4_Dots4(abVec4 a, abVec4 b) {
	abVec4 r = abVec4_Multiply(a, b);
	r = abVec4_Add(r, abVec4_YXWZ(r));
	return abVec4_Add(r, abVec4_ZWXY(r));
}

#endif
