#include "abGPU.h"
#include "../abMath/abVec.h"

void abGPU_VertexData_Convert_Float32x3ToSNorm16x4(int16_t * target, float const * source, size_t vertexCount) {
	while (vertexCount != 0u && (((size_t) target & 15u) || ((size_t) source & 15u))) {
		target[0u] = (int16_t) (source[0u] * (float) INT16_MAX);
		target[1u] = (int16_t) (source[1u] * (float) INT16_MAX);
		target[2u] = (int16_t) (source[2u] * (float) INT16_MAX);
		target[3u] = 0;
		source += 3u;
		target += 4u;
		--vertexCount;
	}
	if (vertexCount > 4u) {
		abVec4 const scale = abVec4_LoadX4((float) INT16_MAX);
		while (vertexCount > 4u) {
			abVec4 f0_xyz0x1 = abVec4_LoadAligned(source);
			abVec4 yz1xy2 = abVec4_LoadAligned(source + 4u);
			abVec4 z2xyz3 = abVec4_LoadAligned(source + 8u);
			f0_xyz0x1 = abVec4_Multiply(f0_xyz0x1, scale);
			yz1xy2 = abVec4_Multiply(yz1xy2, scale);
			z2xyz3 = abVec4_Multiply(z2xyz3, scale);
			abVec4 f1, f2, f3;
			abVec4_FourVec3ToVec4(f0_xyz0x1, yz1xy2, z2xyz3, &f1, &f2, &f3);
			abVec4s32 s0 = abVec4_ToS32(f0_xyz0x1), s1 = abVec4_ToS32(f1), s2 = abVec4_ToS32(f2), s3 = abVec4_ToS32(f3);
			abVec8s16_StoreAligned(target, abVec4s32_ToS16Saturate(s0, s1));
			abVec8s16_StoreAligned(target + 8u, abVec4s32_ToS16Saturate(s2, s3));
			source += 12u;
			target += 16u;
			vertexCount -= 4u;
		}
	}
	while (vertexCount != 0u) {
		target[0u] = (int16_t) (source[0u] * (float) INT16_MAX);
		target[1u] = (int16_t) (source[1u] * (float) INT16_MAX);
		target[2u] = (int16_t) (source[2u] * (float) INT16_MAX);
		target[3u] = 0;
		source += 3u;
		target += 4u;
		--vertexCount;
	}
}

void abGPU_VertexData_Convert_UNorm8ToFloat32_Array(float * target, uint8_t const * source, size_t count) {
	while (count != 0u && (((size_t) target & 15u) || ((size_t) source & 15u))) {
		*(target++) = *(source++) * (1.0f / UINT8_MAX);
		--count;
	}
	if (count > 16u) {
		abVec4 const scale = abVec4_LoadX4(1.0f / UINT8_MAX);
		while (count > 16u) {
			abVec16u8 sixteenComponents = abVec8u16_LoadAligned(source);
			abVec8u16 eightComponentsLow = abVec16u8_ToU16Low(sixteenComponents);
			abVec8u16 eightComponentsHigh = abVec16u8_ToU16High(sixteenComponents);
			abVec4_StoreAligned(target, abVec4_Multiply(abVec4s32_ToF32(abVec4u32_AsS32(abVec8u16_ToU32Low(eightComponentsLow))), scale));
			abVec4_StoreAligned(target + 4u, abVec4_Multiply(abVec4s32_ToF32(abVec4u32_AsS32(abVec8u16_ToU32High(eightComponentsLow))), scale));
			abVec4_StoreAligned(target + 8u, abVec4_Multiply(abVec4s32_ToF32(abVec4u32_AsS32(abVec8u16_ToU32Low(eightComponentsHigh))), scale));
			abVec4_StoreAligned(target + 12u, abVec4_Multiply(abVec4s32_ToF32(abVec4u32_AsS32(abVec8u16_ToU32High(eightComponentsHigh))), scale));
			source += 16u;
			target += 16u;
			count -= 16u;
		}
	}
	while (count != 0u) {
		*(target++) = *(source++) * (1.0f / UINT8_MAX);
		--count;
	}
}

