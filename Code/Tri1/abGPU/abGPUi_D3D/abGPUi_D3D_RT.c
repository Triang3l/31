#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

unsigned int abGPUi_D3D_RTStore_DescriptorSizeColor, abGPUi_D3D_RTStore_DescriptorSizeDepth;

abBool abGPU_RTStore_Init(abGPU_RTStore * store, unsigned int countColor, unsigned int countDepth) {
	D3D12_DESCRIPTOR_HEAP_DESC desc = { 0 };
	if (countColor != 0u) {
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = countColor;
		if (FAILED(ID3D12Device_CreateDescriptorHeap(abGPUi_D3D_Device, &desc, &IID_ID3D12DescriptorHeap, &store->i_descriptorHeapColor))) {
			return abFalse;
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
			return abFalse;
		}
		store->i_cpuDescriptorHandleStartDepth = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(store->i_descriptorHeapDepth);
	} else {
		store->i_descriptorHeapDepth = abNull;
		store->i_cpuDescriptorHandleStartDepth.ptr = 0u;
	}
	store->renderTargets = abMemory_Alloc(abGPUi_D3D_MemoryTag, (countColor + countDepth) * sizeof(abGPU_RTStore_RT), abFalse);
	return abTrue;
}

abBool abGPU_RTStore_SetColor(abGPU_RTStore * store, unsigned int rtIndex, abGPU_Image * image, abGPU_Image_Slice slice) {
	abGPU_Image_Options imageOptions = image->options;
	if (rtIndex >= store->countColor || !(imageOptions & abGPU_Image_Options_Renderable) ||
			abGPU_Image_Format_IsDepth(image->format) || !abGPU_Image_HasSlice(image, slice)) {
		return abFalse;
	}
	D3D12_RENDER_TARGET_VIEW_DESC desc;
	desc.Format = abGPUi_D3D_Image_FormatToResource(image->format);
	if (imageOptions & abGPU_Image_Options_3D) {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = abGPU_Image_Slice_Mip(slice);
		desc.Texture3D.FirstWSlice = abGPU_Image_Slice_Layer(slice);
		desc.Texture3D.WSize = 1u;
	} else if (imageOptions & (abGPU_Image_Options_Array | abGPU_Image_Options_Cube)) {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipSlice = abGPU_Image_Slice_Mip(slice);
		desc.Texture2DArray.FirstArraySlice = 0u;
		if (imageOptions & abGPU_Image_Options_Array) {
			desc.Texture2DArray.FirstArraySlice = abGPU_Image_Slice_Layer(slice);
		}
		if (imageOptions & abGPU_Image_Options_Cube) {
			desc.Texture2DArray.FirstArraySlice = desc.Texture2DArray.FirstArraySlice * 6u + abGPU_Image_Slice_Side(slice);
		}
		desc.Texture2DArray.ArraySize = 1u;
		desc.Texture2DArray.PlaneSlice = 0u;
	} else {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = abGPU_Image_Slice_Mip(slice);
		desc.Texture2D.PlaneSlice = 0u;
	}
	abGPU_RTStore_RT * rt = &store->renderTargets[rtIndex];
	rt->image = image;
	rt->slice = slice;
	ID3D12Device_CreateRenderTargetView(abGPUi_D3D_Device, image->i_resource, &desc, abGPUi_D3D_RTStore_GetDescriptorHandleColor(store, rtIndex));
	return abTrue;
}

abBool abGPU_RTStore_SetDepth(abGPU_RTStore * store, unsigned int rtIndex, abGPU_Image * image, abGPU_Image_Slice slice, abBool readOnly) {
	abGPU_Image_Options imageOptions = image->options;
	if (rtIndex >= store->countDepth || (imageOptions & abGPU_Image_Options_3D) || !(imageOptions & abGPU_Image_Options_Renderable) ||
			!abGPU_Image_Format_IsDepth(image->format) || !abGPU_Image_HasSlice(image, slice)) {
		return abFalse;
	}
	D3D12_DEPTH_STENCIL_VIEW_DESC desc;
	desc.Format = abGPUi_D3D_Image_FormatToDepthStencil(image->format);
	desc.Flags = (readOnly ? (D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL) : D3D12_DSV_FLAG_NONE);
	if (imageOptions & (abGPU_Image_Options_Array | abGPU_Image_Options_Cube)) {
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipSlice = abGPU_Image_Slice_Mip(slice);
		desc.Texture2DArray.FirstArraySlice = 0u;
		if (imageOptions & abGPU_Image_Options_Array) {
			desc.Texture2DArray.FirstArraySlice = abGPU_Image_Slice_Layer(slice);
		}
		if (imageOptions & abGPU_Image_Options_Cube) {
			desc.Texture2DArray.FirstArraySlice = desc.Texture2DArray.FirstArraySlice * 6u + abGPU_Image_Slice_Side(slice);
		}
		desc.Texture2DArray.ArraySize = 1u;
	} else {
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = abGPU_Image_Slice_Mip(slice);
	}
	abGPU_RTStore_RT * rt = &store->renderTargets[store->countColor + rtIndex];
	rt->image = image;
	rt->slice = slice;
	ID3D12Device_CreateDepthStencilView(abGPUi_D3D_Device, image->i_resource, &desc, abGPUi_D3D_RTStore_GetDescriptorHandleDepth(store, rtIndex));
	return abTrue;
}

