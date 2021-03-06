#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

unsigned int abGPUi_D3D_SamplerStore_DescriptorSize;

abBool abGPU_SamplerStore_Init(abGPU_SamplerStore * store, abTextU8 const * name, unsigned int samplerCount) {
	D3D12_DESCRIPTOR_HEAP_DESC desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		.NumDescriptors = samplerCount,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	};
	ID3D12DescriptorHeap * heap;
	if (FAILED(ID3D12Device_CreateDescriptorHeap(abGPUi_D3D_Device, &desc, &IID_ID3D12DescriptorHeap, &heap))) {
		return abFalse;
	}
	abGPUi_D3D_SetObjectName(heap, (abGPUi_D3D_ObjectNameSetter) heap->lpVtbl->SetName, name);
	store->samplerCount = samplerCount;
	store->i_descriptorHeap = heap;
	((abGPUi_D3D_ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart)
			heap->lpVtbl->GetCPUDescriptorHandleForHeapStart)(heap, &store->i_cpuDescriptorHandleStart);
	((abGPUi_D3D_ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart)
			heap->lpVtbl->GetGPUDescriptorHandleForHeapStart)(heap, &store->i_gpuDescriptorHandleStart);
	return abTrue;
}

void abGPU_SamplerStore_SetSampler(abGPU_SamplerStore * store, unsigned int samplerIndex, abGPU_Sampler sampler) {
	if (samplerIndex >= store->samplerCount) {
		return;
	}
	D3D12_SAMPLER_DESC desc;
	unsigned int compareFail = (sampler >> abGPU_Sampler_CompareFailShift) & abGPU_Sampler_CompareFailMask;
	D3D12_FILTER_REDUCTION_TYPE reduction = (compareFail != 0u ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : D3D12_FILTER_REDUCTION_TYPE_STANDARD);
	if (sampler & abGPU_Sampler_FilterAniso) {
		desc.Filter = D3D12_ENCODE_ANISOTROPIC_FILTER(reduction);
		desc.MaxAnisotropy = (1u << (((sampler >> abGPU_Sampler_FilterAnisoPowerM1Shift) & abGPU_Sampler_FilterAnisoPowerM1Mask) + 1u));
	} else {
		desc.Filter = D3D12_ENCODE_BASIC_FILTER(
				(sampler & abGPU_Sampler_FilterLinearMin) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				(sampler & abGPU_Sampler_FilterLinearMag) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				(sampler & abGPU_Sampler_FilterLinearMip) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT, reduction);
		desc.MaxAnisotropy = 0u;
	}
	desc.AddressU = ((sampler & abGPU_Sampler_RepeatS) ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	desc.AddressV = ((sampler & abGPU_Sampler_RepeatT) ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	desc.AddressW = ((sampler & abGPU_Sampler_RepeatR) ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	desc.MipLODBias = 0.0f;
	desc.ComparisonFunc = (D3D12_COMPARISON_FUNC) ((compareFail ^ abGPU_Sampler_CompareFailMask) + 1u); // Change from fail to pass.
	desc.BorderColor[0u] = 1.0f;
	desc.BorderColor[1u] = 1.0f;
	desc.BorderColor[2u] = 1.0f;
	desc.BorderColor[3u] = 1.0f;
	desc.MinLOD = 0.0f;
	unsigned int maxLOD = (sampler >> abGPU_Sampler_MipCountShift) & abGPU_Sampler_MipCountMask;
	desc.MaxLOD = (maxLOD != abGPU_Sampler_MipCountMask ? (float) maxLOD : FLT_MAX);
	ID3D12Device_CreateSampler(abGPUi_D3D_Device, &desc, abGPUi_D3D_SamplerStore_GetCPUDescriptorHandle(store, samplerIndex));
}

void abGPU_SamplerStore_Destroy(abGPU_SamplerStore * store) {
	ID3D12DescriptorHeap_Release(store->i_descriptorHeap);
}

void abGPUi_D3D_Sampler_WriteStaticSamplerDesc(abGPU_Sampler sampler, D3D12_STATIC_SAMPLER_DESC * desc) {
	unsigned int compareFail = (sampler >> abGPU_Sampler_CompareFailShift) & abGPU_Sampler_CompareFailMask;
	D3D12_FILTER_REDUCTION_TYPE reduction = (compareFail != 0u ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : D3D12_FILTER_REDUCTION_TYPE_STANDARD);
	if (sampler & abGPU_Sampler_FilterAniso) {
		desc->Filter = D3D12_ENCODE_ANISOTROPIC_FILTER(reduction);
		desc->MaxAnisotropy = (1u << (((sampler >> abGPU_Sampler_FilterAnisoPowerM1Shift) & abGPU_Sampler_FilterAnisoPowerM1Mask) + 1u));
	} else {
		desc->Filter = D3D12_ENCODE_BASIC_FILTER(
				(sampler & abGPU_Sampler_FilterLinearMin) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				(sampler & abGPU_Sampler_FilterLinearMag) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				(sampler & abGPU_Sampler_FilterLinearMip) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT, reduction);
		desc->MaxAnisotropy = 0u;
	}
	desc->AddressU = ((sampler & abGPU_Sampler_RepeatS) ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	desc->AddressV = ((sampler & abGPU_Sampler_RepeatT) ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	desc->AddressW = ((sampler & abGPU_Sampler_RepeatR) ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	desc->MipLODBias = 0.0f;
	desc->ComparisonFunc = (D3D12_COMPARISON_FUNC) ((compareFail ^ abGPU_Sampler_CompareFailMask) + 1u); // Change from fail to pass.
	desc->BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	desc->MinLOD = 0.0f;
	unsigned int maxLOD = (sampler >> abGPU_Sampler_MipCountShift) & abGPU_Sampler_MipCountMask;
	desc->MaxLOD = (maxLOD != abGPU_Sampler_MipCountMask ? (float) maxLOD : FLT_MAX);
}

#endif
