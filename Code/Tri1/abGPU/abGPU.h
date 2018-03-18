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
#define abGPU_Init_Result_Success 0u
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
	abGPU_Image_Type_Regular = 0u,
	abGPU_Image_Type_Upload = 1u, // Only accessible from the CPU - other type bits are ignored.
	abGPU_Image_Type_Renderable = abGPU_Image_Type_Upload << 1u, // Can be a render target.
	abGPU_Image_Type_Editable = abGPU_Image_Type_Renderable << 1u // Can be edited by shaders.
};

typedef enum abGPU_Image_Dimensions {
	abGPU_Image_Dimensions_2D,
	abGPU_Image_Dimensions_2DArray,
	abGPU_Image_Dimensions_Cube,
	abGPU_Image_Dimensions_CubeArray,
	abGPU_Image_Dimensions_3D
} abGPU_Image_Dimensions;
abForceInline bool abGPU_Image_DimensionsAre2D(abGPU_Image_Dimensions dimensions) {
	return dimensions == abGPU_Image_Dimensions_2D || dimensions == abGPU_Image_Dimensions_2DArray;
}
abForceInline bool abGPU_Image_DimensionsAreCube(abGPU_Image_Dimensions dimensions) {
	return dimensions == abGPU_Image_Dimensions_Cube || dimensions == abGPU_Image_Dimensions_CubeArray;
}
abForceInline bool abGPU_Image_DimensionsAre3D(abGPU_Image_Dimensions dimensions) {
	return dimensions == abGPU_Image_Dimensions_3D;
}
abForceInline bool abGPU_Image_DimensionsAreArray(abGPU_Image_Dimensions dimensions) {
	return dimensions == abGPU_Image_Dimensions_2DArray || dimensions == abGPU_Image_Dimensions_CubeArray;
}

typedef enum abGPU_Image_Format {
		abGPU_Image_Format_RawLDRStart,
	abGPU_Image_Format_R8G8B8A8 = abGPU_Image_Format_RawLDRStart,
	abGPU_Image_Format_R8G8B8A8_sRGB,
	abGPU_Image_Format_R8G8B8A8_Signed,
		abGPU_Image_Format_RawLDREnd = abGPU_Image_Format_R8G8B8A8_Signed,

		abGPU_Image_Format_4x4Start,
	abGPU_Image_Format_S3TC_A1 = abGPU_Image_Format_4x4Start,
	abGPU_Image_Format_S3TC_A1_sRGB,
	abGPU_Image_Format_S3TC_A4,
	abGPU_Image_Format_S3TC_A4_sRGB,
	abGPU_Image_Format_S3TC_A8,
	abGPU_Image_Format_S3TC_A8_sRGB,
	abGPU_Image_Format_3Dc_X,
	abGPU_Image_Format_3Dc_X_Signed,
	abGPU_Image_Format_3Dc_XY,
	abGPU_Image_Format_3Dc_XY_Signed,
		abGPU_Image_Format_4x4End = abGPU_Image_Format_3Dc_XY_Signed,

		abGPU_Image_Format_DepthStart,
	abGPU_Image_Format_D32 = abGPU_Image_Format_DepthStart,
		abGPU_Image_Format_DepthStencilStart,
	abGPU_Image_Format_D24S8 = abGPU_Image_Format_DepthStencilStart,
		abGPU_Image_Format_DepthStencilEnd = abGPU_Image_Format_D24S8,
		abGPU_Image_Format_DepthEnd = abGPU_Image_Format_DepthStencilEnd
} abGPU_Image_Format;
abForceInline bool abGPU_Image_Format_Is4x4(abGPU_Image_Format format) {
	return format >= abGPU_Image_Format_4x4Start && format <= abGPU_Image_Format_4x4End;
}
abForceInline bool abGPU_Image_Format_IsDepth(abGPU_Image_Format format) {
	return format >= abGPU_Image_Format_DepthStart && format <= abGPU_Image_Format_DepthEnd;
}
abForceInline bool abGPU_Image_Format_IsDepthStencil(abGPU_Image_Format format) {
	return format >= abGPU_Image_Format_DepthStencilStart && format <= abGPU_Image_Format_DepthStencilEnd;
}
unsigned int abGPU_Image_Format_GetSize(abGPU_Image_Format format); // Block size for compressed formats.

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
	// Layouts - stencil plane not counted (but this has no use for depth/stencil images anyway).
	DXGI_FORMAT copyFormat;
	unsigned int mipOffset[D3D12_REQ_MIP_LEVELS];
	unsigned int mipRowStride[D3D12_REQ_MIP_LEVELS];
	unsigned int layerStride; // 0 for non-cubemaps and non-arrays.
	#endif
} abGPU_Image_Internal;

