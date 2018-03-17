#include "abGPU.h"

/*
 * GPU abstraction layer helper functions.
 */

void abGPU_Image_ClampSizeToMax(abGPU_Image_Dimensions dimensions,
		unsigned int *w, unsigned int *h, unsigned int *d, unsigned int *mips) {
	unsigned int maxWH, maxD;
	abGPU_Image_GetMaxSize(dimensions, &maxWH, &maxD);
	*w = abClamp(*w, 1, maxWH);
	*h = abClamp(*h, 1, maxWH);
	*d = abClamp(*d, 1, maxD);
	if (mips != abNull) {
		unsigned int maxMips = abGPU_Image_CalculateMipCount(dimensions, *w, *h, *d);
		*mips = abClamp(*mips, 1, maxMips);
	}
}
