#ifdef abBuild_GPUi_D3D
#include "../abGPU.h"

IDXGIFactory2 *abGPUi_D3D_DXGIFactory = abNull;
IDXGIAdapter3 *abGPUi_D3D_DXGIAdapterMain = abNull;
ID3D12Device *abGPUi_D3D_Device = abNull;
ID3D12CommandQueue *abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Count] = { 0 };

enum {
	abGPUi_D3D_Init_Result_DXGIFactoryCreationFailed = 1,
	abGPUi_D3D_Init_Result_AdapterNotFound,
	abGPUi_D3D_Init_Result_AdapterInterfaceQueryFailed,
	abGPUi_D3D_Init_Result_DeviceCreationFailed,
	abGPUi_D3D_Init_Result_GraphicsCommandQueueCreationFailed,
	abGPUi_D3D_Init_Result_CopyCommandQueueCreationFailed
};

abGPU_Init_Result abGPU_Init(bool debug) {
	if (abGPUi_D3D_Device != abNull) {
		return abGPU_Init_Result_Success;
	}

	if (debug) {
		ID3D12Debug *debugInterface;
		if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug, &debugInterface))) {
			ID3D12Debug_EnableDebugLayer(debugInterface);
			ID3D12Debug_Release(debugInterface);
		} else {
			debug = false;
		}
	}

	if (FAILED(CreateDXGIFactory2(debug ? DXGI_CREATE_FACTORY_DEBUG : 0, &IID_IDXGIFactory2, &abGPUi_D3D_DXGIFactory))) {
		return abGPUi_D3D_Init_Result_DXGIFactoryCreationFailed;
	}

	{
		IDXGIAdapter1 *adapter = abNull, *softwareAdapter = abNull;
		unsigned int adapterIndex = 0;
		while (IDXGIFactory2_EnumAdapters1(abGPUi_D3D_DXGIFactory, adapterIndex, &adapter) == S_OK) {
			if (SUCCEEDED(D3D12CreateDevice((IUnknown *) adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, abNull))) {
				DXGI_ADAPTER_DESC1 adapterDesc;
				IDXGIAdapter1_GetDesc1(adapter, &adapterDesc);
				if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
					if (softwareAdapter == abNull) { // Otherwise won't release the previous one if there are multiple.
						softwareAdapter = adapter;
					}
					// The software adapter is only used as a last resort - keep searching.
				} else {
					// Found a hardware adapter - keep it as `adapter`.
					break;
				}
			}
			// This adapter either doesn't support D3D11 or was chosen as a last resort software adapter.
			if (adapter != softwareAdapter) {
				IDXGIAdapter1_Release(adapter);
			}
			adapter = abNull;
			++adapterIndex;
		}
		if (adapter != abNull) {
			// Found a hardware adapter.
			if (softwareAdapter != abNull) {
				IDXGIAdapter1_Release(softwareAdapter);
			}
		} else {
			if (softwareAdapter == abNull) {
				abGPU_Shutdown();
				return abGPUi_D3D_Init_Result_AdapterNotFound;
			}
			adapter = softwareAdapter;
		}
		// Query adapter interface version 3 for memory information access.
		if (FAILED(IDXGIAdapter1_QueryInterface(adapter, &IID_IDXGIAdapter3, &abGPUi_D3D_DXGIAdapterMain))) {
			IDXGIAdapter1_Release(adapter);
			abGPU_Shutdown();
			return abGPUi_D3D_Init_Result_AdapterInterfaceQueryFailed;
		}
		IDXGIAdapter1_Release(adapter); // Don't need version 1 anymore.
	}

	if (FAILED(D3D12CreateDevice((IUnknown *) abGPUi_D3D_DXGIAdapterMain, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, &abGPUi_D3D_Device))) {
		abGPU_Shutdown();
		return abGPUi_D3D_Init_Result_DeviceCreationFailed;
	}

	{
		D3D12_COMMAND_QUEUE_DESC commandQueueDesc = { 0 };
		commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		if (FAILED(ID3D12Device_CreateCommandQueue(abGPUi_D3D_Device, &commandQueueDesc, &IID_ID3D12CommandQueue,
				&abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Graphics]))) {
			abGPU_Shutdown();
			return abGPUi_D3D_Init_Result_GraphicsCommandQueueCreationFailed;
		}
		commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		if (FAILED(ID3D12Device_CreateCommandQueue(abGPUi_D3D_Device, &commandQueueDesc, &IID_ID3D12CommandQueue,
				&abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Copy]))) {
			abGPU_Shutdown();
			return abGPUi_D3D_Init_Result_CopyCommandQueueCreationFailed;
		}
	}

	return abGPU_Init_Result_Success;
}

const char *abGPU_Init_ResultToString(abGPU_Init_Result result) {
	switch (result) {
	case abGPUi_D3D_Init_Result_DXGIFactoryCreationFailed:
		return "DXGI factory creation failed.";
	case abGPUi_D3D_Init_Result_AdapterNotFound:
		return "No Direct3D feature level 11_0 adapter found.";
	case abGPUi_D3D_Init_Result_AdapterInterfaceQueryFailed:
		return "DXGI adapter interface version 3 query failed.";
	case abGPUi_D3D_Init_Result_DeviceCreationFailed:
		return "Direct3D 12 feature level 11_0 device creation failed.";
	case abGPUi_D3D_Init_Result_GraphicsCommandQueueCreationFailed:
		return "Graphics command queue creation failed.";
	case abGPUi_D3D_Init_Result_CopyCommandQueueCreationFailed:
		return "Copy command queue creation failed.";
	}
	return abNull;
}

void abGPU_Shutdown() {
	// May be called from Init, so needs to do null checks.
	unsigned int commandQueueIndex;
	for (commandQueueIndex = 0; commandQueueIndex < (unsigned) abGPU_CmdQueue_Count; ++commandQueueIndex) {
		ID3D12CommandQueue **queue = &abGPUi_D3D_CommandQueues[commandQueueIndex];
		if (*queue != abNull) {
			ID3D12CommandQueue_Release(*queue);
			*queue = abNull;
		}
	}
	if (abGPUi_D3D_Device != abNull) {
		ID3D12Device_Release(abGPUi_D3D_Device);
		abGPUi_D3D_Device = abNull;
	}
	if (abGPUi_D3D_DXGIAdapterMain != abNull) {
		IDXGIAdapter3_Release(abGPUi_D3D_DXGIAdapterMain);
		abGPUi_D3D_DXGIAdapterMain = abNull;
	}
	if (abGPUi_D3D_DXGIFactory != abNull) {
		IDXGIFactory2_Release(abGPUi_D3D_DXGIFactory);
		abGPUi_D3D_DXGIFactory = abNull;
	}
}

#endif
