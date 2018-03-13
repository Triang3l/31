#ifndef abInclude_abGPU
#define abInclude_abGPU

#include "../abCommon.h"

#if defined(abBuild_GPUi_D3D)
#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <dxgi1_4.h>
#include <d3d12.h>
#else
#error No GPU abstraction layer backend (abConfig_GPUi_) selected while configuring the build.
#endif

/*
 * GPU abstraction layer.
 */

typedef unsigned int abGPU_Init_Result;
#define abGPU_Init_Result_Success 0
abGPU_Init_Result abGPU_Init(bool debug);
// Returns an immutable string. May return null for success or unknown errors.
const char *abGPU_Init_ResultToString(abGPU_Init_Result result);
void abGPU_Shutdown();

typedef enum abGPU_CmdQueue {
	abGPU_CmdQueue_Graphics,
	abGPU_CmdQueue_Copy,
		abGPU_CmdQueue_Count
} abGPU_CmdQueue;

// These are for use by implementations only!
#if defined(abBuild_GPUi_D3D)
extern IDXGIFactory2 *abGPUi_D3D_DXGIFactory;
extern IDXGIAdapter3 *abGPUi_D3D_DXGIAdapterMain;
extern ID3D12Device *abGPUi_D3D_Device;
extern ID3D12CommandQueue *abGPUi_D3D_CommandQueues[abGPU_CmdQueue_Count];
#endif

/*********
 * Fences
 *********/

typedef struct abGPU_Fence {
	#if defined(abBuild_GPUi_D3D)
	abGPU_CmdQueue queue;
	ID3D12Fence *fence;
	HANDLE completionEvent;
	uint64_t awaitedValue;
	#endif
} abGPU_Fence;

bool abGPU_Fence_Init(abGPU_Fence *fence, abGPU_CmdQueue queue);
void abGPU_Fence_Destroy(abGPU_Fence *fence);
void abGPU_Fence_Enqueue(abGPU_Fence *fence);
bool abGPU_Fence_IsCrossed(abGPU_Fence *fence);
void abGPU_Fence_Await(abGPU_Fence *fence);

/*********
 * Images
 *********/

typedef enum abGPU_Image_Dimensions {
	abGPU_Image_Dimensions_2D,
	abGPU_Image_Dimensions_2DArray,
	abGPU_Image_Dimensions_Cube,
	abGPU_Image_Dimensions_CubeArray,
	abGPU_Image_Dimensions_3D
} abGPU_Image_Dimensions;

typedef struct abGPU_Image_Size {
	// Depth is 1 for 2D and cubemaps, Z depth for 3D, and number of layers for arrays.
	unsigned int w, h, d;
} abGPU_Image_Size;

typedef enum abGPU_Image_Format {
		abGPU_Image_Format_RawLDRStart,
	abGPU_Image_Format_R8G8B8A8 = abGPU_Image_Format_RawLDRStart,
		abGPU_Image_Format_RawLDREnd = abGPU_Image_Format_R8G8B8A8
} abGPU_Image_Format;

typedef struct abGPU_Image_Private {
	#if defined(abBuild_GPUi_D3D)
	ID3D12Resource *resource;
	#endif
} abGPU_Image_Private;

typedef struct abGPU_Image {
	abGPU_Image_Dimensions dimensions;
	abGPU_Image_Size size;
	abGPU_Image_Format format;
	abGPU_Image_Private p;
} abGPU_Image;

#endif
