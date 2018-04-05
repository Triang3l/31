#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

unsigned int abGPUi_D3D_HandleStore_DescriptorSize;

abBool abGPU_HandleStore_Init(abGPU_HandleStore * store, abTextU8 const * name, unsigned int handleCount) {
	D3D12_DESCRIPTOR_HEAP_DESC desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = handleCount,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	};
	if (FAILED(ID3D12Device_CreateDescriptorHeap(abGPUi_D3D_Device, &desc, &IID_ID3D12DescriptorHeap, &store->i_descriptorHeap))) {
		return abFalse;
	}
	abGPUi_D3D_SetObjectName(store->i_descriptorHeap, (abGPUi_D3D_ObjectNameSetter) store->i_descriptorHeap->lpVtbl->SetName, name);
	store->handleCount = handleCount;
	store->i_cpuDescriptorHandleStart = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(store->i_descriptorHeap);
	store->i_gpuDescriptorHandleStart = ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(store->i_descriptorHeap);
	return abTrue;
}

void abGPU_HandleStore_SetConstantBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int offset, unsigned int size) {
	if (handleIndex >= store->handleCount) {
		return;
	}
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {
		.BufferLocation = buffer->i_gpuVirtualAddress + (offset & (abGPU_Buffer_ConstantAlignment - 1u)),
		.SizeInBytes = abAlign(size, abGPU_Buffer_ConstantAlignment)
	};
	ID3D12Device_CreateConstantBufferView(abGPUi_D3D_Device, &desc, abGPUi_D3D_HandleStore_GetCPUDescriptorHandle(store, handleIndex));
}

void abGPU_HandleStore_Destroy(abGPU_HandleStore * store) {
	ID3D12DescriptorHeap_Release(store->i_descriptorHeap);
}

#endif
