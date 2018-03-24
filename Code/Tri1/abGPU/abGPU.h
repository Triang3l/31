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
char const * abGPU_Init_ResultToString(abGPU_Init_Result result);
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
	abGPU_CmdQueue queue;

	#if defined(abBuild_GPUi_D3D)
	ID3D12Fence * i_fence;
	HANDLE i_completionEvent;
	uint64_t i_awaitedValue;
	#endif
} abGPU_Fence;

bool abGPU_Fence_Init(abGPU_Fence * fence, abGPU_CmdQueue queue);
void abGPU_Fence_Enqueue(abGPU_Fence * fence);
bool abGPU_Fence_IsCrossed(abGPU_Fence * fence);
void abGPU_Fence_Await(abGPU_Fence * fence);
void abGPU_Fence_Destroy(abGPU_Fence * fence);

/**********
 * Buffers
 **********/

typedef enum abGPU_Buffer_Access {
	abGPU_Buffer_Access_GPUInternal,
	abGPU_Buffer_Access_CPUWritable,
	abGPU_Buffer_Access_Upload
} abGPU_Buffer_Access;

typedef struct abGPU_Buffer {
	abGPU_Buffer_Access access;
	unsigned int size;

	#if defined(abBuild_GPUi_D3D)
	ID3D12Resource * i_resource;
	D3D12_GPU_VIRTUAL_ADDRESS i_gpuVirtualAddress;
	#endif
} abGPU_Buffer;

#define abGPU_Buffer_ConstantAlignment 256u

typedef enum abGPU_Buffer_Usage {
	abGPU_Buffer_Usage_Vertices,
	abGPU_Buffer_Usage_Constants,
	abGPU_Buffer_Usage_Indices,
	abGPU_Buffer_Usage_Structures, // Structured data readable in pixel shaders.
	abGPU_Buffer_Usage_StructuresNonPixelStage,  // Structured data readable in non-pixel shaders.
	abGPU_Buffer_Usage_StructuresAnyStage, // Structured data readable at all shader stages.
	abGPU_Buffer_Usage_Edit, // Directly editable in shaders.
	abGPU_Buffer_Usage_CopySource,
	abGPU_Buffer_Usage_CopyDestination,
	abGPU_Buffer_Usage_CopyQueue, // Owned by the copy command queue (which doesn't have the concept of usages).
	abGPU_Buffer_Usage_CPUWrite, // CPU-writable or upload buffer.
		abGPU_Buffer_Usage_Count
} abGPU_Buffer_Usage;

bool abGPU_Buffer_Init(abGPU_Buffer * buffer, abGPU_Buffer_Access access,
		unsigned int size, bool editable, abGPU_Buffer_Usage initialUsage);
void * abGPU_Buffer_Map(abGPU_Buffer * buffer);
// Written range can be null, in this case, it is assumed that the whole buffer was modified.
void abGPU_Buffer_Unmap(abGPU_Buffer * buffer, void * mapping, unsigned int const writtenOffsetAndSize[2]);
void abGPU_Buffer_Destroy(abGPU_Buffer * buffer);

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
		abGPU_Image_Format_DepthEnd = abGPU_Image_Format_DepthStencilEnd,

		abGPU_Image_Format_Count
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

#define abGPU_Image_DimensionsShift 24u
typedef struct abGPU_Image {
	unsigned int typeAndDimensions; // Below DimensionsShift - type flags, starting from it - dimensions.
	// Depth is 1 for 2D and cubemaps, Z depth for 3D, and number of layers for arrays.
	unsigned int w, h, d;
	unsigned int mips;
	abGPU_Image_Format format;
	unsigned int memoryUsage;

	#if defined(abBuild_GPUi_D3D)
	ID3D12Resource * i_resource;
	// Copyable footprints - stencil plane not counted (but this has no use for depth/stencil images anyway).
	DXGI_FORMAT i_copyFormat;
	unsigned int i_mipOffset[D3D12_REQ_MIP_LEVELS];
	unsigned int i_mipRowStride[D3D12_REQ_MIP_LEVELS];
	unsigned int i_layerStride; // 0 for non-cubemaps and non-arrays.
	#endif
} abGPU_Image;

