#ifndef abInclude_abGPU
#define abInclude_abGPU
#include "../abMath/abBit.h"
#include "../abData/abText.h"

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
abGPU_Init_Result abGPU_Init(abBool debug);
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

abBool abGPU_Fence_Init(abGPU_Fence * fence, abTextU8 const * name, abGPU_CmdQueue queue);
void abGPU_Fence_Enqueue(abGPU_Fence * fence);
abBool abGPU_Fence_IsCrossed(abGPU_Fence * fence);
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

abBool abGPU_Buffer_Init(abGPU_Buffer * buffer, abTextU8 const * name, abGPU_Buffer_Access access,
		unsigned int size, abBool editable, abGPU_Buffer_Usage initialUsage);
void * abGPU_Buffer_Map(abGPU_Buffer * buffer);
// Written range can be null, in this case, it is assumed that the whole buffer was modified.
void abGPU_Buffer_Unmap(abGPU_Buffer * buffer, void * mapping, unsigned int const writtenOffsetAndSize[2u]);
void abGPU_Buffer_Destroy(abGPU_Buffer * buffer);

/*********
 * Images
 *********/

typedef unsigned int abGPU_Image_Options;
enum {
		abGPU_Image_Options_None = 0u, // Regular 2D texture.
	abGPU_Image_Options_Array = 1u, // Has multiple layers (not applicable to 3D).
	abGPU_Image_Options_Cube = abGPU_Image_Options_Array << 1u, // Has 6 sides (not applicable to 3D).
	abGPU_Image_Options_3D = abGPU_Image_Options_Cube << 1u, // Has 3 filterable dimensions (can't be an array or a cube).
		abGPU_Image_Options_DimensionsMask = abGPU_Image_Options_Array | abGPU_Image_Options_Cube | abGPU_Image_Options_3D,
	abGPU_Image_Options_Renderable = abGPU_Image_Options_3D << 1u, // Can be a render target.
	abGPU_Image_Options_Editable = abGPU_Image_Options_Renderable << 1u, // Can be edited by shaders.
	abGPU_Image_Options_Upload = abGPU_Image_Options_Editable << 1u, // Only accessible from the CPU (also can't be renderable or editable).
		abGPU_Image_Options_PurposeMask = abGPU_Image_Options_Renderable | abGPU_Image_Options_Editable | abGPU_Image_Options_Upload
};
abForceInline abGPU_Image_Options abGPU_Image_Options_Normalize(abGPU_Image_Options options) {
	if (options & abGPU_Image_Options_3D) { options &= ~(abGPU_Image_Options_Array | abGPU_Image_Options_Cube); }
	if (options & abGPU_Image_Options_Upload) { options &= ~(abGPU_Image_Options_Renderable | abGPU_Image_Options_Editable); }
	return options;
}

