#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

static D3D12_RESOURCE_STATES abGPUi_D3D_Buffer_UsageToStatesMap[abGPU_Buffer_Usage_Count] = {
	[abGPU_Buffer_Usage_Vertices] = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
	[abGPU_Buffer_Usage_Constants] = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
	[abGPU_Buffer_Usage_Indices] = D3D12_RESOURCE_STATE_INDEX_BUFFER,
	[abGPU_Buffer_Usage_Structures] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	[abGPU_Buffer_Usage_StructuresNonPixelStage] = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
	[abGPU_Buffer_Usage_StructuresAnyStage] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
	[abGPU_Buffer_Usage_Edit] = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
	[abGPU_Buffer_Usage_CopySource] = D3D12_RESOURCE_STATE_COPY_SOURCE,
	[abGPU_Buffer_Usage_CopyDestination] = D3D12_RESOURCE_STATE_COPY_DEST,
	[abGPU_Buffer_Usage_CopyQueue] = D3D12_RESOURCE_STATE_COMMON,
	[abGPU_Buffer_Usage_CPUWrite] = D3D12_RESOURCE_STATE_GENERIC_READ
	// 0 is D3D12_RESOURCE_STATE_COMMON.
};

D3D12_RESOURCE_STATES abGPUi_D3D_Buffer_UsageToStates(abGPU_Buffer_Usage usage) {
	return ((unsigned int) usage < abArrayLength(abGPUi_D3D_Buffer_UsageToStatesMap) ?
			abGPUi_D3D_Buffer_UsageToStatesMap[usage] : D3D12_RESOURCE_STATE_COMMON);
}

bool abGPU_Buffer_Init(abGPU_Buffer * buffer, abGPU_Buffer_Access access,
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
	if (FAILED(ID3D12Device_CreateCommittedResource(abGPUi_D3D_Device, &heapProperties, D3D12_HEAP_FLAG_NONE,
			&desc, abGPUi_D3D_Buffer_UsageToStates(initialUsage), abNull, &IID_ID3D12Resource, &buffer->i_resource))) {
		return false;
	}
	buffer->i_gpuVirtualAddress = ID3D12Resource_GetGPUVirtualAddress(buffer->i_resource);
	return true;
}

void * abGPU_Buffer_Map(abGPU_Buffer * buffer) {
	if (buffer->access == abGPU_Buffer_Access_GPUInternal) {
		return abNull;
	}
	void * mapping;
	return (SUCCEEDED(ID3D12Resource_Map(buffer->i_resource, 0u, abNull, &mapping)) ? mapping : abNull);
}

void abGPU_Buffer_Unmap(abGPU_Buffer * buffer, void * mapping, unsigned int const writtenOffsetAndSize[2]) {
	if (mapping == abNull) {
		return;
	}
	D3D12_RANGE writtenRange;
	if (writtenOffsetAndSize != abNull) {
		writtenRange.Begin = writtenOffsetAndSize[0u];
		writtenRange.End = writtenOffsetAndSize[0u] + writtenOffsetAndSize[1u];
	}
	ID3D12Resource_Unmap(buffer->i_resource, 0u, writtenOffsetAndSize != abNull ? &writtenRange : abNull);
}

void abGPU_Buffer_Destroy(abGPU_Buffer * buffer) {
	ID3D12Resource_Release(buffer->i_resource);
}

#endif
