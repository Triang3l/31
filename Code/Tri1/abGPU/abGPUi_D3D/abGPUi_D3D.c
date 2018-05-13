#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"
#include <dxgidebug.h>

abMemory_Tag * abGPUi_D3D_MemoryTag = abNull;

abBool abGPUi_D3D_DebugEnabled;

IDXGIFactory2 * abGPUi_D3D_DXGIFactory = abNull;
IDXGIAdapter3 * abGPUi_D3D_DXGIAdapterMain = abNull;
ID3D12Device * abGPUi_D3D_Device = abNull;
ID3D12CommandQueue * abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Count] = { 0 };

void abGPUi_D3D_SetObjectName(void * object, abGPUi_D3D_ObjectNameSetter setter, abTextU8 const * name) {
	if (name == abNull || name[0u] == '\0') {
		return;
	}
	size_t nameU16Size = abTextU8_LengthInU16(name) + 1u;
	abTextU16 * nameU16 = abStackAlloc(nameU16Size * sizeof(abTextU16));
	abTextU16_FromU8(nameU16, nameU16Size, name);
	setter(object, (WCHAR const *) nameU16);
}

enum {
	abGPUi_D3D_Init_Result_DXGIFactoryCreationFailed = 1u,
	abGPUi_D3D_Init_Result_AdapterNotFound,
	abGPUi_D3D_Init_Result_AdapterInterfaceQueryFailed,
	abGPUi_D3D_Init_Result_DeviceCreationFailed,
	abGPUi_D3D_Init_Result_GraphicsCommandQueueCreationFailed,
	abGPUi_D3D_Init_Result_CopyCommandQueueCreationFailed
};

abGPU_Init_Result abGPU_Init(abBool debug) {
	if (abGPUi_D3D_Device != abNull) {
		return abGPU_Init_Result_Success;
	}

	abGPUi_D3D_MemoryTag = abMemory_Tag_Create("GPUi_D3D");

	if (debug) {
		ID3D12Debug * debugInterface;
		if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug, &debugInterface))) {
			ID3D12Debug_EnableDebugLayer(debugInterface);
			ID3D12Debug_Release(debugInterface);
		} else {
			debug = abFalse;
		}
	}
	abGPUi_D3D_DebugEnabled = debug;

	if (FAILED(CreateDXGIFactory2(debug ? DXGI_CREATE_FACTORY_DEBUG : 0u, &IID_IDXGIFactory2, &abGPUi_D3D_DXGIFactory))) {
		return abGPUi_D3D_Init_Result_DXGIFactoryCreationFailed;
	}
	IDXGIFactory2_SetPrivateData(abGPUi_D3D_DXGIFactory, &WKPDID_D3DDebugObjectName,
			sizeof("abGPUi_D3D_DXGIFactory") - 1u, "abGPUi_D3D_DXGIFactory");

	IDXGIAdapter1 * adapter = abNull, * softwareAdapter = abNull;
	unsigned int adapterIndex = 0u;
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
	IDXGIAdapter3_SetPrivateData(abGPUi_D3D_DXGIAdapterMain, &WKPDID_D3DDebugObjectName,
			sizeof("abGPUi_D3D_DXGIAdapterMain") - 1u, "abGPUi_D3D_DXGIAdapterMain");

	if (FAILED(D3D12CreateDevice((IUnknown *) abGPUi_D3D_DXGIAdapterMain, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, &abGPUi_D3D_Device))) {
		abGPU_Shutdown();
		return abGPUi_D3D_Init_Result_DeviceCreationFailed;
	}
	ID3D12Device_SetName(abGPUi_D3D_Device, L"abGPUi_D3D_Device");

	D3D12_COMMAND_QUEUE_DESC queueDesc = { 0 };
	ID3D12CommandQueue * * queuePointer = &abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Graphics];
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (FAILED(ID3D12Device_CreateCommandQueue(abGPUi_D3D_Device, &queueDesc, &IID_ID3D12CommandQueue, queuePointer))) {
		abGPU_Shutdown();
		return abGPUi_D3D_Init_Result_GraphicsCommandQueueCreationFailed;
	}
	ID3D12CommandQueue_SetName(*queuePointer, L"abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Graphics]");
	queuePointer = &abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Copy];
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	if (FAILED(ID3D12Device_CreateCommandQueue(abGPUi_D3D_Device, &queueDesc, &IID_ID3D12CommandQueue, queuePointer))) {
		abGPU_Shutdown();
		return abGPUi_D3D_Init_Result_CopyCommandQueueCreationFailed;
	}
	ID3D12CommandQueue_SetName(*queuePointer, L"abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Copy]");

	abGPUi_D3D_HandleStore_DescriptorSize = ID3D12Device_GetDescriptorHandleIncrementSize(abGPUi_D3D_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	abGPUi_D3D_RTStore_DescriptorSizeColor = ID3D12Device_GetDescriptorHandleIncrementSize(abGPUi_D3D_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	abGPUi_D3D_RTStore_DescriptorSizeDepth = ID3D12Device_GetDescriptorHandleIncrementSize(abGPUi_D3D_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	abGPUi_D3D_SamplerStore_DescriptorSize = ID3D12Device_GetDescriptorHandleIncrementSize(abGPUi_D3D_Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	return abGPU_Init_Result_Success;
}

char const * abGPU_Init_ResultText(abGPU_Init_Result result) {
	switch (result) {
	case abGPUi_D3D_Init_Result_DXGIFactoryCreationFailed: return "DXGI factory creation failed.";
	case abGPUi_D3D_Init_Result_AdapterNotFound: return "No Direct3D feature level 11_0 adapter found.";
	case abGPUi_D3D_Init_Result_AdapterInterfaceQueryFailed: return "DXGI adapter interface version 3 query failed.";
	case abGPUi_D3D_Init_Result_DeviceCreationFailed: return "Direct3D 12 feature level 11_0 device creation failed.";
	case abGPUi_D3D_Init_Result_GraphicsCommandQueueCreationFailed: return "Graphics command queue creation failed.";
	case abGPUi_D3D_Init_Result_CopyCommandQueueCreationFailed: return "Copy command queue creation failed.";
	}
	return abNull;
}

void abGPU_Shutdown() {
	// May be called from Init, so needs to do null checks.
	for (unsigned int commandQueueIndex = 0u; commandQueueIndex < (unsigned int) abGPU_CmdQueue_Count; ++commandQueueIndex) {
		ID3D12CommandQueue * * queue = &abGPUi_D3D_CommandQueues[commandQueueIndex];
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
	if (abGPUi_D3D_MemoryTag != abNull) {
		abMemory_Tag_Destroy(abGPUi_D3D_MemoryTag);
		abGPUi_D3D_MemoryTag = abNull;
	}
	if (abGPUi_D3D_DebugEnabled) {
		HMODULE debugLibrary = LoadLibraryA("DXGIDebug.dll");
		if (debugLibrary != abNull) {
			IDXGIDebug * dxgiDebug;
			HRESULT (WINAPI * getDebugInterface)(REFIID riid, void * * debug) =
					(void *) GetProcAddress(debugLibrary, "DXGIGetDebugInterface");
			if (getDebugInterface != abNull) {
				if (SUCCEEDED(getDebugInterface(&IID_IDXGIDebug, &dxgiDebug))) {
					IDXGIDebug_ReportLiveObjects(dxgiDebug, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
					IDXGIDebug_Release(dxgiDebug);
				}
			}
			FreeLibrary(debugLibrary);
		}
	}
}

#endif
