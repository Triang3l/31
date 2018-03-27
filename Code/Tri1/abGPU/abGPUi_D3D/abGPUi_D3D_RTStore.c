#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

unsigned int abGPUi_D3D_RTStore_DescriptorSizeColor, abGPUi_D3D_RTStore_DescriptorSizeDepth;

bool abGPU_RTStore_Init(abGPU_RTStore * store, unsigned int countColor, unsigned int countDepth) {
	D3D12_DESCRIPTOR_HEAP_DESC desc = { 0 };
	if (countColor != 0u) {
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = countColor;
		if (FAILED(ID3D12Device_CreateDescriptorHeap(abGPUi_D3D_Device, &desc, &IID_ID3D12DescriptorHeap, &store->i_descriptorHeapColor))) {
			return false;
		}
		store->i_cpuDescriptorHandleStartColor = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(store->i_descriptorHeapColor);
	} else {
		store->i_descriptorHeapColor = abNull;
		store->i_cpuDescriptorHandleStartColor.ptr = 0u;
	}
	if (countDepth != 0u) {
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.NumDescriptors = countDepth;
		if (FAILED(ID3D12Device_CreateDescriptorHeap(abGPUi_D3D_Device, &desc, &IID_ID3D12DescriptorHeap, &store->i_descriptorHeapDepth))) {
			if (countColor != 0u) {
				ID3D12DescriptorHeap_Release(store->i_descriptorHeapColor);
			}
			return false;
		}
		store->i_cpuDescriptorHandleStartDepth = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(store->i_descriptorHeapDepth);
	} else {
		store->i_descriptorHeapDepth = abNull;
		store->i_cpuDescriptorHandleStartDepth.ptr = 0u;
	}
	store->renderTargets = abMemory_Alloc(abGPUi_D3D_MemoryTag, (countColor + countDepth) * sizeof(abGPU_RTStore_RT), false);
	return true;
}

bool abGPU_RTStore_SetColor(abGPU_RTStore * store, unsigned int rtIndex, abGPU_Image * image,
		unsigned int layer, unsigned int side, unsigned int mip) {
	if (rtIndex >= store->countColor || !(image->typeAndDimensions & abGPU_Image_Type_Renderable) ||
			abGPU_Image_Format_IsDepth(image->format) || mip >= image->mips) {
		return false;
	}
	D3D12_RENDER_TARGET_VIEW_DESC desc;
	desc.Format = abGPUi_D3D_Image_FormatToResource(image->format);
	abGPU_Image_Dimensions dimensions = abGPU_Image_GetDimensions(image);
	switch (dimensions) {
	case abGPU_Image_Dimensions_2D:
		layer = side = 0u;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = mip;
		desc.Texture2D.PlaneSlice = 0u;
		break;
	case abGPU_Image_Dimensions_2DArray:
	case abGPU_Image_Dimensions_Cube:
	case abGPU_Image_Dimensions_CubeArray:
		{
			unsigned int arraySlice = 0u;
			if (abGPU_Image_Dimensions_AreArray(dimensions)) {
				if (layer >= image->d) { return false; }
				arraySlice = layer;
			} else {
				layer = 0u;
			}
			if (abGPU_Image_Dimensions_AreCube(dimensions)) {
				if (side >= 6) { return false; }
				arraySlice = arraySlice * 6u + side;
			} else {
				side = 0u;
			}
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = mip;
			desc.Texture2DArray.FirstArraySlice = arraySlice;
			desc.Texture2DArray.ArraySize = 1u;
			desc.Texture2DArray.PlaneSlice = 0u;
		}
		break;
	case abGPU_Image_Dimensions_3D:
		if (layer >= image->d) { return false; }
		layer = side = 0u;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = mip;
		desc.Texture3D.FirstWSlice = layer;
		desc.Texture3D.WSize = 1u;
		break;
	default:
		return false;
	}
	abGPU_RTStore_RT * rt = &store->renderTargets[rtIndex];
	rt->image = image;
	rt->layer = layer;
	rt->side = side;
	rt->mip = mip;
	ID3D12Device_CreateRenderTargetView(abGPUi_D3D_Device, image->i_resource, &desc, abGPUi_D3D_RTStore_GetDescriptorHandleColor(store, rtIndex));
	return true;
}

bool abGPU_RTStore_SetDepth(abGPU_RTStore * store, unsigned int rtIndex, abGPU_Image * image,
		unsigned int layer, unsigned int side, unsigned int mip, bool readOnly) {
	if (rtIndex >= store->countDepth || !(image->typeAndDimensions & abGPU_Image_Type_Renderable) ||
			!abGPU_Image_Format_IsDepth(image->format) || mip >= image->mips) {
		return false;
	}
	D3D12_DEPTH_STENCIL_VIEW_DESC desc;
	desc.Format = abGPUi_D3D_Image_FormatToDepthStencil(image->format);
	desc.Flags = (readOnly ? (D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL) : D3D12_DSV_FLAG_NONE);
	abGPU_Image_Dimensions dimensions = abGPU_Image_GetDimensions(image);
	switch (dimensions) {
	case abGPU_Image_Dimensions_2D:
		layer = side = 0u;
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = mip;
		break;
	case abGPU_Image_Dimensions_2DArray:
	case abGPU_Image_Dimensions_Cube:
	case abGPU_Image_Dimensions_CubeArray:
		{
			unsigned int arraySlice = 0u;
			if (abGPU_Image_Dimensions_AreArray(dimensions)) {
				if (layer >= image->d) { return false; }
				arraySlice = layer;
			} else {
				layer = 0u;
			}
			if (abGPU_Image_Dimensions_AreCube(dimensions)) {
				if (side >= 6) { return false; }
				arraySlice = arraySlice * 6u + side;
			} else {
				side = 0u;
			}
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = mip;
			desc.Texture2DArray.FirstArraySlice = arraySlice;
			desc.Texture2DArray.ArraySize = 1u;
		}
		break;
	default:
		return false;
	}
	abGPU_RTStore_RT * rt = &store->renderTargets[store->countColor + rtIndex];
	rt->image = image;
	rt->layer = layer;
	rt->side = side;
	rt->mip = mip;
	ID3D12Device_CreateDepthStencilView(abGPUi_D3D_Device, image->i_resource, &desc, abGPUi_D3D_RTStore_GetDescriptorHandleDepth(store, rtIndex));
	return true;
}

void abGPU_RTStore_Destroy(abGPU_RTStore * store) {
	if (store->countDepth != 0u) {
		ID3D12DescriptorHeap_Release(store->i_descriptorHeapDepth);
	}
	if (store->countColor != 0u) {
		ID3D12DescriptorHeap_Release(store->i_descriptorHeapColor);
	}
}

#endif
