#include "abGPU.h"

/*
 * GPU abstraction layer helper functions.
 */

abGPU_Image_Format abGPU_Image_Format_ToLinear(abGPU_Image_Format format) {
	switch (format) {
	case abGPU_Image_Format_R8G8B8A8_sRGB:
		return abGPU_Image_Format_R8G8B8A8;
	case abGPU_Image_Format_B8G8R8A8_sRGB:
		return abGPU_Image_Format_B8G8R8A8;
	case abGPU_Image_Format_S3TC_A1_sRGB:
		return abGPU_Image_Format_S3TC_A1;
	case abGPU_Image_Format_S3TC_A4_sRGB:
		return abGPU_Image_Format_S3TC_A4;
	case abGPU_Image_Format_S3TC_A8_sRGB:
		return abGPU_Image_Format_S3TC_A8;
	}
	return format;
}

unsigned int abGPU_Image_Format_GetSize(abGPU_Image_Format format) {
	switch (format) {
	case abGPU_Image_Format_R8:
		return 1u;
	case abGPU_Image_Format_R8G8:
	case abGPU_Image_Format_B5G5R5A1:
	case abGPU_Image_Format_B5G6R5:
		return 2u;
	case abGPU_Image_Format_R8G8B8A8:
	case abGPU_Image_Format_R8G8B8A8_sRGB:
	case abGPU_Image_Format_R8G8B8A8_Signed:
	case abGPU_Image_Format_B8G8R8A8:
	case abGPU_Image_Format_B8G8R8A8_sRGB:
	case abGPU_Image_Format_D32:
	case abGPU_Image_Format_D24S8: // Stencil plane is a very special thing, so not counting it.
		return 4u;
	case abGPU_Image_Format_S3TC_A1:
	case abGPU_Image_Format_S3TC_A1_sRGB:
	case abGPU_Image_Format_3Dc_X:
	case abGPU_Image_Format_3Dc_X_Signed:
		return 8u;
	case abGPU_Image_Format_S3TC_A4:
	case abGPU_Image_Format_S3TC_A4_sRGB:
	case abGPU_Image_Format_S3TC_A8:
	case abGPU_Image_Format_S3TC_A8_sRGB:
	case abGPU_Image_Format_3Dc_XY:
	case abGPU_Image_Format_3Dc_XY_Signed:
		return 16u;
	}
	return 0u; // This shouldn't happen!
}

void abGPU_Image_ClampSizeToSupportedMax(abGPU_Image_Options dimensionOptions,
		unsigned int *w, unsigned int *h, unsigned int *d, unsigned int *mips) {
	unsigned int maxWH, maxD;
	abGPU_Image_GetMaxSupportedSize(dimensionOptions, &maxWH, &maxD);
	*w = abClamp(*w, 1u, maxWH);
	*h = abClamp(*h, 1u, maxWH);
	*d = abClamp(*d, 1u, maxD);
	if (mips != abNull) {
		unsigned int maxMips = abGPU_Image_CalculateMipCount(dimensionOptions, *w, *h, *d);
		*mips = abClamp(*mips, 1u, maxMips);
	}
}