void abGPU_VertexData_Convert_SNorm8ToFloat32_Array(float * target, int8_t const * source, size_t count) {
	while (count != 0u && (((size_t) target & 15u) || ((size_t) source & 15u))) {
		float component = *(source++) * (1.0f / INT8_MAX);
		*(target++) = abMax(component, -1.0f);
		--count;
	}
	if (count > 16u) {
		abVec4 const scale = abVec4_LoadX4(1.0f / INT8_MAX);
		#ifdef abVec16s8_MinMax_Available
		abVec16s8 const minValue = abVec16s8_LoadX16(-INT8_MAX);
		#else
		abVec16s8 const valueToClamp = abVec16s8_LoadX16(INT8_MIN);
		abVec16s8 const clampMask = abVec16s8_LoadX16(1);
		#endif
		while (count > 16u) {
			abVec16s8 sixteenComponents = abVec8s16_LoadAligned(source);
			#ifdef abVec16s8_MinMax_Available
			sixteenComponents = abVec16s8_Max(sixteenComponents, minValue);
			#else
			sixteenComponents = abVec16s8_Add(sixteenComponents, abVec16s8_And(abVec16s8_Equal(sixteenComponents, valueToClamp), clampMask));
			#endif
			abVec8s16 eightComponentsLow = abVec16s8_ToS16Low(sixteenComponents);
			abVec8s16 eightComponentsHigh = abVec16s8_ToS16High(sixteenComponents);
			abVec4_StoreAligned(target, abVec4_Multiply(abVec4s32_ToF32(abVec8s16_ToS32Low(eightComponentsLow)), scale));
			abVec4_StoreAligned(target + 4u, abVec4_Multiply(abVec4s32_ToF32(abVec8s16_ToS32High(eightComponentsLow)), scale));
			abVec4_StoreAligned(target + 8u, abVec4_Multiply(abVec4s32_ToF32(abVec8s16_ToS32Low(eightComponentsHigh)), scale));
			abVec4_StoreAligned(target + 12u, abVec4_Multiply(abVec4s32_ToF32(abVec8s16_ToS32High(eightComponentsHigh)), scale));
			source += 16u;
			target += 16u;
			count -= 16u;
		}
	}
	while (count != 0u) {
		float component = *(source++) * (1.0f / INT8_MAX);
		*(target++) = abMax(component, -1.0f);
		--count;
	}
}

void abGPU_VertexData_Convert_UNorm16ToFloat32_Array(float * target, uint16_t const * source, size_t count) {
	while (count != 0u && (((size_t) target & 15u) || ((size_t) source & 15u))) {
		*(target++) = *(source++) * (1.0f / UINT16_MAX);
		--count;
	}
	if (count > 8u) {
		abVec4 const scale = abVec4_LoadX4(1.0f / UINT16_MAX);
		while (count > 8u) {
			abVec8u16 eightComponents = abVec8u16_LoadAligned(source);
			abVec4_StoreAligned(target, abVec4_Multiply(abVec4s32_ToF32(abVec4u32_AsS32(abVec8u16_ToU32Low(eightComponents))), scale));
			abVec4_StoreAligned(target + 4u, abVec4_Multiply(abVec4s32_ToF32(abVec4u32_AsS32(abVec8u16_ToU32High(eightComponents))), scale));
			source += 8u;
			target += 8u;
			count -= 8u;
		}
	}
	while (count != 0u) {
		*(target++) = *(source++) * (1.0f / UINT16_MAX);
		--count;
	}
}

void abGPU_VertexData_Convert_SNorm16ToFloat32_Array(float * target, int16_t const * source, size_t count) {
	while (count != 0u && (((size_t) target & 15u) || ((size_t) source & 15u))) {
		float component = *(source++) * (1.0f / INT16_MAX);
		*(target++) = abMax(component, -1.0f);
		--count;
	}
	if (count > 8u) {
		abVec4 const scale = abVec4_LoadX4(1.0f / INT16_MAX);
		abVec8s16 const minValue = abVec8s16_LoadX8(-INT16_MAX);
		while (count > 8u) {
			abVec8u16 eightComponents = abVec8s16_Max(abVec8s16_LoadAligned(source), minValue);
			abVec4_StoreAligned(target, abVec4_Multiply(abVec4s32_ToF32(abVec8s16_ToS32Low(eightComponents)), scale));
			abVec4_StoreAligned(target + 4u, abVec4_Multiply(abVec4s32_ToF32(abVec8s16_ToS32High(eightComponents)), scale));
			source += 8u;
			target += 8u;
			count -= 8u;
		}
	}
	while (count != 0u) {
		float component = *(source++) * (1.0f / INT16_MAX);
		*(target++) = abMax(component, -1.0f);
		--count;
	}
}
