#ifndef abInclude_abGPU_abGPUi_D3D
#define abInclude_abGPU_abGPUi_D3D
#ifdef abBuild_GPUi_D3D
#include "../abGPU.h"

extern IDXGIFactory2 *abGPUi_D3D_DXGIFactory;
extern IDXGIAdapter3 *abGPUi_D3D_DXGIAdapterMain;
extern ID3D12Device *abGPUi_D3D_Device;
extern ID3D12CommandQueue *abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Count];

// Images.

D3D12_RESOURCE_STATES abGPUi_D3D_Image_UsageToStates(abGPU_Image_Usage usage);

// Samplers.

extern unsigned int abGPUi_D3D_SamplerStore_DescriptorSize;
void abGPUi_D3D_Sampler_WriteStaticSamplerDesc(abGPU_Sampler sampler, D3D12_STATIC_SAMPLER_DESC *desc);

#endif
#endif
