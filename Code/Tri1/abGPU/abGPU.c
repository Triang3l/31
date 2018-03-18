#include "abGPU.h"

/*
 * GPU abstraction layer helper functions.
 */

unsigned int abGPU_Image_Format_GetSize(abGPU_Image_Format format) {
	switch (format) {
	case abGPU_Image_Format_R8G8B8A8:
	case abGPU_Image_Format_R8G8B8A8_sRGB:
	case abGPU_Image_Format_R8G8B8A8_Signed:
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

void abGPU_Image_ClampSizeToMax(abGPU_Image_Dimensions dimensions,
		unsigned int *w, unsigned int *h, unsigned int *d, unsigned int *mips) {
	unsigned int maxWH, maxD;
	abGPU_Image_GetMaxSize(dimensions, &maxWH, &maxD);
	*w = abClamp(*w, 1u, maxWH);
	*h = abClamp(*h, 1u, maxWH);
	*d = abClamp(*d, 1u, maxD);
	if (mips != abNull) {
		unsigned int maxMips = abGPU_Image_CalculateMipCount(dimensions, *w, *h, *d);
		*mips = abClamp(*mips, 1u, maxMips);
	}
}
