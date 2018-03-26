#ifndef abInclude_abGPU_abGPUi_D3D
#define abInclude_abGPU_abGPUi_D3D
#ifdef abBuild_GPUi_D3D
#include "../abGPU.h"
#include "../../abMemory/abMemory.h"

extern abMemory_Tag * abGPUi_D3D_MemoryTag;

extern IDXGIFactory2 * abGPUi_D3D_DXGIFactory;
extern IDXGIAdapter3 * abGPUi_D3D_DXGIAdapterMain;
extern ID3D12Device * abGPUi_D3D_Device;
extern ID3D12CommandQueue * abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Count];

// Images.

D3D12_RESOURCE_STATES abGPUi_D3D_Image_UsageToStates(abGPU_Image_Usage usage);

// Buffer and image handles.

extern unsigned int abGPUi_D3D_HandleStore_DescriptorSize;
abForceInline D3D12_CPU_DESCRIPTOR_HANDLE abGPUi_D3D_HandleStore_GetCPUDescriptorHandle(abGPU_HandleStore const * store, unsigned int handleIndex) {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = store->i_cpuDescriptorHandleStart;
	handle.ptr += handleIndex * abGPUi_D3D_HandleStore_DescriptorSize;
	return handle;
}
abForceInline D3D12_GPU_DESCRIPTOR_HANDLE abGPUi_D3D_HandleStore_GetGPUDescriptorHandle(abGPU_HandleStore const * store, unsigned int handleIndex) {
	D3D12_GPU_DESCRIPTOR_HANDLE handle = store->i_gpuDescriptorHandleStart;
	handle.ptr += handleIndex * abGPUi_D3D_HandleStore_DescriptorSize;
	return handle;
}

// Samplers.

extern unsigned int abGPUi_D3D_SamplerStore_DescriptorSize;
abForceInline D3D12_CPU_DESCRIPTOR_HANDLE abGPUi_D3D_SamplerStore_GetCPUDescriptorHandle(abGPU_SamplerStore const * store, unsigned int samplerIndex) {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = store->i_cpuDescriptorHandleStart;
	handle.ptr += samplerIndex * abGPUi_D3D_SamplerStore_DescriptorSize;
	return handle;
}
abForceInline D3D12_GPU_DESCRIPTOR_HANDLE abGPUi_D3D_SamplerStore_GetGPUDescriptorHandle(abGPU_SamplerStore const * store, unsigned int samplerIndex) {
	D3D12_GPU_DESCRIPTOR_HANDLE handle = store->i_gpuDescriptorHandleStart;
	handle.ptr += samplerIndex * abGPUi_D3D_SamplerStore_DescriptorSize;
	return handle;
}
void abGPUi_D3D_Sampler_WriteStaticSamplerDesc(abGPU_Sampler sampler, D3D12_STATIC_SAMPLER_DESC * desc);

#endif
#endif
