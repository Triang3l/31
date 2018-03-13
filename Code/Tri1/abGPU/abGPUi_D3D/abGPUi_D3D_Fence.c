#ifdef abBuild_GPUi_D3D
#include "../abGPU.h"

bool abGPU_Fence_Init(abGPU_Fence *fence, abGPU_CmdQueue queue) {
	fence->queue = queue;
	if (FAILED(ID3D12Device_CreateFence(abGPUi_D3D_Device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, &fence->fence))) {
		return false;
	}
	fence->completionEvent = CreateEvent(abNull, FALSE, FALSE, abNull);
	if (fence->completionEvent == abNull) {
		ID3D12Fence_Release(fence->fence);
		return false;
	}
	fence->awaitedValue = 0;
	return true;
}

void abGPU_Fence_Destroy(abGPU_Fence *fence) {
	ID3D12Fence_Release(fence->fence);
	CloseHandle(fence->completionEvent);
}

void abGPU_Fence_Enqueue(abGPU_Fence *fence) {
	ID3D12CommandQueue_Signal(abGPUi_D3D_CommandQueues[fence->queue], fence->fence, ++fence->awaitedValue);
}

bool abGPU_Fence_IsCrossed(abGPU_Fence *fence) {
	return ID3D12Fence_GetCompletedValue(fence->fence) >= fence->awaitedValue;
}

void abGPU_Fence_Await(abGPU_Fence *fence) {
	if (ID3D12Fence_GetCompletedValue(fence->fence) >= fence->awaitedValue) {
		return;
	}
	ID3D12Fence_SetEventOnCompletion(fence->fence, fence->awaitedValue, fence->completionEvent);
	WaitForSingleObject(fence->completionEvent, INFINITE);
}

#endif