typedef enum abGPU_Image_Format {
	abGPU_Image_Format_Invalid, // Zero.

		abGPU_Image_Format_RawLDRStart,
	abGPU_Image_Format_R8 = abGPU_Image_Format_RawLDRStart,
	abGPU_Image_Format_R8G8,
	abGPU_Image_Format_R8G8B8A8,
	abGPU_Image_Format_R8G8B8A8_sRGB,
	abGPU_Image_Format_R8G8B8A8_Signed,
	abGPU_Image_Format_B8G8R8A8,
	abGPU_Image_Format_B8G8R8A8_sRGB,
	abGPU_Image_Format_B5G5R5A1,
	abGPU_Image_Format_B5G6R5,
		abGPU_Image_Format_RawLDREnd = abGPU_Image_Format_B5G6R5,

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
abForceInline abBool abGPU_Image_Format_Is4x4(abGPU_Image_Format format) {
	return format >= abGPU_Image_Format_4x4Start && format <= abGPU_Image_Format_4x4End;
}
abForceInline abBool abGPU_Image_Format_IsDepth(abGPU_Image_Format format) {
	return format >= abGPU_Image_Format_DepthStart && format <= abGPU_Image_Format_DepthEnd;
}
abForceInline abBool abGPU_Image_Format_IsDepthStencil(abGPU_Image_Format format) {
	return format >= abGPU_Image_Format_DepthStencilStart && format <= abGPU_Image_Format_DepthStencilEnd;
}
abGPU_Image_Format abGPU_Image_Format_ToLinear(abGPU_Image_Format format);
unsigned int abGPU_Image_Format_GetSize(abGPU_Image_Format format); // Block size for compressed formats.

typedef union abGPU_Image_Texel {
	float color[4u];
	struct {
		float depth;
		uint8_t stencil;
	} ds;
} abGPU_Image_Texel;

typedef struct abGPU_Image {
	abGPU_Image_Options options;
	// Depth is 1 for 2D and cubes, Z depth for 3D, and number of layers for arrays.
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

abForceInline void abGPU_Image_GetMipSize(abGPU_Image const * image, unsigned int mip,
		unsigned int * w, unsigned int * h, unsigned int * d) {
	if (w != abNull) *w = abMax(image->w >> mip, 1u);
	if (h != abNull) *h = abMax(image->h >> mip, 1u);
	if (d != abNull) *d = ((image->options & abGPU_Image_Options_3D) ? abMax(image->d >> mip, 1u) : image->d);
}

typedef unsigned int abGPU_Image_Slice;
#define abGPU_Image_Slice_Make(layer, side, mip) ((abGPU_Image_Slice) (((layer) << 8u) | ((side) << 5u) | (mip)))
#define abGPU_Image_Slice_Mip(slice) ((unsigned int) ((slice) & 31u))
#define abGPU_Image_Slice_Side(slice) ((unsigned int) (((slice) >> 5u) & 7u))
#define abGPU_Image_Slice_Layer(slice) ((unsigned int) ((slice) >> 8u))
inline abBool abGPU_Image_HasSlice(abGPU_Image const * image, abGPU_Image_Slice slice) {
	if (abGPU_Image_Slice_Mip(slice) >= image->mips) { return abFalse; }
	if (abGPU_Image_Slice_Side(slice) > ((image->options & abGPU_Image_Options_Cube) ? 5u : 0u)) { return abFalse; }
	if (abGPU_Image_Slice_Layer(slice) >= ((image->options & abGPU_Image_Options_Array) ? image->d : 1u)) { return abFalse; }
	return abTrue;
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

// These are hard engine limits, independent from the implementation, mostly to prevent overflows in loading.
enum {
	abGPU_Image_MaxAbsoluteMips2DCube = 15u,
	abGPU_Image_MaxAbsoluteMips3D = 12u,
	abGPU_Image_MaxAbsoluteSize2DCube = 1u << (abGPU_Image_MaxAbsoluteMips2DCube - 1u),
	abGPU_Image_MaxAbsoluteSize3D = 1u << (abGPU_Image_MaxAbsoluteMips3D - 1u),
	abGPU_Image_MaxAbsoluteArraySize2D = 2048u,
	abGPU_Image_MaxAbsoluteArraySizeCube = abGPU_Image_MaxAbsoluteArraySize2D / 6u
};

abForceInline unsigned int abGPU_Image_CalculateMipCount(abGPU_Image_Options dimensionOptions,
		unsigned int w, unsigned int h, unsigned int d) {
	unsigned int size = abMax(w, h);
	if (dimensionOptions & abGPU_Image_Options_3D) {
		size = abMax(size, d);
	}
	return abBit_HighestOne32(size + (size == 0u)) + 1u;
}

// Clamps to [1, max].
void abGPU_Image_ClampSizeToSupportedMax(abGPU_Image_Options dimensionOptions,
		unsigned int * w, unsigned int * h, unsigned int * d, /* optional */ unsigned int * mips);

/*
 * Implementation functions.
 */
void abGPU_Image_GetMaxSupportedSize(abGPU_Image_Options dimensionOptions,
		/* optional */ unsigned int * wh, /* optional */ unsigned int * d);
unsigned int abGPU_Image_CalculateMemoryUsage(abGPU_Image_Options options,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format);
// Usage must be Upload for upload buffers. Clear value is null for images that are not render targets.
abBool abGPU_Image_Init(abGPU_Image * image, abTextU8 const * name, abGPU_Image_Options options,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format,
		abGPU_Image_Usage initialUsage, abGPU_Image_Texel const * clearValue);
abBool abGPU_Image_RespecifyUploadBuffer(abGPU_Image * image, abGPU_Image_Options dimensionOptions,
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
		void * mapping, unsigned int const writtenOffsetAndSize[2u]);
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

abBool abGPU_HandleStore_Init(abGPU_HandleStore * store, abTextU8 const * name, unsigned int handleCount);
void abGPU_HandleStore_SetConstantBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int offset, unsigned int size);
void abGPU_HandleStore_SetStructureBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int elementSize, unsigned int firstElementIndex, unsigned int elementCount);
void abGPU_HandleStore_SetRawStructureBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int offset, unsigned int size); // 4-aligned.
void abGPU_HandleStore_SetEditBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int elementSize, unsigned int firstElementIndex, unsigned int elementCount);
void abGPU_HandleStore_SetRawEditBuffer(abGPU_HandleStore * store, unsigned int handleIndex,
		abGPU_Buffer * buffer, unsigned int offset, unsigned int size); // 4-aligned.
void abGPU_HandleStore_SetTexture(abGPU_HandleStore * store, unsigned int handleIndex, abGPU_Image * image);
void abGPU_HandleStore_SetEditImage(abGPU_HandleStore * store, unsigned int handleIndex, abGPU_Image * image, unsigned int mip);
void abGPU_HandleStore_Destroy(abGPU_HandleStore * store);

/***********************
 * Render target stores
 ***********************/

typedef struct abGPU_RTStore_RT {
	abGPU_Image * image;
	abGPU_Image_Slice slice; // For 3D images, layer is Z.
} abGPU_RTStore_RT;

typedef struct abGPU_RTStore {
	unsigned int countColor, countDepth;
	abGPU_RTStore_RT * renderTargets; // countColor + countDepth render targets.

	#if defined(abBuild_GPUi_D3D)
	ID3D12DescriptorHeap * i_descriptorHeapColor, * i_descriptorHeapDepth;
	D3D12_CPU_DESCRIPTOR_HANDLE i_cpuDescriptorHandleStartColor, i_cpuDescriptorHandleStartDepth;
	#endif
} abGPU_RTStore;