abForceInline abGPU_Image_Type abGPU_Image_GetType(abGPU_Image const * image) {
	return (abGPU_Image_Type) (image->typeAndDimensions & ((1u << abGPU_Image_DimensionsShift) - 1u));
}

abForceInline abGPU_Image_Dimensions abGPU_Image_GetDimensions(abGPU_Image const * image) {
	return (abGPU_Image_Dimensions) (image->typeAndDimensions >> abGPU_Image_DimensionsShift);
}

abForceInline void abGPU_Image_GetMipSize(abGPU_Image const * image, unsigned int mip,
		unsigned int * w, unsigned int * h, unsigned int * d) {
	if (w != abNull) *w = abMax(image->w >> mip, 1u);
	if (h != abNull) *h = abMax(image->h >> mip, 1u);
	if (d != abNull) *d = (abGPU_Image_DimensionsAre3D(abGPU_Image_GetDimensions(image)) ? abMax(image->d >> mip, 1u) : image->d);
}

typedef unsigned int abGPU_Image_Slice;
#define abGPU_Image_SliceMake(layer, side, mip) ((abGPU_Image_Slice) (((layer) << 8u) | ((side) << 5u) | (mip)))
#define abGPU_Image_SliceMip(slice) ((unsigned int) ((slice) & 31u))
#define abGPU_Image_SliceSide(slice) ((unsigned int) (((slice) >> 5u) & 7u))
#define abGPU_Image_SliceLayer(slice) ((unsigned int) ((slice) >> 8u))
inline bool abGPUi_Image_HasSlice(abGPU_Image const * image, abGPU_Image_Slice slice) {
	if (abGPU_Image_SliceMip(slice) >= image->mips) {
		return false;
	}
	abGPU_Image_Dimensions dimensions = abGPU_Image_GetDimensions(image);
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
	abGPU_Image_Usage_CopyQueue, // Owned by the copy command queue (which doesn't have the concept of usages).
	abGPU_Image_Usage_Upload, // Upload buffer (can't really switch to or from this usage).
		abGPU_Image_Usage_Count
} abGPU_Image_Usage;

typedef enum abGPU_Image_Comparison {
	// Basically D3D comparison function minus one.
	abGPU_Image_Comparison_Less = 1u << 0u,
	abGPU_Image_Comparison_Equal = 1u << 1u,
	abGPU_Image_Comparison_Greater = 1u << 2u,
	abGPU_Image_Comparison_Never = 0,
	abGPU_Image_Comparison_LessEqual = abGPU_Image_Comparison_Less | abGPU_Image_Comparison_Equal,
	abGPU_Image_Comparison_NotEqual = abGPU_Image_Comparison_Less | abGPU_Image_Comparison_Greater,
	abGPU_Image_Comparison_GreaterEqual = abGPU_Image_Comparison_Greater | abGPU_Image_Comparison_Equal,
	abGPU_Image_Comparison_Always = abGPU_Image_Comparison_Equal | abGPU_Image_Comparison_NotEqual
} abGPU_Image_Comparison;

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
		unsigned int * w, unsigned int * h, unsigned int * d, /* optional */ unsigned int * mips);

/*
 * Implementation functions.
 */
void abGPU_Image_GetMaxSize(abGPU_Image_Dimensions dimensions,
		/* optional */ unsigned int * wh, /* optional */ unsigned int * d);
unsigned int abGPU_Image_CalculateMemoryUsage(abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format);
// Usage must be Upload for upload buffers. Clear value is null for images that are not render targets.
bool abGPU_Image_Init(abGPU_Image * image, abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format,
		abGPU_Image_Usage initialUsage, abGPU_Image_Texel const * clearValue);
bool abGPU_Image_RespecifyUploadBuffer(abGPU_Image * image, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format);
// Provides the mapping of the memory for the upload functions, may return null even in case of success.
void * abGPU_Image_UploadBegin(abGPU_Image * image, abGPU_Image_Slice slice);
// Strides must be equal to or greater than real sizes and multiples of texel/block size.
// For 4x4 compressed images, one row is 4 texels tall. 0 strides can be provided for tight packing.
// Mapping can be null, in this case, UploadBegin and UploadEnd will be called implicitly.
void abGPU_Image_Upload(abGPU_Image * image, abGPU_Image_Slice slice,
		unsigned int x, unsigned int y, unsigned int z, unsigned int w, unsigned int h, unsigned int d,
		unsigned int yStride, unsigned int zStride, void * mapping, void const * data);
