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
	ID3D12GraphicsCommandList_Close(list->i_list); // So it's not opened twice when it's recorded for the first time.
	return abTrue;
}

void abGPU_CmdList_Record(abGPU_CmdList * list) {
	ID3D12CommandAllocator_Reset(list->i_allocator);
	ID3D12GraphicsCommandList_Reset(list->i_list, list->i_allocator, abNull);
	list->i_drawConfig = abNull;
	list->i_computeConfig = abNull;
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

/******************
 * Usage switching
 ******************/

void abGPU_Cmd_UsageControl(abGPU_CmdList * list, unsigned int actionCount, abGPU_Cmd_UsageControl_Action const * actions) {
	D3D12_RESOURCE_BARRIER * barriers = abStackAlloc(actionCount * sizeof(D3D12_RESOURCE_BARRIER));
	unsigned int barrierCount = 0u; // Excluding switches into the same usage.
	for (unsigned int actionIndex = 0u; actionIndex < actionCount; ++actionIndex) {
		abGPU_Cmd_UsageControl_Action const * action = &actions[actionIndex];
		D3D12_RESOURCE_BARRIER * barrier = &barriers[barrierCount];
		if (action->time == abGPU_Cmd_UsageControl_Time_Start) {
			barrier->Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
		} else if (action->time == abGPU_Cmd_UsageControl_Time_Finish) {
			barrier->Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
		} else {
			barrier->Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		}
		switch (action->type) {
		case abGPU_Cmd_UsageControl_Type_BufferSwitch:
			barrier->Transition.StateBefore = abGPUi_D3D_Buffer_UsageToStates(action->parameters.bufferSwitch.oldUsage);
			barrier->Transition.StateAfter = abGPUi_D3D_Buffer_UsageToStates(action->parameters.bufferSwitch.newUsage);
			if (barrier->Transition.StateBefore == barrier->Transition.StateAfter) {
				continue; // Vertex buffer and constant buffer are the same states, for instance.
			}
			barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier->Transition.pResource = action->parameters.bufferSwitch.buffer->i_resource;
			barrier->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			break;
		case abGPU_Cmd_UsageControl_Type_ImageSwitch:
			barrier->Transition.StateBefore = abGPUi_D3D_Image_UsageToStates(action->parameters.imageSwitch.oldUsage);
			barrier->Transition.StateAfter = abGPUi_D3D_Image_UsageToStates(action->parameters.imageSwitch.newUsage);
			if (barrier->Transition.StateBefore == barrier->Transition.StateAfter) {
				continue;
			}
			barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier->Transition.pResource = action->parameters.imageSwitch.image->i_resource;
			barrier->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			break;
		case abGPU_Cmd_UsageControl_Type_BufferEditCommit:
			barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier->UAV.pResource = action->parameters.bufferEditCommit.buffer->i_resource;
			break;
		case abGPU_Cmd_UsageControl_Type_ImageEditCommit:
			barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier->UAV.pResource = action->parameters.imageEditCommit.image->i_resource;
			break;
		default:
			continue;
		}
		++barrierCount; // Continue to prevent if the action isn't valid.
	}
	if (barrierCount != 0u) {
		ID3D12GraphicsCommandList_ResourceBarrier(list->i_list, barrierCount, barriers);
	}
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
	list->i_drawConfig = abNull;
}

abBool abGPU_Cmd_DrawSetConfig(abGPU_CmdList * list, abGPU_DrawConfig * drawConfig) {
	abBool inputDifferent = (list->i_drawConfig == abNull || list->i_drawConfig->inputConfig != drawConfig->inputConfig);
	list->i_drawConfig = drawConfig;
	ID3D12GraphicsCommandList_SetPipelineState(list->i_list, drawConfig->i_pipelineState);
	if (inputDifferent) {
		ID3D12GraphicsCommandList_SetGraphicsRootSignature(list->i_list, drawConfig->inputConfig->i_rootSignature);
	}
	return inputDifferent;
}

void abGPU_Cmd_DrawSetTopology(abGPU_CmdList * list, abGPU_Cmd_Topology topology) {
	D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
	switch (topology) {
	case abGPU_Cmd_Topology_TriangleList: primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
	case abGPU_Cmd_Topology_TriangleStrip: primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
	case abGPU_Cmd_Topology_PointList: primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
	case abGPU_Cmd_Topology_LineList: primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
	case abGPU_Cmd_Topology_LineStrip: primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
	default: return;
	}
	ID3D12GraphicsCommandList_IASetPrimitiveTopology(list->i_list, primitiveTopology);
}

void abGPU_Cmd_DrawSetVertexIndices(abGPU_CmdList * list, abGPU_Buffer * buffer, unsigned int offset, unsigned int indexCount) {
	if (list->i_drawConfig == abNull) {
		return;
	}
	D3D12_INDEX_BUFFER_VIEW view;
	view.BufferLocation = buffer->i_gpuVirtualAddress + offset;
	if (list->i_drawConfig->options & abGPU_DrawConfig_Options_16BitVertexIndices) {
		view.SizeInBytes = indexCount * (unsigned int) sizeof(uint16_t);
		view.Format = DXGI_FORMAT_R16_UINT;
	} else {
		view.SizeInBytes = indexCount * (unsigned int) sizeof(uint32_t);
		view.Format = DXGI_FORMAT_R32_UINT;
	}
	ID3D12GraphicsCommandList_IASetIndexBuffer(list->i_list, &view);
}

void abGPU_Cmd_DrawSetViewport(abGPU_CmdList * list, float x, float y, float w, float h, float zZero, float zOne) {
	D3D12_VIEWPORT viewport = { .TopLeftX = x, .TopLeftY = y, .Width = w, .Height = h, .MinDepth = zZero, .MaxDepth = zOne };
	ID3D12GraphicsCommandList_RSSetViewports(list->i_list, 1u, &viewport);
}

void abGPU_Cmd_DrawSetScissor(abGPU_CmdList * list, int x, int y, unsigned int w, unsigned int h) {
	D3D12_RECT scissor = { .left = x, .top = y, .right = x + w, .bottom = y + h };
	ID3D12GraphicsCommandList_RSSetScissorRects(list->i_list, 1u, &scissor);
}

void abGPU_Cmd_DrawSetStencilReference(abGPU_CmdList * list, uint8_t reference) {
	ID3D12GraphicsCommandList_OMSetStencilRef(list->i_list, reference);
}

void abGPU_Cmd_DrawSetBlendConstant(abGPU_CmdList * list, float const rgba[4u]) {
	ID3D12GraphicsCommandList_OMSetBlendFactor(list->i_list, rgba);
}

void abGPU_Cmd_DrawIndexed(abGPU_CmdList * list, unsigned int indexCount, unsigned int firstIndex, int vertexIndexOffset,
		unsigned int instanceCount, unsigned int firstInstance) {
	ID3D12GraphicsCommandList_DrawIndexedInstanced(list->i_list, indexCount, instanceCount, firstIndex, vertexIndexOffset, firstInstance);
}

void abGPU_Cmd_DrawSequential(abGPU_CmdList * list, unsigned int vertexCount, unsigned int firstVertex,
		unsigned int instanceCount, unsigned int firstInstance) {
	ID3D12GraphicsCommandList_DrawInstanced(list->i_list, vertexCount, instanceCount, firstVertex, firstInstance);
}

/************
 * Computing
 ************/

void abGPU_Cmd_ComputingEnd(abGPU_CmdList * list) {
	list->i_computeConfig = abNull;
}

abBool abGPU_Cmd_ComputeSetConfig(abGPU_CmdList * list, abGPU_ComputeConfig * computeConfig) {
	abBool inputDifferent = (list->i_computeConfig == abNull || list->i_computeConfig->inputConfig != computeConfig->inputConfig);
	list->i_computeConfig = computeConfig;
	ID3D12GraphicsCommandList_SetPipelineState(list->i_list, computeConfig->i_pipelineState);
	if (inputDifferent) {
		ID3D12GraphicsCommandList_SetComputeRootSignature(list->i_list, computeConfig->inputConfig->i_rootSignature);
	}
	return inputDifferent;
}

void abGPU_Cmd_Compute(abGPU_CmdList * list, unsigned int groupCountX, unsigned int groupCountY, unsigned int groupCountZ) {
	ID3D12GraphicsCommandList_Dispatch(list->i_list, groupCountX, groupCountY, groupCountZ);
}

/*********
 * Inputs
 *********/

static abGPU_InputConfig * abGPUi_D3D_CmdList_GetInputConfig(abGPU_CmdList * list, abBool * compute) {
	if (list->i_drawConfig != abNull) {
		*compute = abFalse;
		return list->i_drawConfig->inputConfig;
	}
	return abNull;
}

void abGPU_Cmd_InputUniform32BitValues(abGPU_CmdList * list, void const * values) {
	abGPU_InputConfig const * inputConfig;
	abBool compute;
	if ((inputConfig = abGPUi_D3D_CmdList_GetInputConfig(list, &compute)) == abNull) {
		return;
	}
	unsigned int valueCount = inputConfig->uniform32BitCount;
	if (valueCount == 0u) {
		return;
	}
	if (compute) {
		ID3D12GraphicsCommandList_SetComputeRoot32BitConstants(list->i_list, 0u, valueCount, values, 0u);
	} else {
		ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(list->i_list, 0u, valueCount, values, 0u);
	}
}

void abGPU_Cmd_InputUniformBuffer(abGPU_CmdList * list, abGPU_Buffer * buffer, unsigned int offset, unsigned int size) {
	abGPU_InputConfig const * inputConfig;
	abBool compute;
	if ((inputConfig = abGPUi_D3D_CmdList_GetInputConfig(list, &compute)) == abNull || !inputConfig->uniformUseBuffer) {
		return;
	}
	unsigned int rootParameterIndex = ((inputConfig->uniform32BitCount != 0u) ? 1u : 0u);
	D3D12_GPU_VIRTUAL_ADDRESS address = buffer->i_gpuVirtualAddress + offset;
	if (compute) {
		ID3D12GraphicsCommandList_SetComputeRootConstantBufferView(list->i_list, rootParameterIndex, address);
	} else {
		ID3D12GraphicsCommandList_SetGraphicsRootConstantBufferView(list->i_list, rootParameterIndex, address);
	}
}

#define abGPUi_D3D_CmdList_GetRootParameter_Invalid UINT_MAX

static unsigned int abGPUi_D3D_CmdList_GetRootParameter(abGPU_CmdList * list, unsigned int inputIndex, abGPU_Input_Type type, abBool * compute) {
	abGPU_InputConfig const * inputConfig;
	if ((inputConfig = abGPUi_D3D_CmdList_GetInputConfig(list, compute)) == abNull) {
		return abGPUi_D3D_CmdList_GetRootParameter_Invalid;
	}
	if (inputIndex >= inputConfig->inputCount || inputConfig->inputs[inputIndex].type != type) {
		return abGPUi_D3D_CmdList_GetRootParameter_Invalid;
	}
	unsigned int rootParameter = inputConfig->i_rootParameters[inputIndex];
	if (rootParameter == UINT8_MAX) {
		return abGPUi_D3D_CmdList_GetRootParameter_Invalid;
	}
	return rootParameter;
}

void abGPU_Cmd_InputConstantBuffer(abGPU_CmdList * list, unsigned int inputIndex, abGPU_Buffer * buffer, unsigned int offset, unsigned int size) {
	abBool compute;
	unsigned int rootParameter = abGPUi_D3D_CmdList_GetRootParameter(list, inputIndex, abGPU_Input_Type_ConstantBuffer, &compute);
	if (rootParameter == abGPUi_D3D_CmdList_GetRootParameter_Invalid) {
		return;
	}
	D3D12_GPU_VIRTUAL_ADDRESS address = buffer->i_gpuVirtualAddress + offset;
	if (compute) {
		ID3D12GraphicsCommandList_SetComputeRootConstantBufferView(list->i_list, rootParameter, address);
	} else {
		ID3D12GraphicsCommandList_SetGraphicsRootConstantBufferView(list->i_list, rootParameter, address);
	}
}

static void abGPUi_D3D_CmdList_InputHandles(abGPU_CmdList * list, unsigned int inputIndex, abGPU_Input_Type type, unsigned int firstHandleIndex) {
	if (list->i_handleStore == abNull) {
		return;
	}
	abBool compute;
	unsigned int rootParameter = abGPUi_D3D_CmdList_GetRootParameter(list, inputIndex, type, &compute);
	if (rootParameter == abGPUi_D3D_CmdList_GetRootParameter_Invalid) {
		return;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle = abGPUi_D3D_HandleStore_GetGPUDescriptorHandle(list->i_handleStore, firstHandleIndex);
	if (compute) {
		ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(list->i_list, rootParameter, descriptorHandle);
	} else {
		ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(list->i_list, rootParameter, descriptorHandle);
	}
}

void abGPU_Cmd_InputConstantBufferHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex) {
	abGPUi_D3D_CmdList_InputHandles(list, inputIndex, abGPU_Input_Type_ConstantBufferHandle, firstHandleIndex);
}

void abGPU_Cmd_InputStructureBufferHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex) {
	abGPUi_D3D_CmdList_InputHandles(list, inputIndex, abGPU_Input_Type_StructureBufferHandle, firstHandleIndex);
}