abForceInline abGPU_RTStore_RT * abGPU_RTStore_GetColor(abGPU_RTStore const * store, unsigned int rtIndex) {
	return &store->renderTargets[rtIndex];
}
abForceInline abGPU_RTStore_RT * abGPU_RTStore_GetDepth(abGPU_RTStore const * store, unsigned int rtIndex) {
	return &store->renderTargets[store->countColor + rtIndex];
}

// Implementation functions.
abBool abGPU_RTStore_Init(abGPU_RTStore * store, abTextU8 const * name, unsigned int countColor, unsigned int countDepth);
// For 3D color render targets (3D depth render targets are not supported), the array layer is the Z.
abBool abGPU_RTStore_SetColor(abGPU_RTStore * store, unsigned int rtIndex, abGPU_Image * image, abGPU_Image_Slice slice);
abBool abGPU_RTStore_SetDepth(abGPU_RTStore * store, unsigned int rtIndex, abGPU_Image * image, abGPU_Image_Slice slice, abBool readOnly);
void abGPU_RTStore_Destroy(abGPU_RTStore * store);

/***********************
 * Display image chains
 ***********************/

#define abGPU_DisplayChain_MaxImages 3u

typedef struct abGPU_DisplayChain {
	abGPU_Image images[abGPU_DisplayChain_MaxImages]; // By default have Display usage.
	unsigned int imageCount;

	#if defined(abBuild_GPUi_D3D)
	IDXGISwapChain3 *i_swapChain;
	#endif
} abGPU_DisplayChain;

#if defined(abPlatform_OS_WindowsDesktop)
abBool abGPU_DisplayChain_InitForWindowsHWnd(abGPU_DisplayChain * chain, abTextU8 const * name,
		HWND hWnd, unsigned int imageCount, abGPU_Image_Format format, unsigned int width, unsigned int height);
#endif
unsigned int abGPU_DisplayChain_GetCurrentImageIndex(abGPU_DisplayChain * chain);
void abGPU_DisplayChain_Display(abGPU_DisplayChain * chain, abBool verticalSync);
void abGPU_DisplayChain_Destroy(abGPU_DisplayChain * chain);

/*******************************
 * Render target configurations
 *******************************/

#define abGPU_RT_Count 8u

typedef unsigned int abGPU_RT_PrePostAction;
enum {
	abGPU_RT_PreDiscard,
	abGPU_RT_PreClear,
	abGPU_RT_PreLoad,
		abGPU_RT_PreMask = 3u,
	abGPU_RT_PostStore = 0u << 2u,
	abGPU_RT_PostDiscard = 1u << 2u,
		abGPU_RT_PostMask = 1u << 2u
};

typedef struct abGPU_RT {
	unsigned int indexInStore;
	abGPU_RT_PrePostAction prePostAction;
	abGPU_Image_Texel clearValue;
} abGPU_RT;

#define abGPU_RTConfig_DepthIndexNone 0xffffffffu

typedef struct abGPU_RTConfig {
	unsigned int colorCount;
	abGPU_RT color[abGPU_RT_Count];
	abGPU_RT depth;
	abGPU_RT_PrePostAction stencilPrePostAction;

	#if defined(abBuild_GPUi_D3D)
	D3D12_CPU_DESCRIPTOR_HANDLE i_descriptorHandles[abGPU_RT_Count + 2u]; // Depth and stencil (duplicated) at abGPU_RT_Count.
	ID3D12Resource * i_resources[abGPU_RT_Count + 2u]; // Depth and stencil (duplicated) at abGPU_RT_Count.
	unsigned int i_subresources[abGPU_RT_Count + 2u]; // Depth and stencil at abGPU_RT_Count.
	unsigned int i_preDiscardBits, i_preClearBits, i_postDiscardBits; // Depth and stencil at (1...2)<<abGPU_RT_Count.
	#endif
} abGPU_RTConfig;

abBool abGPU_RTConfig_Register(abGPU_RTConfig * config, abTextU8 const * name, abGPU_RTStore const * store);
#if defined(abBuild_GPUi_D3D)
#define abGPU_RTConfig_Unregister(config) {}
#else
void abGPU_RTConfig_Unregister(abGPU_RTConfig * config);
#endif

/**********
 * Shaders
 **********/

typedef enum abGPU_ShaderStage {
	abGPU_ShaderStage_Vertex,
	abGPU_ShaderStage_Pixel,
	abGPU_ShaderStage_Compute,
		abGPU_ShaderStage_Count
} abGPU_ShaderStage;

// abGPU_Input depends on this being 8 bits or less!
typedef enum abGPU_ShaderStageBits {
	abGPU_ShaderStageBits_Vertex = 1u << abGPU_ShaderStage_Vertex,
	abGPU_ShaderStageBits_Pixel = 1u << abGPU_ShaderStage_Pixel,
	abGPU_ShaderStageBits_Compute = 1u << abGPU_ShaderStage_Compute
} abGPU_ShaderStageBits;

