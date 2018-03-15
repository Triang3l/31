#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

void abGPU_Image_GetMaxSize(abGPU_Image_Dimensions dimensions, unsigned int *wh, unsigned int *d) {
	unsigned int maxWH = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, maxD = 1;
	switch (dimensions) {
	case abGPU_Image_Dimensions_2DArray:
		maxD = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
		return;
	case abGPU_Image_Dimensions_Cube:
		maxWH = D3D12_REQ_TEXTURECUBE_DIMENSION;
		return;
	case abGPU_Image_Dimensions_CubeArray:
		maxWH = D3D12_REQ_TEXTURECUBE_DIMENSION;
		maxD = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / 6;
		return;
	case abGPU_Image_Dimensions_3D:
		maxWH = maxD = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
		return;
	}
	if (wh != abNull) {
		*wh = maxWH;
	}
	if (d != abNull) {
		*d = maxD;
	}
}

#endif