// Written range can be null, in this case, it is assumed that the whole sub-image was modified.
void abGPU_Image_UploadEnd(abGPU_Image * image, abGPU_Image_Slice slice,
		void * mapping, unsigned int const writtenOffsetAndSize[2]);
void abGPU_Image_Destroy(abGPU_Image * image);

/*********************************
 * Buffer and image handle stores
 *********************************/

typedef struct abGPU_HandleStore {
	unsigned int handleCount;

	#if defined(abBuild_GPUi_D3D)
	ID3D12DescriptorHeap * i_descriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE i_cpuDescriptorHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE i_gpuDescriptorHandleStart;
	#endif
} abGPU_HandleStore;

bool abGPU_HandleStore_Init(abGPU_HandleStore * store, unsigned int handleCount);
void abGPU_HandleStore_SetConstantBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer const * buffer, unsigned int offset, unsigned int size);
void abGPU_HandleStore_Destroy(abGPU_HandleStore * store);

/****************
 * Shader stages
 ****************/

typedef enum abGPU_ShaderStage {
	abGPU_ShaderStage_Vertex,
	abGPU_ShaderStage_Pixel,
	abGPU_ShaderStage_Compute
} abGPU_ShaderStage;

// abGPU_Input depends on this being 8 bits or less!
typedef enum abGPU_ShaderStageBits {
	abGPU_ShaderStageBits_Vertex = 1u << abGPU_ShaderStage_Vertex,
	abGPU_ShaderStageBits_Pixel = 1u << abGPU_ShaderStage_Pixel,
	abGPU_ShaderStageBits_Compute = 1u << abGPU_ShaderStage_Compute
} abGPU_ShaderStageBits;

/***********
 * Samplers
 ***********/

typedef uint16_t abGPU_Sampler;
enum {
	// Filtering is either non-anisotropic or anisotropic - the same 3 bits are used for anisotropic power-1 and linear/nearest!

	abGPU_Sampler_FilterAniso = 1u << 0u,

	abGPU_Sampler_FilterLinearMag = 1u << 1u,
	abGPU_Sampler_FilterLinearMin = 1u << 2u,
	abGPU_Sampler_FilterLinearMip = 1u << 3u,
	abGPU_Sampler_FilterLinear = abGPU_Sampler_FilterLinearMag | abGPU_Sampler_FilterLinearMin | abGPU_Sampler_FilterLinearMip,

	abGPU_Sampler_FilterAnisoPowerM1Shift = 1u,
	abGPU_Sampler_FilterAnisoPowerM1Mask = 3u, // After right shift.
	abGPU_Sampler_FilterAniso2X = abGPU_Sampler_FilterAniso,
	abGPU_Sampler_FilterAniso4X = abGPU_Sampler_FilterAniso | (1u << abGPU_Sampler_FilterAnisoPowerM1Shift),
	abGPU_Sampler_FilterAniso8X = abGPU_Sampler_FilterAniso | (2u << abGPU_Sampler_FilterAnisoPowerM1Shift),
	abGPU_Sampler_FilterAniso16X = abGPU_Sampler_FilterAniso | (3u << abGPU_Sampler_FilterAnisoPowerM1Shift),

	// Mipmap index can be clamped (for alphatested things) to up to 14 (to 1x1 at 16384x16384).
	abGPU_Sampler_MipCountShift = 4u,
	abGPU_Sampler_MipCountMask = 15u,
	abGPU_Sampler_MipAll = abGPU_Sampler_MipCountMask << abGPU_Sampler_MipCountShift, // Use all mipmaps.

	abGPU_Sampler_RepeatS = 1u << 8u,
	abGPU_Sampler_RepeatT = 1u << 9u,
	abGPU_Sampler_RepeatR = 1u << 10u,
	abGPU_Sampler_RepeatST = abGPU_Sampler_RepeatS | abGPU_Sampler_RepeatT,
	abGPU_Sampler_Repeat = abGPU_Sampler_RepeatST | abGPU_Sampler_RepeatR,