typedef struct abGPU_ShaderCode {
	#if defined(abBuild_GPUi_D3D)
	ID3D10Blob * i_blob;
	#endif
} abGPU_ShaderCode;

// Creates a shader library (in D3D, with a single entry point). Doesn't hold a reference to the source.
abBool abGPU_ShaderCode_Init(abGPU_ShaderCode * code, void const * source, size_t sourceLength);
void abGPU_ShaderCode_Destroy(abGPU_ShaderCode * code);

/***********
 * Vertices
 ***********/

#define abGPU_VertexData_MaxAttributes 16u
#define abGPU_VertexData_MaxBuffers abGPU_VertexData_MaxAttributes

typedef enum abGPU_VertexData_Type {
	abGPU_VertexData_Type_Position, // Recommended Float32x3.
	abGPU_VertexData_Type_Normal, // SNorm16x4.
	abGPU_VertexData_Type_Tangent, // SNorm16x4. S tangent in XYZ and T tangent (cross) sign in W.
	abGPU_VertexData_Type_BlendIndices, // UInt8x4.
	abGPU_VertexData_Type_BlendWeights, // UNorm8x4.
	abGPU_VertexData_Type_Color, // UNorm8x4.
	abGPU_VertexData_Type_TexCoord, // Float32x2 generally.
	abGPU_VertexData_Type_TexCoordDetail, // Float32x2 or Float32x3 generally.
	abGPU_VertexData_Type_Custom,
		abGPU_VertexData_Type_CustomCount = abGPU_VertexData_MaxAttributes - abGPU_VertexData_Type_Custom
} abGPU_VertexData_Type;

typedef enum abGPU_VertexData_Format {
	abGPU_VertexData_Format_UInt8x4,
	abGPU_VertexData_Format_UNorm8x4,
	abGPU_VertexData_Format_SNorm8x4,

	abGPU_VertexData_Format_SNorm16x2,
	abGPU_VertexData_Format_Float16x2,
	abGPU_VertexData_Format_SNorm16x4,
	abGPU_VertexData_Format_Float16x4,

	abGPU_VertexData_Format_Float32x1,
	abGPU_VertexData_Format_Float32x2,
	abGPU_VertexData_Format_Float32x3,
	abGPU_VertexData_Format_Float32x4
} abGPU_VertexData_Format;

unsigned int abGPU_VertexData_Format_GetSize(abGPU_VertexData_Format format);

typedef struct abGPU_VertexData_Attribute {
	uint8_t type; // abGPU_VertexData_Type - only one attribute per type allowed.
	uint8_t format; // abGPU_VertexData_Format.
	uint8_t bufferIndex;
	uint8_t offset;
} abGPU_VertexData_Attribute;

typedef struct abGPU_VertexData_Buffer {
	uint8_t stride; // Can't be smaller than one vertex or one sequence of instances - will be clamped (auto-calculated) if it is!
	uint8_t instanceRate; // 0 for per-vertex, 1 for per-instance, 2 for loading every 2 instances...
} abGPU_VertexData_Buffer;

void abGPU_VertexData_Convert_Float32ToSNorm16_Array(int16_t * target, float const * source, size_t componentCount);
void abGPU_VertexData_Convert_Float32x3ToSNorm16x4_Array(int16_t * target, float const * source, size_t vertexCount);
void abGPU_VertexData_Convert_UNorm8ToFloat32_Array(float * target, uint8_t const * source, size_t componentCount);
void abGPU_VertexData_Convert_SNorm8ToFloat32_Array(float * target, int8_t const * source, size_t componentCount);
void abGPU_VertexData_Convert_UNorm16ToFloat32_Array(float * target, uint16_t const * source, size_t componentCount);
void abGPU_VertexData_Convert_SNorm16ToFloat32_Array(float * target, int16_t const * source, size_t componentCount);

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

abBool abGPU_SamplerStore_Init(abGPU_SamplerStore * store, abTextU8 const * name, unsigned int samplerCount);
void abGPU_SamplerStore_SetSampler(abGPU_SamplerStore * store, unsigned int samplerIndex, abGPU_Sampler sampler);
void abGPU_SamplerStore_Destroy(abGPU_SamplerStore * store);

/****************
 * Shader inputs
 ****************/

/*
 * Uniforms are a special type of shader input.
 * They are used to send a small amount of frequently changing data (below 4 KB, generally less than 256 bytes).
 *
 * An abGPU implementation may choose an optimal uniform binding strategy depending on what the graphics API allows.
 *
 * On Metal, set[Stage]Bytes is the preferred way of sending data smaller than 4 KB. But on Direct3D and Vulkan,
 * root/push constants are used for tiny amounts of data (even a float4 may be too much), so a buffer is needed.
 *
 * With D3D's 256 alignment requirement, it may be wasteful to use a new 256-byte page for every draw.
 * In this case, the buffer bound is one or multiple (like LCM(needed data size, 256)/256 pages) pages,
 * and one 32-bit constant is used for the index of the per-draw data block within the bound pages.
 *
 * However, this has to be handled by the game rendering code, and imposes some requirements on the input configuration.
 * Depending on abGPU_Input_PreferredUniformStrategy, the game should create or not create buffers for uniforms,
 * align buffer offsets, send the draw index as a 32-bit constant.
 * If the shader uses uniforms the two-level way described above on certain APIs,
 * the number of 32-bit constants should be (abGPU_InputConfig_UniformDrawIndex32BitCount + user 32-bit constant count).
 * abGPU_InputConfig_UniformDrawIndex32BitCount is 1 or 0 depending on whether two-level uniforms are used.
 */

