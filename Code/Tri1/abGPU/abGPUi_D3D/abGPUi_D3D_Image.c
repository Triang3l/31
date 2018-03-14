#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

void abGPU_Image_Destroy(abGPU_Image *image) {
	ID3D12Resource_Release(image->p.resource);
}

#endif
