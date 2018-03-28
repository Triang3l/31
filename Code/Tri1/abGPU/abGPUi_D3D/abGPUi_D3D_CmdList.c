#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"
#include "../../abMath/abBit.h"

bool abGPU_CmdList_Init(abGPU_CmdList * list, abGPU_CmdQueue queue) {
	D3D12_COMMAND_LIST_TYPE type;
	switch (queue) {
	case abGPU_CmdQueue_Graphics:
		type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case abGPU_CmdQueue_Copy:
		type = D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	default:
		return false;
	}
	if (FAILED(ID3D12Device_CreateCommandAllocator(abGPUi_D3D_Device, type, &IID_ID3D12CommandAllocator, &list->i_allocator))) {
		return false;
	}
	if (FAILED(ID3D12Device_CreateCommandList(abGPUi_D3D_Device, 0u, type, list->i_allocator, abNull,
			&IID_ID3D12GraphicsCommandList, &list->i_list))) {
		ID3D12CommandAllocator_Release(list->i_allocator);
		return false;
	}
	if (FAILED(ID3D12GraphicsCommandList_QueryInterface(list->i_list, &IID_ID3D12CommandList, &list->i_executeList))) {
		ID3D12CommandAllocator_Release(list->i_list);
		ID3D12CommandAllocator_Release(list->i_allocator);
		return false;
	}
	ID3D12GraphicsCommandList_Close(list->i_list); // So it's not opened twice when it's used for the first time.
	return true;
}

void abGPU_CmdList_Record(abGPU_CmdList * list) {
	ID3D12CommandAllocator_Reset(list->i_allocator);
	ID3D12GraphicsCommandList_Reset(list->i_list, list->i_allocator, abNull);
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

void abGPU_Cmd_SetHandleAndSamplerStores(abGPU_CmdList * list,
		abGPU_HandleStore * handleStore, abGPU_SamplerStore * samplerStore) {
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

#endif