#define abGPU_Image_DimensionsShift 24u
typedef struct abGPU_Image {
	unsigned int typeAndDimensions; // Below DimensionsShift - type flags, starting from it - dimensions.
	// Depth is 1 for 2D and cubemaps, Z depth for 3D, and number of layers for arrays.
	unsigned int w, h, d;
	unsigned int mips;
	abGPU_Image_Format format;
	unsigned int memoryUsage;
	abGPU_Image_Internal i;
} abGPU_Image;

abForceInline abGPU_Image_Type abGPU_Image_GetType(const abGPU_Image *image) {
	return (abGPU_Image_Type) (image->typeAndDimensions & ((1u << abGPU_Image_DimensionsShift) - 1u));
}

abForceInline abGPU_Image_Dimensions abGPU_Image_GetDimensions(const abGPU_Image *image) {
	return (abGPU_Image_Dimensions) (image->typeAndDimensions >> abGPU_Image_DimensionsShift);
}

abForceInline void abGPU_Image_GetMipSize(const abGPU_Image *image, unsigned int mip,
		unsigned int *w, unsigned int *h, unsigned int *d) {
	if (w != abNull) *w = abMax(image->w >> mip, 1u);
	if (h != abNull) *h = abMax(image->h >> mip, 1u);
	if (d != abNull) *d = (abGPU_Image_DimensionsAre3D(abGPU_Image_GetDimensions(image)) ? abMax(image->d >> mip, 1u) : image->d);
}

typedef unsigned int abGPU_Image_Slice;
#define abGPU_Image_SliceMake(layer, side, mip) ((abGPU_Image_Slice) (((layer) << 8u) | ((side) << 5u) | (mip)))
#define abGPU_Image_SliceMip(slice) ((unsigned int) ((slice) & 31u))
#define abGPU_Image_SliceSide(slice) ((unsigned int) (((slice) >> 5u) & 7u))
#define abGPU_Image_SliceLayer(slice) ((unsigned int) ((slice) >> 8u))
inline bool abGPUi_Image_HasSlice(const abGPU_Image *image, abGPU_Image_Slice slice) {
	abGPU_Image_Dimensions dimensions;
	if (abGPU_Image_SliceMip(slice) >= image->mips) {
		return false;
	}
	if (abGPU_Image_SliceSide(slice) > (abGPU_Image_DimensionsAreCube(dimensions) ? 5u : 0u)) {
		return false;
	}
	if (abGPU_Image_SliceLayer(slice) >= (abGPU_Image_DimensionsAreArray(dimensions) ? image->d : 1u)) {
		return false;
	}
	return true;
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
	if (abGPU_Image_DimensionsAre3D(dimensions)) {
		size = abMax(size, d);
	}
	return abBit_HighestOne32(size + (size == 0u)) + 1u;
}

// Clamps to [1, max].
void abGPU_Image_ClampSizeToMax(abGPU_Image_Dimensions dimensions,
		unsigned int *w, unsigned int *h, unsigned int *d, /* optional */ unsigned int *mips);

/*
 * Implementation functions.
 */
void abGPU_Image_GetMaxSize(abGPU_Image_Dimensions dimensions,
		/* optional */ unsigned int *wh, /* optional */ unsigned int *d);
unsigned int abGPU_Image_CalculateMemoryUsage(abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format);
// Usage is ignored for upload buffers. Clear value is null for images that are not render targets.
bool abGPU_Image_Init(abGPU_Image *image, abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format,
		abGPU_Image_Usage initialUsage, const abGPU_Image_Texel *clearValue);
bool abGPU_Image_RespecifyUploadBuffer(abGPU_Image *image, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format);
// Provides the mapping of the memory for the upload functions, may return null even in case of success.
void *abGPU_Image_UploadBegin(abGPU_Image *image, abGPU_Image_Slice slice);
// Strides must be equal to or greater than real sizes and multiples of texel/block size.
// For 4x4 compressed images, one row is 4 texels tall. 0 strides can be provided for tight packing.
// Mapping can be null, in this case, UploadBegin and UploadEnd will be called implicitly.
void abGPU_Image_Upload(abGPU_Image *image, abGPU_Image_Slice slice,
		unsigned int x, unsigned int y, unsigned int z, unsigned int w, unsigned int h, unsigned int d,
		unsigned int yStride, unsigned int zStride, void *mapping, const void *data);
// Written range can be null, in this case, it is assumed that the whole sub-image was modified.
void abGPU_Image_UploadEnd(abGPU_Image *image, abGPU_Image_Slice slice,
		void *mapping, const unsigned int writtenOffsetAndSize[2]);
void abGPU_Image_Destroy(abGPU_Image *image);

#endif
