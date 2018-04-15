#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"
#include "../../abMath/abBit.h"

/**************************
 * Command list management
 **************************/

abBool abGPU_CmdList_Init(abGPU_CmdList * list, abTextU8 const * name, abGPU_CmdQueue queue) {
	D3D12_COMMAND_LIST_TYPE type;
	switch (queue) {
	case abGPU_CmdQueue_Graphics:
		type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case abGPU_CmdQueue_Copy:
		type = D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	default:
		return abFalse;
	}
	if (FAILED(ID3D12Device_CreateCommandAllocator(abGPUi_D3D_Device, type, &IID_ID3D12CommandAllocator, &list->i_allocator))) {
		return abFalse;
	}
	abGPUi_D3D_SetObjectName(list->i_allocator, (abGPUi_D3D_ObjectNameSetter) list->i_allocator->lpVtbl->SetName, name);
	if (FAILED(ID3D12Device_CreateCommandList(abGPUi_D3D_Device, 0u, type, list->i_allocator, abNull,
			&IID_ID3D12GraphicsCommandList, &list->i_list))) {
		ID3D12CommandAllocator_Release(list->i_allocator);
		return abFalse;
	}
	abGPUi_D3D_SetObjectName(list->i_list, (abGPUi_D3D_ObjectNameSetter) list->i_list->lpVtbl->SetName, name);
	if (FAILED(ID3D12GraphicsCommandList_QueryInterface(list->i_list, &IID_ID3D12CommandList, &list->i_executeList))) {
		ID3D12CommandAllocator_Release(list->i_list);
		ID3D12CommandAllocator_Release(list->i_allocator);
		return abFalse;
	}
	ID3D12GraphicsCommandList_Close(list->i_list); // So it's not opened twice when it's used for the first time.
	return abTrue;
}

void abGPU_CmdList_Record(abGPU_CmdList * list) {
	ID3D12CommandAllocator_Reset(list->i_allocator);
	ID3D12GraphicsCommandList_Reset(list->i_list, list->i_allocator, abNull);
}

void abGPU_CmdList_Abort(abGPU_CmdList * list) {
	ID3D12GraphicsCommandList_Close(list->i_list);
}

void abGPU_CmdList_Submit(abGPU_CmdList * const * lists, unsigned int listCount) {
	if (listCount == 0u) {
		return;
	}
	if (listCount == 1u) {
		ID3D12GraphicsCommandList_Close(lists[0u]->i_list);
		ID3D12CommandQueue_ExecuteCommandLists(abGPUi_D3D_CommandQueues[lists[0u]->queue], 1u, &lists[0u]->i_executeList);
		return;
	}
	ID3D12CommandList * * executeLists = abStackAlloc(listCount * abGPU_CmdQueue_Count * sizeof(ID3D12CommandList *));
	unsigned int executeCounts[abGPU_CmdQueue_Count] = { 0 };
	for (unsigned int listIndex = 0u; listIndex < listCount; ++listIndex) {
		abGPU_CmdList * list = lists[listIndex];
		ID3D12GraphicsCommandList_Close(list->i_list);
		executeLists[list->queue * listCount + executeCounts[list->queue]++] = list->i_executeList;
	}
	for (unsigned int queueIndex = 0u; queueIndex < (unsigned int) abGPU_CmdQueue_Count; ++queueIndex) {
		if (executeCounts[queueIndex] != 0u) {
			ID3D12CommandQueue_ExecuteCommandLists(abGPUi_D3D_CommandQueues[queueIndex],
					executeCounts[queueIndex], executeLists + (queueIndex * listCount));
		}
	}
}

void abGPU_CmdList_Destroy(abGPU_CmdList * list) {
	ID3D12CommandList_Release(list->i_executeList);
	ID3D12GraphicsCommandList_Release(list->i_list);
	ID3D12CommandAllocator_Release(list->i_allocator);
}

/********
 * Setup
 ********/

void abGPU_Cmd_SetHandleAndSamplerStores(abGPU_CmdList * list, abGPU_HandleStore * handleStore, abGPU_SamplerStore * samplerStore) {
	ID3D12DescriptorHeap * heaps[2u];
	unsigned int heapCount = 0u;
	if (handleStore != abNull) {
		heaps[heapCount++] = handleStore->i_descriptorHeap;
	}
	if (samplerStore != abNull) {
		heaps[heapCount++] = samplerStore->i_descriptorHeap;
	}
	ID3D12GraphicsCommandList_SetDescriptorHeaps(list->i_list, heapCount, heaps);
}

