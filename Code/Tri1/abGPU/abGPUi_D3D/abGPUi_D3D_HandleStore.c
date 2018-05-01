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

void abGPU_HandleStore_SetStructureBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int elementSize, unsigned int firstElementIndex, unsigned int elementCount) {
	if (handleIndex >= store->handleCount) {
		return;
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer = {
			.FirstElement = firstElementIndex,
			.NumElements = elementCount,
			.StructureByteStride = elementSize
		}
	};
	ID3D12Device_CreateShaderResourceView(abGPUi_D3D_Device, buffer->i_resource, &desc,
			abGPUi_D3D_HandleStore_GetCPUDescriptorHandle(store, handleIndex));
}

void abGPU_HandleStore_SetRawStructureBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int offset, unsigned int size) {
	if (handleIndex >= store->handleCount) {
		return;
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {
		.Format = DXGI_FORMAT_R32_TYPELESS,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer = {
			.FirstElement = offset >> 2u,
			.NumElements = size >> 2u,
			.Flags = D3D12_BUFFER_SRV_FLAG_RAW
		}
	};
	ID3D12Device_CreateShaderResourceView(abGPUi_D3D_Device, buffer->i_resource, &desc,
			abGPUi_D3D_HandleStore_GetCPUDescriptorHandle(store, handleIndex));
}

void abGPU_HandleStore_SetEditBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int elementSize, unsigned int firstElementIndex, unsigned int elementCount) {
	if (handleIndex >= store->handleCount) {
		return;
	}
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
		.Buffer = {
			.FirstElement = firstElementIndex,
			.NumElements = elementCount,
			.StructureByteStride = elementSize
		}
	};
	ID3D12Device_CreateUnorderedAccessView(abGPUi_D3D_Device, buffer->i_resource, abNull, &desc,
			abGPUi_D3D_HandleStore_GetCPUDescriptorHandle(store, handleIndex));
}

void abGPU_HandleStore_SetRawEditBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int offset, unsigned int size) {
	if (handleIndex >= store->handleCount) {
		return;
	}
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {
		.Format = DXGI_FORMAT_R32_TYPELESS,
		.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
		.Buffer = {
			.FirstElement = offset >> 2u,
			.NumElements = size >> 2u,
			.Flags = D3D12_BUFFER_UAV_FLAG_RAW
		}
	};
	ID3D12Device_CreateUnorderedAccessView(abGPUi_D3D_Device, buffer->i_resource, abNull, &desc,
			abGPUi_D3D_HandleStore_GetCPUDescriptorHandle(store, handleIndex));
}

void abGPU_HandleStore_SetTexture(abGPU_HandleStore * store, unsigned int handleIndex, abGPU_Image * image) {
	if (handleIndex >= store->handleCount) {
		return;
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {
		.Format = abGPUi_D3D_Image_FormatToShaderResource(image->format),
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
	};
	abGPU_Image_Options options = image->options;
	if (options & abGPU_Image_Options_3D) {
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipLevels = UINT_MAX;
	} else if (options & abGPU_Image_Options_Cube) {
		if (options & abGPU_Image_Options_Array) {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
			desc.TextureCubeArray.MipLevels = UINT_MAX;
			desc.TextureCubeArray.NumCubes = image->d;
		} else {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			desc.TextureCube.MipLevels = UINT_MAX;
		}
	} else {
		if (options & abGPU_Image_Options_Array) {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipLevels = UINT_MAX;
			desc.Texture2DArray.ArraySize = image->d;
		} else {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = UINT_MAX;
		}
	}
	ID3D12Device_CreateShaderResourceView(abGPUi_D3D_Device, image->i_resource, &desc,
			abGPUi_D3D_HandleStore_GetCPUDescriptorHandle(store, handleIndex));
}

void abGPU_HandleStore_SetEditImage(abGPU_HandleStore * store, unsigned int handleIndex, abGPU_Image * image, unsigned int mip) {
	if (handleIndex >= store->handleCount) {
		return;
	}
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
	desc.Format = abGPUi_D3D_Image_FormatToShaderResource(image->format);
	abGPU_Image_Options options = image->options;
	if (options & abGPU_Image_Options_3D) {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = mip;
		desc.Texture3D.FirstWSlice = 0u;
		desc.Texture3D.WSize = image->d;
	} else if (options & (abGPU_Image_Options_Array | abGPU_Image_Options_Cube)) {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipSlice = mip;
		desc.Texture2DArray.FirstArraySlice = 0u;
		desc.Texture2DArray.ArraySize = image->d * ((options & abGPU_Image_Options_Cube) ? 6u : 1u);
		desc.Texture2DArray.PlaneSlice = 0u;
	} else {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = mip;
		desc.Texture2D.PlaneSlice = 0u;
	}
	ID3D12Device_CreateUnorderedAccessView(abGPUi_D3D_Device, image->i_resource, abNull, &desc,
			abGPUi_D3D_HandleStore_GetCPUDescriptorHandle(store, handleIndex));
}

void abGPU_HandleStore_Destroy(abGPU_HandleStore * store) {
	ID3D12DescriptorHeap_Release(store->i_descriptorHeap);
}

#endif
