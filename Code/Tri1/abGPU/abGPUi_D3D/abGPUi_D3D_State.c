#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"
#include <d3dcompiler.h>

bool abGPU_ShaderCode_Init(abGPU_ShaderCode * code, void const * source, size_t sourceLength) {
	if (FAILED(D3DCreateBlob(sourceLength, &code->i_blob))) {
		return false;
	}
	memcpy(ID3D10Blob_GetBufferPointer(code->i_blob), source, sourceLength);
	return true;
}

void abGPU_ShaderCode_Destroy(abGPU_ShaderCode * code) {
	ID3D10Blob_Release(code->i_blob);
}

#endif