/**********
 * Drawing
 **********/

void abGPU_Cmd_DrawingBegin(abGPU_CmdList * list, abGPU_RTConfig const * rtConfig) {
	ID3D12GraphicsCommandList * cmdList = list->i_list;
	list->i_drawRTConfig = rtConfig;

	int rtIndex;

	D3D12_DISCARD_REGION discardRegion = { .NumRects = 0u, .pRects = abNull, .NumSubresources = 1u };
	unsigned int preDiscardBits = rtConfig->i_preDiscardBits;
	while ((rtIndex = abBit_LowestOne32(preDiscardBits)) >= 0) {
		preDiscardBits &= ~(1u << rtIndex);
		discardRegion.FirstSubresource = rtConfig->i_subresources[rtIndex];
		ID3D12GraphicsCommandList_DiscardResource(cmdList, rtConfig->i_resources[rtIndex], &discardRegion);
	}

	ID3D12GraphicsCommandList_OMSetRenderTargets(cmdList, rtConfig->colorCount, rtConfig->i_descriptorHandles, FALSE,
			(rtConfig->depth.indexInStore != abGPU_RTConfig_DepthIndexNone) ? &rtConfig->i_descriptorHandles[abGPU_RT_Count] : abNull);

	unsigned int preClearColorBits = rtConfig->i_preClearBits & ((1u << abGPU_RT_Count) - 1u);
	while ((rtIndex = abBit_LowestOne32(preClearColorBits)) >= 0) {
		preClearColorBits &= ~(1u << rtIndex);
		ID3D12GraphicsCommandList_ClearRenderTargetView(cmdList, rtConfig->i_descriptorHandles[rtIndex],
				rtConfig->color[rtIndex].clearValue.color, 0u, abNull);
	}
	D3D12_CLEAR_FLAGS depthStencilClearFlags = 0u;
	if (rtConfig->i_preClearBits & (1u << abGPU_RT_Count)) {
		depthStencilClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
	}
	if (rtConfig->i_preClearBits & (1u << (abGPU_RT_Count + 1u))) {
		depthStencilClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
	}
	if (depthStencilClearFlags != 0u) {
		ID3D12GraphicsCommandList_ClearDepthStencilView(cmdList, rtConfig->i_descriptorHandles[abGPU_RT_Count], depthStencilClearFlags,
				rtConfig->depth.clearValue.ds.depth, rtConfig->depth.clearValue.ds.stencil, 0u, abNull);
	}
}

void abGPU_Cmd_DrawingEnd(abGPU_CmdList * list) {
	ID3D12GraphicsCommandList * cmdList = list->i_list;
	abGPU_RTConfig const * rtConfig = list->i_drawRTConfig;
	D3D12_DISCARD_REGION discardRegion = { .NumRects = 0u, .pRects = abNull, .NumSubresources = 1u };
	unsigned int postDiscardBits = rtConfig->i_preDiscardBits;
	int rtIndex;
	while ((rtIndex = abBit_LowestOne32(postDiscardBits)) >= 0) {
		postDiscardBits &= ~(1u << rtIndex);
		discardRegion.FirstSubresource = rtConfig->i_subresources[rtIndex];
		ID3D12GraphicsCommandList_DiscardResource(cmdList, rtConfig->i_resources[rtIndex], &discardRegion);
	}
}

/**********
 * Copying
 **********/

void abGPU_Cmd_CopyBuffer(abGPU_CmdList * list, abGPU_Buffer * target, abGPU_Buffer * source) {
	ID3D12GraphicsCommandList_CopyResource(list->i_list, target->i_resource, source->i_resource);
}

void abGPU_Cmd_CopyBufferRange(abGPU_CmdList * list, abGPU_Buffer * target, unsigned int targetOffset,
		abGPU_Buffer * source, unsigned int sourceOffset, unsigned int size) {
	ID3D12GraphicsCommandList_CopyBufferRegion(list->i_list, target->i_resource, targetOffset, source->i_resource, sourceOffset, size);
}

void abGPU_Cmd_CopyImage(abGPU_CmdList * list, abGPU_Image * target, abGPU_Image * source) {
	ID3D12GraphicsCommandList_CopyResource(list->i_list, target->i_resource, source->i_resource);
}

