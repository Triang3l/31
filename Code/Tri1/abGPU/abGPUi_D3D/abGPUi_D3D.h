#ifndef abInclude_abGPU_abGPUi_D3D
#define abInclude_abGPU_abGPUi_D3D
#ifdef abBuild_GPUi_D3D
#include "../abGPU.h"
#include "../../abMemory/abMemory.h"

extern abMemory_Tag * abGPUi_D3D_MemoryTag;

extern abBool abGPUi_D3D_DebugEnabled;

extern IDXGIFactory2 * abGPUi_D3D_DXGIFactory;
extern IDXGIAdapter3 * abGPUi_D3D_DXGIAdapterMain;
extern ID3D12Device * abGPUi_D3D_Device;
extern ID3D12CommandQueue * abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Count];

typedef HRESULT (STDMETHODCALLTYPE * abGPUi_D3D_ObjectNameSetter)(void * object, WCHAR const * name);
void abGPUi_D3D_SetObjectName(void * object, abGPUi_D3D_ObjectNameSetter setter, abTextU8 const * name); // Setter is SetName from vtbl.

// Fixes for methods returning structures.
typedef void (STDMETHODCALLTYPE * abGPUi_D3D_ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart)(
		ID3D12DescriptorHeap * heap, D3D12_CPU_DESCRIPTOR_HANDLE * handle);
typedef void (STDMETHODCALLTYPE * abGPUi_D3D_ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart)(
		ID3D12DescriptorHeap * heap, D3D12_GPU_DESCRIPTOR_HANDLE * handle);
typedef void (STDMETHODCALLTYPE * abGPUi_D3D_ID3D12Resource_GetDesc)(ID3D12Resource * resource, D3D12_RESOURCE_DESC * desc);

// Buffers.

D3D12_RESOURCE_STATES abGPUi_D3D_Buffer_UsageToStates(abGPU_Buffer_Usage usage);

// Images.

DXGI_FORMAT abGPUi_D3D_Image_FormatToResource(abGPU_Image_Format format);
DXGI_FORMAT abGPUi_D3D_Image_FormatToShaderResource(abGPU_Image_Format format);
DXGI_FORMAT abGPUi_D3D_Image_FormatToDepthStencil(abGPU_Image_Format format);
D3D12_RESOURCE_STATES abGPUi_D3D_Image_UsageToStates(abGPU_Image_Usage usage);

inline unsigned int abGPUi_D3D_Image_SliceToSubresource(abGPU_Image const * image, abGPU_Image_Slice slice, abBool stencil) {
	unsigned int subresource = abGPU_Image_Slice_Layer(slice);
	if (stencil) { subresource += ((image->options & abGPU_Image_Options_Array) ? image->d : 1u); }
	if (image->options & abGPU_Image_Options_Cube) { subresource = subresource * 6u + abGPU_Image_Slice_Side(slice); }
	return subresource * image->mips + abGPU_Image_Slice_Mip(slice);
}

// Format may be sRGB, but swap chains require a linear format, so it's passed explicitly.
void abGPUi_D3D_Image_InitForSwapChainBuffer(abGPU_Image * image, ID3D12Resource * resource, abGPU_Image_Format format);

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

// Render targets.

extern unsigned int abGPUi_D3D_RTStore_DescriptorSizeColor, abGPUi_D3D_RTStore_DescriptorSizeDepth;
abForceInline D3D12_CPU_DESCRIPTOR_HANDLE abGPUi_D3D_RTStore_GetDescriptorHandleColor(abGPU_RTStore const * store, unsigned int rtIndex) {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = store->i_cpuDescriptorHandleStartColor;
	handle.ptr += rtIndex * abGPUi_D3D_RTStore_DescriptorSizeColor;
	return handle;
}
abForceInline D3D12_CPU_DESCRIPTOR_HANDLE abGPUi_D3D_RTStore_GetDescriptorHandleDepth(abGPU_RTStore const * store, unsigned int rtIndex) {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = store->i_cpuDescriptorHandleStartDepth;
	handle.ptr += rtIndex * abGPUi_D3D_RTStore_DescriptorSizeDepth;
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
