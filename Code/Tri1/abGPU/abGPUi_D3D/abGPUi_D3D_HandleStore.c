#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

unsigned int abGPUi_D3D_HandleStore_DescriptorSize;

abBool abGPU_HandleStore_Init(abGPU_HandleStore * store, abTextU8 const * name, unsigned int handleCount) {
	D3D12_DESCRIPTOR_HEAP_DESC desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = handleCount,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	};
	ID3D12DescriptorHeap * heap;
	if (FAILED(ID3D12Device_CreateDescriptorHeap(abGPUi_D3D_Device, &desc, &IID_ID3D12DescriptorHeap, &heap))) {
		return abFalse;
	}
	abGPUi_D3D_SetObjectName(heap, (abGPUi_D3D_ObjectNameSetter) heap->lpVtbl->SetName, name);
	store->handleCount = handleCount;
	store->i_descriptorHeap = heap;
	((abGPUi_D3D_ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart)
			heap->lpVtbl->GetCPUDescriptorHandleForHeapStart)(heap, &store->i_cpuDescriptorHandleStart);
	((abGPUi_D3D_ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart)
			heap->lpVtbl->GetGPUDescriptorHandleForHeapStart)(heap, &store->i_gpuDescriptorHandleStart);
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
