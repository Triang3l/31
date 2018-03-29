#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

abBool abGPU_Fence_Init(abGPU_Fence * fence, abGPU_CmdQueue queue) {
	if (FAILED(ID3D12Device_CreateFence(abGPUi_D3D_Device, 0ull, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, &fence->i_fence))) {
		return abFalse;
	}
	fence->i_completionEvent = CreateEvent(abNull, FALSE, FALSE, abNull);
	if (fence->i_completionEvent == abNull) {
		ID3D12Fence_Release(fence->i_fence);
		return abFalse;
	}
	fence->queue = queue;
	fence->i_awaitedValue = 0ull;
	return abTrue;
}

void abGPU_Fence_Enqueue(abGPU_Fence * fence) {
	ID3D12CommandQueue_Signal(abGPUi_D3D_CommandQueues[fence->queue], fence->i_fence, ++fence->i_awaitedValue);
}

abBool abGPU_Fence_IsCrossed(abGPU_Fence * fence) {
	return ID3D12Fence_GetCompletedValue(fence->i_fence) >= fence->i_awaitedValue;
}

void abGPU_Fence_Await(abGPU_Fence * fence) {
	if (ID3D12Fence_GetCompletedValue(fence->i_fence) >= fence->i_awaitedValue) {
		return;
	}
	ID3D12Fence_SetEventOnCompletion(fence->i_fence, fence->i_awaitedValue, fence->i_completionEvent);
	WaitForSingleObject(fence->i_completionEvent, INFINITE);
}

void abGPU_Fence_Destroy(abGPU_Fence * fence) {
	ID3D12Fence_Release(fence->i_fence);
	CloseHandle(fence->i_completionEvent);
}

#endif
