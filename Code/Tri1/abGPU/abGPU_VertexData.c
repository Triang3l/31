#include "abGPU.h"
#include "../abMath/abVec.h"

void abGPU_VertexData_Convert_UNorm8ToFloat32_Array(float * target, uint8_t const * source, unsigned int count) {
	while (count != 0u && (((size_t) target & 15u) || ((size_t) source & 15u))) {
		*(target++) = *(source++) * (1.0f / 255.0f);
		--count;
	}
	abVec4 scale = abVec4_LoadX4(1.0f / 255.0f);
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
	while (count != 0u) {
		*(target++) = *(source++) * (1.0f / 255.0f);
		--count;
	}
}

void abGPU_VertexData_Convert_UNorm16ToFloat32_Array(float * target, uint16_t const * source, unsigned int count) {
	while (count != 0u && (((size_t) target & 15u) || ((size_t) source & 15u))) {
		*(target++) = *(source++) * (1.0f / 65535.0f);
		--count;
	}
	abVec4 scale = abVec4_LoadX4(1.0f / 65535.0f);
	while (count > 8u) {
		abVec8u16 eightComponents = abVec8u16_LoadAligned(source);
		abVec4_StoreAligned(target, abVec4_Multiply(abVec4s32_ToF32(abVec4u32_AsS32(abVec8u16_ToU32Low(eightComponents))), scale));
		abVec4_StoreAligned(target + 4u, abVec4_Multiply(abVec4s32_ToF32(abVec4u32_AsS32(abVec8u16_ToU32High(eightComponents))), scale));
		source += 8u;
		target += 8u;
		count -= 8u;
	}
	while (count != 0u) {
		*(target++) = *(source++) * (1.0f / 65535.0f);
		--count;
	}
}