void abGPU_RTStore_Destroy(abGPU_RTStore * store) {
	if (store->countDepth != 0u) {
		ID3D12DescriptorHeap_Release(store->i_descriptorHeapDepth);
	}
	if (store->countColor != 0u) {
		ID3D12DescriptorHeap_Release(store->i_descriptorHeapColor);
	}
}

static void abGPUi_D3D_RTConfig_PrePostActionToBits(abGPU_RTConfig * config, abGPU_RT_PrePostAction action, unsigned int shift) {
	unsigned int bit = 1u << shift;
	switch (action & abGPU_RT_PreMask) {
	case abGPU_RT_PreDiscard:
		config->i_preDiscardBits |= bit;
		break;
	case abGPU_RT_PreClear:
		config->i_preClearBits |= bit;
		break;
	}
	if ((action & abGPU_RT_PostMask) == abGPU_RT_PostDiscard) {
		config->i_postDiscardBits |= bit;
	}
}

abBool abGPU_RTConfig_Register(abGPU_RTConfig * config, abGPU_RTStore const * store) {
	if (config->colorCount > abGPU_RT_Count) {
		return abFalse;
	}

	config->i_preDiscardBits = config->i_preClearBits = config->i_postDiscardBits = 0u;

	for (unsigned int rtIndex = 0; rtIndex < config->colorCount; ++rtIndex) {
		abGPU_RT * rt = &config->color[rtIndex];
		abGPU_RTStore_RT const * storeRT = &store->renderTargets[rt->indexInStore];
		config->i_descriptorHandles[rtIndex].ptr = store->i_cpuDescriptorHandleStartColor.ptr +
				rt->indexInStore * abGPUi_D3D_RTStore_DescriptorSizeColor;
		config->i_resources[rtIndex] = storeRT->image->i_resource;
		config->i_subresources[rtIndex] = abGPUi_D3D_Image_SliceToSubresource(storeRT->image, storeRT->slice, abFalse);
		// Discarding 3D render targets is not supported because D3D12_RECT is 2D.
		abGPU_RT_PrePostAction * action = &config->color[rtIndex].prePostAction;
		if (storeRT->image->options & abGPU_Image_Options_3D) {
			if ((*action & abGPU_RT_PreMask) == abGPU_RT_PreDiscard) {
				*action = (*action & ~abGPU_RT_PreMask) | abGPU_RT_PreLoad;
			}
			if ((*action & abGPU_RT_PostMask) == abGPU_RT_PostDiscard) {
				*action = (*action & ~abGPU_RT_PostMask) | abGPU_RT_PostStore;
			}
		}
		abGPUi_D3D_RTConfig_PrePostActionToBits(config, *action, rtIndex);
	}

	if (config->depth.indexInStore != abGPU_RTConfig_DepthIndexNone) {
		abGPU_RTStore_RT const * storeRT = &store->renderTargets[config->depth.indexInStore];
		config->i_descriptorHandles[abGPU_RT_Count].ptr = store->i_cpuDescriptorHandleStartDepth.ptr +
				config->depth.indexInStore * abGPUi_D3D_RTStore_DescriptorSizeDepth;
		config->i_resources[abGPU_RT_Count] = storeRT->image->i_resource;
		config->i_subresources[abGPU_RT_Count] = abGPUi_D3D_Image_SliceToSubresource(storeRT->image, storeRT->slice, abFalse);
		abGPUi_D3D_RTConfig_PrePostActionToBits(config, config->depth.prePostAction, abGPU_RT_Count);
		if (abGPU_Image_Format_IsDepthStencil(storeRT->image->format)) {
			config->i_descriptorHandles[abGPU_RT_Count + 1] = config->i_descriptorHandles[abGPU_RT_Count];
			config->i_resources[abGPU_RT_Count + 1] = config->i_resources[abGPU_RT_Count];
			config->i_subresources[abGPU_RT_Count + 1] = abGPUi_D3D_Image_SliceToSubresource(storeRT->image, storeRT->slice, abTrue);
			abGPUi_D3D_RTConfig_PrePostActionToBits(config, config->stencilPrePostAction, abGPU_RT_Count + 1);
		} else {
			// Do no action (= do load/store) on stencil if there's no stencil.
			config->stencilPrePostAction = abGPU_RT_PreLoad | abGPU_RT_PostStore;
		}
	}

	return abTrue;
}

unsigned int abGPU_DisplayChain_GetCurrentImageIndex(abGPU_DisplayChain * chain) {
	return IDXGISwapChain3_GetCurrentBackBufferIndex(chain->i_swapChain);
}

void abGPU_DisplayChain_Display(abGPU_DisplayChain * chain, abBool verticalSync) {
	IDXGISwapChain3_Present(chain->i_swapChain, verticalSync ? 1u : 0u, 0u);
}

void abGPU_DisplayChain_Shutdown(abGPU_DisplayChain * chain) {
	for (unsigned int imageIndex = 0; imageIndex < chain->imageCount; ++imageIndex) {
		abGPU_Image_Destroy(&chain->images[imageIndex]);
	}
	IDXGISwapChain3_Release(chain->i_swapChain);
}

#endif
