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

bool abGPU_RTStore_SetColor(abGPU_RTStore * store, unsigned int rtIndex, abGPU_Image * image, abGPU_Image_Slice slice) {
	if (rtIndex >= store->countColor || !(image->typeAndDimensions & abGPU_Image_Type_Renderable) ||
			abGPU_Image_Format_IsDepth(image->format) || !abGPUi_Image_HasSlice(image, slice)) {
		return false;
	}
	D3D12_RENDER_TARGET_VIEW_DESC desc;
	desc.Format = abGPUi_D3D_Image_FormatToResource(image->format);
	abGPU_Image_Dimensions dimensions = abGPU_Image_GetDimensions(image);
	switch (dimensions) {
	case abGPU_Image_Dimensions_2D:
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = abGPU_Image_SliceMip(slice);
		desc.Texture2D.PlaneSlice = 0u;
		break;
	case abGPU_Image_Dimensions_2DArray:
	case abGPU_Image_Dimensions_Cube:
	case abGPU_Image_Dimensions_CubeArray:
		{
			unsigned int arraySlice = 0u;
			if (abGPU_Image_Dimensions_AreArray(dimensions)) {
				arraySlice = abGPU_Image_SliceLayer(slice);
			}
			if (abGPU_Image_Dimensions_AreCube(dimensions)) {
				arraySlice = arraySlice * 6u + abGPU_Image_SliceSide(slice);
			}
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = abGPU_Image_SliceMip(slice);
			desc.Texture2DArray.FirstArraySlice = arraySlice;
			desc.Texture2DArray.ArraySize = 1u;
			desc.Texture2DArray.PlaneSlice = 0u;
		}
		break;
	case abGPU_Image_Dimensions_3D:
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = abGPU_Image_SliceMip(slice);
		desc.Texture3D.FirstWSlice = abGPU_Image_SliceLayer(slice);
		desc.Texture3D.WSize = 1u;
		break;
	default:
		return false;
	}
	abGPU_RTStore_RT * rt = &store->renderTargets[rtIndex];
	rt->image = image;
	rt->slice = slice;
	ID3D12Device_CreateRenderTargetView(abGPUi_D3D_Device, image->i_resource, &desc, abGPUi_D3D_RTStore_GetDescriptorHandleColor(store, rtIndex));
	return true;
}

bool abGPU_RTStore_SetDepth(abGPU_RTStore * store, unsigned int rtIndex, abGPU_Image * image, abGPU_Image_Slice slice, bool readOnly) {
	if (rtIndex >= store->countDepth || !(image->typeAndDimensions & abGPU_Image_Type_Renderable) ||
			!abGPU_Image_Format_IsDepth(image->format) || !abGPUi_Image_HasSlice(image, slice)) {
		return false;
	}
	D3D12_DEPTH_STENCIL_VIEW_DESC desc;
	desc.Format = abGPUi_D3D_Image_FormatToDepthStencil(image->format);
	desc.Flags = (readOnly ? (D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL) : D3D12_DSV_FLAG_NONE);
	abGPU_Image_Dimensions dimensions = abGPU_Image_GetDimensions(image);
	switch (dimensions) {
	case abGPU_Image_Dimensions_2D:
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = abGPU_Image_SliceMip(slice);
		break;
	case abGPU_Image_Dimensions_2DArray:
	case abGPU_Image_Dimensions_Cube:
	case abGPU_Image_Dimensions_CubeArray:
		{
			unsigned int arraySlice = 0u;
			if (abGPU_Image_Dimensions_AreArray(dimensions)) {
				arraySlice = abGPU_Image_SliceLayer(slice);
			}
			if (abGPU_Image_Dimensions_AreCube(dimensions)) {
				arraySlice = arraySlice * 6u + abGPU_Image_SliceSide(slice);
			}
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = abGPU_Image_SliceMip(slice);
			desc.Texture2DArray.FirstArraySlice = arraySlice;
			desc.Texture2DArray.ArraySize = 1u;
		}
		break;
	default:
		return false;
	}
	abGPU_RTStore_RT * rt = &store->renderTargets[store->countColor + rtIndex];
	rt->image = image;
	rt->slice = slice;
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

bool abGPU_RTConfig_Register(abGPU_RTConfig * config, abGPU_RTStore const * store) {
	config->colorCount = abMin(config->colorCount, abGPU_RT_Count);
	config->i_rtStore = store;

	// Discarding 3D render targets is not supported.
	for (unsigned int rtIndex = 0; rtIndex < config->colorCount; ++rtIndex) {
		abGPU_RT * rt = &config->color[rtIndex];
		if (abGPU_Image_Dimensions_Are3D(abGPU_Image_GetDimensions(store->renderTargets[rt->indexInStore].image))) {
			abGPU_RT_PrePostAction * action = config->color[rtIndex].prePostAction;
			if ((*action & abGPU_RT_PreMask) == abGPU_RT_PreDiscard) {
				*action = (*action & ~abGPU_RT_PreMask) | abGPU_RT_PreLoad;
			}
			if ((*action & abGPU_RT_PostMask) == abGPU_RT_PostDiscard) {
				*action = (*action & ~abGPU_RT_PostMask) | abGPU_RT_PostStore;
			}
		}
	}

	// Do no action (= do load/store) on stencil if there's no stencil, also pre-calculate stencil subresource for discarding
	if (config->depth.indexInStore != abGPU_RTConfig_DepthIndexNone) {
		abGPU_RTStore_RT const * depthStoreRT = &store->renderTargets[config->depth.indexInStore];
		abGPU_Image const * depthImage = depthStoreRT->image;
		if (abGPU_Image_Format_IsDepthStencil(depthImage->format)) {
			abGPU_Image_Dimensions depthDimensions = abGPU_Image_GetDimensions(depthImage);
			config->i_stencilSubresource = (abGPU_Image_Dimensions_AreArray(depthDimensions) ? depthImage->d : 1) *
					(abGPU_Image_Dimensions_AreCube(depthDimensions) ? 6 : 1) * depthImage->mips +
					abGPUi_D3D_Image_SliceToSubresource(depthImage, depthStoreRT->slice);
		} else {
			config->stencilPrePostAction = abGPU_RT_PreLoad | abGPU_RT_PostStore;
		}
	}

	return true;
}

#endif
