#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

D3D12_RESOURCE_STATES abGPUi_D3D_Buffer_UsageToStates(abGPU_Buffer_Usage usage) {
	switch (usage) {
	case abGPU_Buffer_Usage_Vertices:
	case abGPU_Buffer_Usage_Constants:
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	case abGPU_Buffer_Usage_Indices:
		return D3D12_RESOURCE_STATE_INDEX_BUFFER;
	case abGPU_Buffer_Usage_Structures:
		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case abGPU_Buffer_Usage_StructuresNonPixelStage:
		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	case abGPU_Buffer_Usage_StructuresAnyStage:
		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	case abGPU_Buffer_Usage_Edit:
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	case abGPU_Buffer_Usage_CopySource:
		return D3D12_RESOURCE_STATE_COPY_SOURCE;
	case abGPU_Buffer_Usage_CopyDestination:
		return D3D12_RESOURCE_STATE_COPY_DEST;
	case abGPU_Buffer_Usage_CopyQueue:
		return D3D12_RESOURCE_STATE_COMMON;
	case abGPU_Buffer_Usage_CPUWrite:
		return D3D12_RESOURCE_STATE_GENERIC_READ;
	}
	return D3D12_RESOURCE_STATE_COMMON; // This shouldn't happen!
}

bool abGPU_Buffer_Init(abGPU_Buffer *buffer, abGPU_Buffer_Access access,
		unsigned int size, bool editable, abGPU_Buffer_Usage initialUsage) {
	if (size == 0u) {
		return false;
	}

	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = size;
	desc.Height = 1u;
	desc.DepthOrArraySize = 1u;
	desc.MipLevels = 1u;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1u;
	desc.SampleDesc.Quality = 0u;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = editable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProperties = { 0 };
	if (access == abGPU_Buffer_Access_GPUInternal) {
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	} else {
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		initialUsage = abGPU_Buffer_Usage_CPUWrite;
	}

	buffer->access = access;
	buffer->size = size;
	return SUCCEEDED(ID3D12Device_CreateCommittedResource(abGPUi_D3D_Device,
			&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, abGPUi_D3D_Buffer_UsageToStates(initialUsage),
			abNull, &IID_ID3D12Resource, &buffer->i.resource)) ? true : false;
}

void *abGPU_Buffer_Map(abGPU_Buffer *buffer) {
	if (buffer->access == abGPU_Buffer_Access_GPUInternal) {
		return abNull;
	}
	void *mapping;
	return (SUCCEEDED(ID3D12Resource_Map(buffer->i.resource, 0u, abNull, &mapping)) ? mapping : abNull);
}

void abGPU_Buffer_Unmap(abGPU_Buffer *buffer, void *mapping, const unsigned int writtenOffsetAndSize[2]) {
	if (mapping == abNull) {
		return;
	}
	D3D12_RANGE writtenRange;
	if (writtenOffsetAndSize != abNull) {
		writtenRange.Begin = writtenOffsetAndSize[0u];
		writtenRange.End = writtenOffsetAndSize[0u] + writtenOffsetAndSize[1u];
	}
	ID3D12Resource_Unmap(buffer->i.resource, 0u, writtenOffsetAndSize != abNull ? &writtenRange : abNull);
}

void abGPU_Buffer_Destroy(abGPU_Buffer *buffer) {
	ID3D12Resource_Release(buffer->i.resource);
}

#endif