// Strategy for binding of small amounts of constant data (below 4 KB, generally less than 256 bytes).
typedef enum abGPU_Input_UniformStrategy {
	abGPU_Input_UniformStrategy_RawData, // Send the data directly through the command list.
	abGPU_Input_UniformStrategy_256AlignedBuffer // Bind a buffer directly, without a handle, with a 256-aligned offset.
} abGPU_Input_UniformStrategy;
#if defined(abBuild_GPUi_D3D)
#define abGPU_Input_PreferredUniformStrategy abGPU_Input_UniformStrategy_256AlignedBuffer
#else
#error No shader input preferences defined for the target GPU API.
#endif

typedef enum abGPU_Input_Type {
	abGPU_Input_Type_ConstantBuffer, // Buffer and offset bound directly.
	abGPU_Input_Type_ConstantBufferHandle, // Buffer bound via a handle.
	abGPU_Input_Type_Uniform, // One of the constant input types, depending on the strategy.
	abGPU_Input_Type_StructureBufferHandle,
	abGPU_Input_Type_EditBufferHandle,
	abGPU_Input_Type_TextureHandle,
	abGPU_Input_Type_EditImageHandle,
	abGPU_Input_Type_SamplerHandle
} abGPU_Input_Type;

/*
 * Mapping of input types to shader binding points:
 *
 * Direct3D:
 * - Constant buffers and constant buffer handles: b# space0.
 * - Uniform: b0 space1 for 32-bit constants, b1 space1 for the buffer (both are optional, first in the root signature).
 * - Structure buffers: t# space1.
 * - Editable buffers: u# space1.
 * - Textures: t# space0.
 * - Editable images: u# space0.
 * - Samplers: s#, both dynamic and static.
 *
 * Metal (potentially) - ordered by likelihood of index not being changed for the same data:
 * - Buffers: constant (like view parameters), structure, uniform (single), vertex.
 * - Images: textures, editable.
 * - Samplers.
 *
 * Vulkan (potentially):
 * - Uniform: push constants, explicit index (globalIndex) for the buffer (both are optional).
 * - Everything else: explicit indices.
 */

// These must be immutable as long as they are attached to an active input list.
#define abGPU_Input_SamplerDynamicOnly UINT8_MAX
typedef struct abGPU_Input {
	uint8_t stages; // abGPU_ShaderStageBits - zero to skip this input.
	uint8_t type;
	union {
		struct { uint8_t constantBufferIndex, globalIndex; } constantBuffer;
		struct { uint8_t constantBufferFirstIndex, globalIndex, count; } constantBufferHandle;
		struct { uint8_t structureBufferFirstIndex, globalIndex, count; } structureBufferHandle;
		struct { uint8_t editBufferFirstIndex, globalIndex, count; } editBufferHandle;
		struct { uint8_t textureFirstIndex, globalIndex, count; } imageHandle;
		struct { uint8_t editImageFirstIndex, globalIndex, count; } editImageHandle;
		// Static samplers are an optimization that may be unsupported, not a full replacement for binding.
		// staticSamplerIndex is an index in the array of static samplers that is passed when the input list is created.
		// If static samplers are supported and provided, they will override bindings for non-SamplerDynamicOnly inputs.
		// However, as they may be unsupported by implementations, samplers still must be bound.
		struct { uint8_t samplerFirstIndex, globalIndex, count, staticSamplerIndex; } samplerHandle;
	} parameters;
} abGPU_Input;

#define abGPU_InputConfig_MaxInputs 24u // No more than 16 should really be used, including uniforms, but for more space for static samplers.

// abGPU_InputConfig_UniformDrawIndex32BitCount should be added to uniform32BitCount if a draw index within 256-byte blocks is used.
#if (abGPU_Input_PreferredUniformStrategy == abGPU_Input_UniformStrategy_256AlignedBuffer)
#define abGPU_InputConfig_UniformDrawIndex32BitCount 1u
#else
#define abGPU_InputConfig_UniformDrawIndex32BitCount 0u
#endif

typedef struct abGPU_InputConfig {
	unsigned int inputCount;
	abGPU_Input inputs[abGPU_InputConfig_MaxInputs];

	uint8_t uniformStages;
	uint8_t uniform32BitCount; // Add abGPU_InputConfig_UniformDrawIndex32BitCount to this if needed.
	abBool uniformUseBuffer;
	uint8_t uniformBufferGlobalIndex;

	unsigned int vertexBufferCount, vertexAttributeCount;
	abGPU_VertexData_Buffer vertexBuffers[abGPU_VertexData_MaxBuffers];
	abGPU_VertexData_Attribute vertexAttributes[abGPU_VertexData_MaxAttributes];

	#if defined(abBuild_GPUi_D3D)
	ID3D12RootSignature * i_rootSignature;
	uint8_t i_rootParameters[abGPU_InputConfig_MaxInputs]; // UINT8_MAX means it's skipped.
	// 32-bit constants and uniform buffer use 0 and 1 root parameter indices (both 0 if there's only one of them).
	D3D12_INPUT_ELEMENT_DESC i_vertexElements[abGPU_VertexData_MaxAttributes];
	#endif
} abGPU_InputConfig;