	abGPU_Sampler_CompareFailShift = 11u,
	abGPU_Sampler_CompareFailMask = 7u, // After right shift. 3 bits are enough (0 is always), who needs always or never here anyway.
};
#define abGPU_Sampler_CompareFail(comparison) ((comparison) << abGPU_Sampler_CompareFailShift)
#define abGPU_Sampler_ComparePass(comparison) abGPU_Sampler_CompareFail((comparison) ^ abGPU_Sampler_CompareFailMask)

typedef struct abGPU_SamplerStore {
	unsigned int samplerCount;

	#if defined(abBuild_GPUi_D3D)
	ID3D12DescriptorHeap * i_descriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE i_cpuDescriptorHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE i_gpuDescriptorHandleStart;
	#endif
} abGPU_SamplerStore;

bool abGPU_SamplerStore_Init(abGPU_SamplerStore * store, unsigned int samplerCount);
void abGPU_SamplerStore_SetSampler(abGPU_SamplerStore * store, unsigned int samplerIndex, abGPU_Sampler sampler);
void abGPU_SamplerStore_Destroy(abGPU_SamplerStore * store);

/****************
 * Shader inputs
 ****************/

// Strategy for binding of small amounts of constant data (below 4 KB, generally less than 256 bytes).
typedef enum abGPU_Input_UniformStrategy {
	abGPU_Input_UniformStrategy_RawData, // Send the data directly through the command list.
	abGPU_Input_UniformStrategy_BufferAndOffset // Bind a buffer directly, without a handle.
} abGPU_Input_UniformStrategy;

// abGPU_Input_StructureBuffersAreImages is 1 if structure buffers use t# indices, 0 if they use b#.
// abGPU_Input_SeparateEditIndices is 1 if editable buffers and images use u# indices, 0 if they use b# and t#.
#if defined(abBuild_GPUi_D3D)
#define abGPU_Input_PreferredUniformStrategy abGPU_Input_UniformStrategy_BufferAndOffset
#define abGPU_Input_StructureBuffersAreImages 1
#define abGPU_Input_SeparateEditIndices 1
#else
#error No shader input preferences defined for the target GPU API.
#endif

typedef enum abGPU_Input_Type {
		abGPU_Input_Type_Skip,
	abGPU_Input_Type_Constant, // Small data passed directly through command list.
	abGPU_Input_Type_ConstantBuffer, // Buffer and offset bound directly.
	abGPU_Input_Type_ConstantBufferHandle, // Buffer bound via a handle.
	abGPU_Input_Type_Uniform, // One of the constant input types, depending on the strategy.
	abGPU_Input_Type_StructureBufferHandle,
	abGPU_Input_Type_EditBufferHandle,
	abGPU_Input_Type_ImageHandle,
	abGPU_Input_Type_EditImageHandle,
	abGPU_Input_Type_SamplerHandle
} abGPU_Input_Type;

// These must be immutable as long as they are attached to an active input list.
#define abGPU_Input_SamplerDynamicOnly UINT8_MAX
typedef struct abGPU_Input {
	uint8_t stages;
	uint8_t type;
	union {
		struct { uint8_t constantIndex, count32Bits; } constant;
		struct { uint8_t constantIndex; } constantBuffer;
		struct { uint8_t bufferFirstIndex, count; } constantBufferHandle;
		struct { uint8_t constantIndex; } uniform;
		struct { uint8_t bufferFirstIndex, imageFirstIndex, count; } structureBufferHandle;
		struct { uint8_t bufferFirstIndex, editFirstIndex, count; } editBufferHandle;
		struct { uint8_t imageFirstIndex, count; } imageHandle;
		struct { uint8_t imageFirstIndex, editFirstIndex, count; } editImageHandle;
		// Static samplers are an optimization that may be unsupported, not a full replacement for binding.
		// staticIndex is an index in the array of static samplers that is passed when the input list is created.
		// If static samplers are supported and provided, they will override bindings for non-SamplerDynamicOnly inputs.
		// However, as they may be unsupported by implementations, samplers still must be bound.
		struct { uint8_t samplerFirstIndex, staticIndex, count; } samplerHandle;
	} parameters;
} abGPU_Input;

#endif
