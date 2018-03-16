#ifndef abInclude_abGPU
#define abInclude_abGPU

#include "../abMath/abBit.h"

#if defined(abBuild_GPUi_D3D)
#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <dxgi1_4.h>
#include <d3d12.h>
#else
#error No GPU abstraction layer backend (abBuild_GPUi_) selected while configuring the build.
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

typedef unsigned int abGPU_Image_Type;
enum {
	abGPU_Image_Type_Regular = 0,
	abGPU_Image_Type_Upload = 1, // Only accessible from the CPU - other type bits are ignored.
	abGPU_Image_Type_Renderable = abGPU_Image_Type_Upload << 1, // Can be a render target.
	abGPU_Image_Type_Editable = abGPU_Image_Type_Renderable << 1 // Can be edited by shaders.
};

typedef enum abGPU_Image_Dimensions {
	abGPU_Image_Dimensions_2D,
	abGPU_Image_Dimensions_2DArray,
	abGPU_Image_Dimensions_Cube,
	abGPU_Image_Dimensions_CubeArray,
	abGPU_Image_Dimensions_3D
} abGPU_Image_Dimensions;

typedef enum abGPU_Image_Format {
		abGPU_Image_Format_RawLDRStart,
	abGPU_Image_Format_R8G8B8A8 = abGPU_Image_Format_RawLDRStart,
		abGPU_Image_Format_RawLDREnd = abGPU_Image_Format_R8G8B8A8,
		abGPU_Image_Format_DepthStart,
	abGPU_Image_Format_D32 = abGPU_Image_Format_DepthStart,
		abGPU_Image_Format_DepthStencilStart,
	abGPU_Image_Format_D24S8 = abGPU_Image_Format_DepthStencilStart,
		abGPU_Image_Format_DepthStencilEnd = abGPU_Image_Format_D24S8,
		abGPU_Image_Format_DepthEnd = abGPU_Image_Format_DepthStencilEnd
} abGPU_Image_Format;
abForceInline bool abGPU_Image_Format_IsDepth(abGPU_Image_Format format) {
	return format >= abGPU_Image_Format_DepthStart && format <= abGPU_Image_Format_DepthEnd;
}
abForceInline bool abGPU_Image_Format_IsDepthStencil(abGPU_Image_Format format) {
	return format >= abGPU_Image_Format_DepthStencilStart && format <= abGPU_Image_Format_DepthStencilEnd;
}

typedef union abGPU_Image_Texel {
	float color[4];
	struct {
		float depth;
		uint8_t stencil;
	} ds;
} abGPU_Image_Texel;

typedef struct abGPU_Image_Private {
	#if defined(abBuild_GPUi_D3D)
	ID3D12Resource *resource;
	DXGI_FORMAT dxgiFormat; // Redundant, but used often.
	// Layouts - stencil plane not counted (but this has no use for depth/stencil images anyway).
	unsigned int mipOffset[D3D12_REQ_MIP_LEVELS];
	unsigned int mipRowStride[D3D12_REQ_MIP_LEVELS];
	unsigned int layerStride; // 0 for non-cubemaps and non-arrays.
	#endif
} abGPU_Image_Private;

#define abGPU_Image_DimensionsShift 24
typedef struct abGPU_Image {
	unsigned int typeAndDimensions; // Below DimensionsShift - type flags, starting from it - dimensions.
	// Depth is 1 for 2D and cubemaps, Z depth for 3D, and number of layers for arrays.
	unsigned int w, h, d;
	unsigned int mips;
	abGPU_Image_Format format;
	unsigned int memoryUsage;
	abGPU_Image_Private p;
} abGPU_Image;

abForceInline abGPU_Image_Type abGPU_Image_GetType(const abGPU_Image *image) {
	return (abGPU_Image_Type) (image->typeAndDimensions & ((1 << abGPU_Image_DimensionsShift) - 1));
}
abForceInline abGPU_Image_Dimensions abGPU_Image_GetDimensions(const abGPU_Image *image) {
	return (abGPU_Image_Dimensions) (image->typeAndDimensions >> abGPU_Image_DimensionsShift);
}

typedef enum abGPU_Image_Usage {
	abGPU_Image_Usage_Texture, // Sampleable in pixel shaders.
	abGPU_Image_Usage_TextureNonPixelStage, // Sampleable in non-pixel shaders.
	abGPU_Image_Usage_TextureAnyStage, // Sampleable at all shader stages.
	abGPU_Image_Usage_RenderTarget,
	abGPU_Image_Usage_Display, // Display chain image being presented to screen.
	abGPU_Image_Usage_DepthWrite,
	abGPU_Image_Usage_DepthTest,
	abGPU_Image_Usage_DepthTestAndTexture, // For depth testing, but also sampleable in pixel shaders.
	abGPU_Image_Usage_Edit, // Directly editable in shaders.
	abGPU_Image_Usage_CopySource,
	abGPU_Image_Usage_CopyDestination,
	abGPU_Image_Usage_CopyQueue // Owned by the copy command queue (which doesn't have the concept of usages).
} abGPU_Image_Usage;

abForceInline unsigned int abGPU_Image_CalculateMipCount(
		abGPU_Image_Dimensions dimensions, unsigned int w, unsigned int h, unsigned int d) {
	unsigned int size = abMax(w, h);
	if (dimensions == abGPU_Image_Dimensions_3D) {
		size = abMax(size, d);
	}
	return abBit_HighestOne32(size + (size == 0)) + 1;
}

// Implementation functions.
void abGPU_Image_GetMaxSize(abGPU_Image_Dimensions dimensions,
		/* optional */ unsigned int *wh, /* optional */ unsigned int *d);
unsigned int abGPU_Image_CalculateMemoryUsage(abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format);
// Usage is ignored for upload buffers. Clear value is null for images that are not render targets.
bool abGPU_Image_Init(abGPU_Image *image, abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format,
		abGPU_Image_Usage initialUsage, const abGPU_Image_Texel *clearValue);
void abGPU_Image_Destroy(abGPU_Image *image);

#endif