abBool abGPU_InputConfig_Register(abGPU_InputConfig * config, abTextU8 const * name, /* optional */ abGPU_Sampler const * staticSamplers);
void abGPU_InputConfig_Unregister(abGPU_InputConfig * config);

/**************************
 * Graphics configurations
 **************************/

typedef unsigned int abGPU_DrawConfig_Options;
enum {
	abGPU_DrawConfig_Options_16BitVertexIndices = 1u,
	abGPU_DrawConfig_Options_FrontCW = abGPU_DrawConfig_Options_16BitVertexIndices << 1u,
	abGPU_DrawConfig_Options_CullBack = abGPU_DrawConfig_Options_FrontCW << 1u, // Either Back or Front!
	abGPU_DrawConfig_Options_CullFront = abGPU_DrawConfig_Options_CullBack << 1u, // Front has a higher priority because it's more special.
	abGPU_DrawConfig_Options_Wireframe = abGPU_DrawConfig_Options_CullFront << 1u,
	abGPU_DrawConfig_Options_DepthClip = abGPU_DrawConfig_Options_Wireframe << 1u,
	abGPU_DrawConfig_Options_DepthNoTest = abGPU_DrawConfig_Options_DepthClip << 1u,
	abGPU_DrawConfig_Options_DepthNoWrite = abGPU_DrawConfig_Options_DepthNoTest << 1u, // No depth writing with depth testing disabled either.
	abGPU_DrawConfig_Options_Stencil = abGPU_DrawConfig_Options_DepthNoWrite << 1u,
	abGPU_DrawConfig_Options_BlendAndMaskSeparate = abGPU_DrawConfig_Options_Stencil << 1u // Separate blend config for each RT, not RT 0's one.
};

typedef struct abGPU_DrawConfig_Shader {
	abGPU_ShaderCode const * code; // Null to skip this stage.
	char const * entryPoint;
} abGPU_DrawConfig_Shader;

typedef enum abGPU_DrawConfig_TopologyClass {
	abGPU_DrawConfig_TopologyClass_Triangle, // Zero.
	abGPU_DrawConfig_TopologyClass_Point,
	abGPU_DrawConfig_TopologyClass_Line
} abGPU_DrawConfig_TopologyClass;

typedef enum abGPU_DrawConfig_StencilOperation {
	abGPU_DrawConfig_StencilOperation_Keep,
	abGPU_DrawConfig_StencilOperation_Zero,
	abGPU_DrawConfig_StencilOperation_Replace,
	abGPU_DrawConfig_StencilOperation_IncrementClamp,
	abGPU_DrawConfig_StencilOperation_DecrementClamp,
	abGPU_DrawConfig_StencilOperation_Invert,
	abGPU_DrawConfig_StencilOperation_IncrementWrap,
	abGPU_DrawConfig_StencilOperation_DecrementWrap,
		abGPU_DrawConfig_StencilOperation_Count
} abGPU_DrawConfig_StencilOperation;

typedef struct abGPU_DrawConfig_StencilSide {
	abGPU_DrawConfig_StencilOperation pass, passDepthFail, fail;
	abGPU_Image_Comparison comparison;
} abGPU_DrawConfig_StencilSide;

typedef enum abGPU_DrawConfig_BlendFactor {
	abGPU_DrawConfig_BlendFactor_Zero,
	abGPU_DrawConfig_BlendFactor_One,
	abGPU_DrawConfig_BlendFactor_SrcColor,
	abGPU_DrawConfig_BlendFactor_SrcColorRev, // 1 - source color.
	abGPU_DrawConfig_BlendFactor_SrcAlpha,
	abGPU_DrawConfig_BlendFactor_SrcAlphaRev,
	abGPU_DrawConfig_BlendFactor_SrcAlphaSat,
	abGPU_DrawConfig_BlendFactor_DestColor,
	abGPU_DrawConfig_BlendFactor_DestColorRev,
	abGPU_DrawConfig_BlendFactor_DestAlpha,
	abGPU_DrawConfig_BlendFactor_DestAlphaRev,
	abGPU_DrawConfig_BlendFactor_Constant, // Set while drawing.
	abGPU_DrawConfig_BlendFactor_ConstantRev,
	abGPU_DrawConfig_BlendFactor_Src1Color, // Fragment shader output 1.
	abGPU_DrawConfig_BlendFactor_Src1ColorRev,
	abGPU_DrawConfig_BlendFactor_Src1Alpha,
	abGPU_DrawConfig_BlendFactor_Src1AlphaRev,
		abGPU_DrawConfig_BlendFactor_Count
} abGPU_DrawConfig_BlendFactor;