void abGPU_Cmd_InputEditBufferHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex) {
	abGPUi_D3D_CmdList_InputHandles(list, inputIndex, abGPU_Input_Type_EditBufferHandle, firstHandleIndex);
}

void abGPU_Cmd_InputTextureHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex) {
	abGPUi_D3D_CmdList_InputHandles(list, inputIndex, abGPU_Input_Type_TextureHandle, firstHandleIndex);
}

void abGPU_Cmd_InputEditImageHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex) {
	abGPUi_D3D_CmdList_InputHandles(list, inputIndex, abGPU_Input_Type_EditImageHandle, firstHandleIndex);
}

void abGPU_Cmd_InputSamplers(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstSamplerIndex) {
	if (list->i_samplerStore == abNull) {
		return;
	}
	abBool compute;
	unsigned int rootParameter = abGPUi_D3D_CmdList_GetRootParameter(list, inputIndex, abGPU_Input_Type_SamplerHandle, &compute);
	if (rootParameter == abGPUi_D3D_CmdList_GetRootParameter_Invalid) { // Will catch static samplers.
		return;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle = abGPUi_D3D_SamplerStore_GetGPUDescriptorHandle(list->i_samplerStore, firstSamplerIndex);
	if (compute) {
		ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(list->i_list, rootParameter, descriptorHandle);
	} else {
		ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(list->i_list, rootParameter, descriptorHandle);
	}
}

void abGPU_Cmd_InputVertexData(abGPU_CmdList * list, unsigned int firstBufferIndex, unsigned int bufferCount,
		abGPU_Buffer * const * buffers, unsigned int const * offsets, unsigned int vertexCount, unsigned int instanceCount) {
	if (list->i_drawConfig == abNull) {
		return;
	}
	abGPU_InputConfig const * inputConfig = list->i_drawConfig->inputConfig;
	if (inputConfig == abNull) {
		return;
	}
	if (firstBufferIndex > inputConfig->vertexBufferCount) {
		return;
	}
	bufferCount = abMin(inputConfig->vertexBufferCount - firstBufferIndex, bufferCount);
	D3D12_VERTEX_BUFFER_VIEW * views = abStackAlloc(bufferCount * sizeof(D3D12_VERTEX_BUFFER_VIEW));
	for (unsigned int bufferIndex = 0u; bufferIndex < bufferCount; ++bufferIndex) {
		D3D12_VERTEX_BUFFER_VIEW * view = &views[bufferIndex];
		abGPU_VertexData_Buffer const * buffer = &inputConfig->vertexBuffers[firstBufferIndex + bufferIndex];
		view->BufferLocation = buffers[bufferIndex]->i_gpuVirtualAddress + ((offsets != abNull) ? offsets[bufferIndex] : 0u);
		switch (buffer->instanceRate) {
		case 0u:
			view->SizeInBytes = vertexCount * buffer->stride;
			break;
		case 1u:
			view->SizeInBytes = instanceCount * buffer->stride;
			break;
		default:
			{
				unsigned int instanceSequenceCount = instanceCount / buffer->instanceRate;
				if ((instanceSequenceCount * buffer->instanceRate) < instanceCount) {
					++instanceSequenceCount;
				}
				view->SizeInBytes = instanceSequenceCount * buffer->stride;
			}
		}
		view->StrideInBytes = buffer->stride;
	}
	ID3D12GraphicsCommandList_IASetVertexBuffers(list->i_list, firstBufferIndex, bufferCount, views);
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