static void abGPUi_D3D_Cmd_FillImageCopyLocation(D3D12_TEXTURE_COPY_LOCATION * location,
		abGPU_Image * image, abGPU_Image_Slice slice, abBool stencil) {
	location->pResource = image->i_resource;
	if (image->options & abGPU_Image_Options_Upload) {
		// Stencil not supported in upload buffers, so don't care about that bit in the slice.
		location->Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		unsigned int mip = abGPU_Image_Slice_Mip(slice);
		location->PlacedFootprint.Offset = (((image->options & abGPU_Image_Options_Cube) ? 6u : 1u) * abGPU_Image_Slice_Layer(slice) +
				abGPU_Image_Slice_Side(slice)) * image->i_layerStride + image->i_mipOffset[mip];
		location->PlacedFootprint.Footprint.Format = image->i_copyFormat;
		abGPU_Image_GetMipSize(image, mip, &location->PlacedFootprint.Footprint.Width, &location->PlacedFootprint.Footprint.Height,
				&location->PlacedFootprint.Footprint.Depth);
		location->PlacedFootprint.Footprint.RowPitch = image->i_mipRowStride[mip];
	} else {
		location->Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		location->SubresourceIndex = abGPUi_D3D_Image_SliceToSubresource(image, slice, stencil);
	}
}

void abGPU_Cmd_CopyImageSlice(abGPU_CmdList * list, abGPU_Image * target, abGPU_Image_Slice targetSlice,
		abGPU_Image * source, abGPU_Image_Slice sourceSlice) {
	D3D12_TEXTURE_COPY_LOCATION targetLocation, sourceLocation;
	abGPUi_D3D_Cmd_FillImageCopyLocation(&targetLocation, target, targetSlice, abFalse);
	abGPUi_D3D_Cmd_FillImageCopyLocation(&sourceLocation, source, sourceSlice, abFalse);
	ID3D12GraphicsCommandList_CopyTextureRegion(list->i_list, &targetLocation, 0u, 0u, 0u, &sourceLocation, abNull);
}

void abGPU_Cmd_CopyImageDepth(abGPU_CmdList * list, abGPU_Image * target, abGPU_Image_Slice targetSlice,
		abGPU_Image * source, abGPU_Image_Slice sourceSlice, abBool depth, abBool stencil) {
	D3D12_TEXTURE_COPY_LOCATION targetLocation, sourceLocation;
	if (depth) {
		abGPUi_D3D_Cmd_FillImageCopyLocation(&targetLocation, target, targetSlice, abFalse);
		abGPUi_D3D_Cmd_FillImageCopyLocation(&sourceLocation, source, sourceSlice, abFalse);
		ID3D12GraphicsCommandList_CopyTextureRegion(list->i_list, &targetLocation, 0u, 0u, 0u, &sourceLocation, abNull);
	}
	if (stencil) {
		abGPUi_D3D_Cmd_FillImageCopyLocation(&targetLocation, target, targetSlice, abTrue);
		abGPUi_D3D_Cmd_FillImageCopyLocation(&sourceLocation, source, sourceSlice, abTrue);
		ID3D12GraphicsCommandList_CopyTextureRegion(list->i_list, &targetLocation, 0u, 0u, 0u, &sourceLocation, abNull);
	}
}

void abGPU_Cmd_CopyImageArea(abGPU_CmdList * list,
		abGPU_Image * target, abGPU_Image_Slice targetSlice, unsigned int targetX, unsigned int targetY, unsigned int targetZ,
		abGPU_Image * source, abGPU_Image_Slice sourceSlice, unsigned int sourceX, unsigned int sourceY, unsigned int sourceZ,
		unsigned int w, unsigned int h, unsigned int d) {
	D3D12_TEXTURE_COPY_LOCATION targetLocation, sourceLocation;
	abGPUi_D3D_Cmd_FillImageCopyLocation(&targetLocation, target, targetSlice, abFalse);
	abGPUi_D3D_Cmd_FillImageCopyLocation(&sourceLocation, source, sourceSlice, abFalse);
	D3D12_BOX sourceBox = { .left = sourceX, .top = sourceY, .front = sourceZ, .right = sourceX + w, .bottom = sourceY + h, .back = sourceZ + d };
	ID3D12GraphicsCommandList_CopyTextureRegion(list->i_list, &targetLocation, targetX, targetY, targetZ, &sourceLocation, &sourceBox);
}

#endif