enum {
	abGPU_DrawConfig_DisableR = 1u << 0u,
	abGPU_DrawConfig_DisableG = 1u << 1u,
	abGPU_DrawConfig_DisableB = 1u << 2u,
	abGPU_DrawConfig_DisableA = 1u << 3u,
		abGPU_DrawConfig_DisableRGB = 7u,
		abGPU_DrawConfig_DisableRGBA = 15u
};

typedef enum abGPU_DrawConfig_BlendOperation {
	abGPU_DrawConfig_BlendOperation_Add, // Zero.
	abGPU_DrawConfig_BlendOperation_Subtract,
	abGPU_DrawConfig_BlendOperation_RevSubtract,
	abGPU_DrawConfig_BlendOperation_Min,
	abGPU_DrawConfig_BlendOperation_Max,
		abGPU_DrawConfig_BlendOperation_Count
} abGPU_DrawConfig_BlendOperation;

typedef enum abGPU_DrawConfig_BitOperation {
	abGPU_DrawConfig_BitOperation_Copy, // s - zero, means bit operation is disabled.
	abGPU_DrawConfig_BitOperation_CopyInv, // ~s
	abGPU_DrawConfig_BitOperation_SetZero, // 0
	abGPU_DrawConfig_BitOperation_SetOne, // 1
	abGPU_DrawConfig_BitOperation_Reverse, // ~d
	abGPU_DrawConfig_BitOperation_And, // d & s
	abGPU_DrawConfig_BitOperation_NotAnd, // ~(d & s)
	abGPU_DrawConfig_BitOperation_Or, // d | s
	abGPU_DrawConfig_BitOperation_NotOr, // ~(d | s)
	abGPU_DrawConfig_BitOperation_Xor, // d ^ s
	abGPU_DrawConfig_BitOperation_Equal, // ~(d ^ s)
	abGPU_DrawConfig_BitOperation_AndRev, // ~d & s
	abGPU_DrawConfig_BitOperation_AndInv, // d & ~s
	abGPU_DrawConfig_BitOperation_OrRev, // ~d | s
	abGPU_DrawConfig_BitOperation_OrInv, // d | ~s
		abGPU_DrawConfig_BitOperation_Count
} abGPU_DrawConfig_BitOperation;
#if defined(abBuild_GPUi_D3D)
#define abGPU_DrawConfig_BitOperationsSupported 1u
#endif

typedef struct abGPU_DrawConfig_RT {
	abGPU_Image_Format format;
	abBool blend;
	uint8_t blendFactorSrcColor, blendFactorSrcAlpha; // abGPU_DrawConfig_BlendFactor
	uint8_t blendFactorDestColor, blendFactorDestAlpha; // abGPU_DrawConfig_BlendFactor
	uint8_t blendOperationColor, blendOperationAlpha; // abGPU_DrawConfig_BlendOperation
	uint8_t bitOperation; // abGPU_DrawConfig_BitOperation
	uint8_t disabledComponentMask;
} abGPU_DrawConfig_RT;

typedef struct abGPU_DrawConfig {
	abGPU_DrawConfig_Options options;

	abGPU_DrawConfig_Shader vertexShader, pixelShader;

	abGPU_InputConfig * inputConfig;

	abGPU_DrawConfig_TopologyClass topologyClass;

	abGPU_Image_Format depthFormat; // If not a depth format (or Invalid, which is 0), depth test/write is disabled, same for stencil.
	int depthBias;
	float depthBiasSlope;
	abGPU_Image_Comparison depthComparison;
	uint8_t stencilReadMask, stencilWriteMask;
	abGPU_DrawConfig_StencilSide stencilFront, stencilBack;

	abGPU_DrawConfig_RT renderTargets[abGPU_RT_Count]; // Terminated with an invalid format RT.

	#if defined(abBuild_GPUi_D3D)
	ID3D12PipelineState * i_pipelineState;
	#endif
} abGPU_DrawConfig;

abBool abGPU_DrawConfig_Register(abGPU_DrawConfig * config, abTextU8 const * name); // May take a significantly long time!
void abGPU_DrawConfig_Unregister(abGPU_DrawConfig * config);

/****************
 * Command lists
 ****************/

typedef struct abGPU_CmdList {
	abGPU_CmdQueue queue;

	#if defined(abBuild_GPUi_D3D)
	ID3D12CommandAllocator * i_allocator;
	ID3D12GraphicsCommandList * i_list;
	ID3D12CommandList * i_executeList; // Same object as i_list, but different interface.

	abGPU_HandleStore const * i_handleStore;
	abGPU_SamplerStore const * i_samplerStore;
	abGPU_RTConfig const * i_drawRTConfig;
	abGPU_DrawConfig const * i_drawConfig;
	#endif
} abGPU_CmdList;

typedef enum abGPU_CmdList_Topology {
	abGPU_CmdList_Topology_TriangleList,
	abGPU_CmdList_Topology_TriangleStrip,
	abGPU_CmdList_Topology_PointList,
	abGPU_CmdList_Topology_LineList,
	abGPU_CmdList_Topology_LineStrip
} abGPU_CmdList_Topology;

abBool abGPU_CmdList_Init(abGPU_CmdList * list, abTextU8 const * name, abGPU_CmdQueue queue);
void abGPU_CmdList_Record(abGPU_CmdList * list);
void abGPU_CmdList_Abort(abGPU_CmdList * list); // Stops recording without submitting (useful for file loading errors).
void abGPU_CmdList_Submit(abGPU_CmdList * const * lists, unsigned int listCount);
void abGPU_CmdList_Destroy(abGPU_CmdList * list);

// Setup.
void abGPU_Cmd_SetHandleAndSamplerStores(abGPU_CmdList * list,
		/* optional */ abGPU_HandleStore * handleStore, /* optional */ abGPU_SamplerStore * samplerStore);

// Drawing.
void abGPU_Cmd_DrawingBegin(abGPU_CmdList * list, abGPU_RTConfig const * rtConfig);
void abGPU_Cmd_DrawingEnd(abGPU_CmdList * list);
abBool abGPU_Cmd_DrawSetConfig(abGPU_CmdList * list, abGPU_DrawConfig * drawConfig); // Returns whether need to rebind all inputs.
void abGPU_Cmd_DrawSetTopology(abGPU_CmdList * list, abGPU_CmdList_Topology topology);
void abGPU_Cmd_DrawSetVertexIndices(abGPU_CmdList * list, abGPU_Buffer * buffer, unsigned int offset, unsigned int indexCount);
void abGPU_Cmd_DrawSetViewport(abGPU_CmdList * list, float x, float y, float w, float h, float zZero, float zOne);
void abGPU_Cmd_DrawSetScissor(abGPU_CmdList * list, int x, int y, unsigned int w, unsigned int h);
void abGPU_Cmd_DrawSetStencilReference(abGPU_CmdList * list, uint8_t reference);
void abGPU_Cmd_DrawSetBlendConstant(abGPU_CmdList * list, float const rgba[4u]);
void abGPU_Cmd_DrawIndexed(abGPU_CmdList * list, unsigned int indexCount, unsigned int firstIndex, int vertexIndexOffset,
		unsigned int instanceCount, unsigned int firstInstance);
void abGPU_Cmd_DrawSequential(abGPU_CmdList * list, unsigned int vertexCount, unsigned int firstVertex,
		unsigned int instanceCount, unsigned int firstInstance);

// Inputs - drawing and computing.
void abGPU_Cmd_InputUniform32BitValues(abGPU_CmdList * list, void const * values);
void abGPU_Cmd_InputUniformBuffer(abGPU_CmdList * list, abGPU_Buffer * buffer, unsigned int offset, unsigned int size);
void abGPU_Cmd_InputConstantBuffer(abGPU_CmdList * list, unsigned int inputIndex, abGPU_Buffer * buffer, unsigned int offset, unsigned int size);
void abGPU_Cmd_InputConstantBufferHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex);
void abGPU_Cmd_InputStructureBufferHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex);
void abGPU_Cmd_InputEditBufferHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex);
void abGPU_Cmd_InputTextureHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex);
void abGPU_Cmd_InputEditImageHandles(abGPU_CmdList * list, unsigned int inputIndex, unsigned int firstHandleIndex);
void abGPU_Cmd_InputVertexData(abGPU_CmdList * list, unsigned int firstBufferIndex, unsigned int bufferCount,
		abGPU_Buffer * const * buffers, /* optional */ unsigned int const * offsets, unsigned int vertexCount, unsigned int instanceCount);

// Copying.
#if defined(abBuild_GPUi_D3D)
#define abGPU_Cmd_CopyingBegin(list) {}
#define abGPU_Cmd_CopyingEnd(list) {}
#else
void abGPU_Cmd_CopyingBegin(abGPU_CmdList * list);
void abGPU_Cmd_CopyingEnd(abGPU_CmdList * list);
#endif
void abGPU_Cmd_CopyBuffer(abGPU_CmdList * list, abGPU_Buffer * target, abGPU_Buffer * source);
void abGPU_Cmd_CopyBufferRange(abGPU_CmdList * list, abGPU_Buffer * target, unsigned int targetOffset,
		abGPU_Buffer * source, unsigned int sourceOffset, unsigned int size);
// For depth/stencil images, only CopyImage or CopyImageDepth must be used - not CopyImageSlice or CopyImageArea!
void abGPU_Cmd_CopyImage(abGPU_CmdList * list, abGPU_Image * target, abGPU_Image * source);
void abGPU_Cmd_CopyImageSlice(abGPU_CmdList * list, abGPU_Image * target, abGPU_Image_Slice targetSlice,
		abGPU_Image * source, abGPU_Image_Slice sourceSlice);
void abGPU_Cmd_CopyImageDepth(abGPU_CmdList * list, abGPU_Image * target, abGPU_Image_Slice targetSlice,
		abGPU_Image * source, abGPU_Image_Slice sourceSlice, abBool depth, abBool stencil);
void abGPU_Cmd_CopyImageArea(abGPU_CmdList * list,
		abGPU_Image * target, abGPU_Image_Slice targetSlice, unsigned int targetX, unsigned int targetY, unsigned int targetZ,
		abGPU_Image * source, abGPU_Image_Slice sourceSlice, unsigned int sourceX, unsigned int sourceY, unsigned int sourceZ,
		unsigned int w, unsigned int h, unsigned int d);

#endif
