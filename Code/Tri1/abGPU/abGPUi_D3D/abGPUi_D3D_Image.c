#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

static DXGI_FORMAT abGPUi_D3D_Image_FormatToResource(abGPU_Image_Format format) {
	switch (format) {
	case abGPU_Image_Format_R8G8B8A8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case abGPU_Image_Format_D32:
		return DXGI_FORMAT_R32_TYPELESS;
	case abGPU_Image_Format_D24S8:
		return DXGI_FORMAT_R24G8_TYPELESS;
	}
	return DXGI_FORMAT_UNKNOWN;
}

D3D12_RESOURCE_STATES abGPUi_D3D_Image_UsageToStates(abGPU_Image_Usage usage) {
	switch (usage) {
	case abGPU_Image_Usage_Texture:
		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case abGPU_Image_Usage_TextureNonPixelStage:
		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	case abGPU_Image_Usage_TextureAnyStage:
		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	case abGPU_Image_Usage_RenderTarget:
		return D3D12_RESOURCE_STATE_RENDER_TARGET;
	case abGPU_Image_Usage_Display:
		return D3D12_RESOURCE_STATE_PRESENT;
	case abGPU_Image_Usage_DepthWrite:
		return D3D12_RESOURCE_STATE_DEPTH_WRITE;
	case abGPU_Image_Usage_DepthTest:
		return D3D12_RESOURCE_STATE_DEPTH_READ;
	case abGPU_Image_Usage_DepthTestAndTexture:
		return D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case abGPU_Image_Usage_Edit:
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	case abGPU_Image_Usage_CopySource:
		return D3D12_RESOURCE_STATE_COPY_SOURCE;
	case abGPU_Image_Usage_CopyDestination:
		return D3D12_RESOURCE_STATE_COPY_DEST;
	case abGPU_Image_Usage_CopyQueue:
		return D3D12_RESOURCE_STATE_COMMON;
	}
	return D3D12_RESOURCE_STATE_COMMON; // This shouldn't happen!
}

static void abGPUi_D3D_Image_FillTextureDesc(abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips,
		abGPU_Image_Format format, D3D12_RESOURCE_DESC *desc) {
	desc->Dimension = (dimensions == abGPU_Image_Dimensions_3D ?
		D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D);
	desc->Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc->Width = w;
	desc->Height = h;
	desc->DepthOrArraySize = d;
	if (dimensions == abGPU_Image_Dimensions_Cube || dimensions == abGPU_Image_Dimensions_CubeArray) {\
		desc->DepthOrArraySize *= 6;
	}
	desc->MipLevels = mips;
	desc->Format = abGPUi_D3D_Image_FormatToResource(format);
	desc->SampleDesc.Count = 1;
	desc->SampleDesc.Quality = 0;
	desc->Flags = D3D12_RESOURCE_FLAG_NONE;
	if (type & abGPU_Image_Type_Renderable) {
		desc->Flags |= (abGPU_Image_Format_IsDepth(format) ?
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	}
	if (type & abGPU_Image_Type_Editable) {
		desc->Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
}

void abGPU_Image_GetMaxSize(abGPU_Image_Dimensions dimensions, unsigned int *wh, unsigned int *d) {
	unsigned int maxWH = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, maxD = 1;
	switch (dimensions) {
	case abGPU_Image_Dimensions_2DArray:
		maxD = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
		return;
	case abGPU_Image_Dimensions_Cube:
		maxWH = D3D12_REQ_TEXTURECUBE_DIMENSION;
		return;
	case abGPU_Image_Dimensions_CubeArray:
		maxWH = D3D12_REQ_TEXTURECUBE_DIMENSION;
		maxD = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / 6;
		return;
	case abGPU_Image_Dimensions_3D:
		maxWH = maxD = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
		return;
	}
	if (wh != abNull) {
		*wh = maxWH;
	}
	if (d != abNull) {
		*d = maxD;
	}
}

static unsigned int abGPUi_D3D_Image_CalculateMemoryUsageForDesc(const D3D12_RESOURCE_DESC *desc) {
	unsigned int subresourceCount = desc->MipLevels;
	uint64_t memoryUsage;
	if (desc->Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
		subresourceCount *= desc->DepthOrArraySize;
	}
	if (desc->Format == DXGI_FORMAT_R24G8_TYPELESS || desc->Format == DXGI_FORMAT_R32G8X24_TYPELESS) {
		subresourceCount *= 2; // Depth and stencil planes.
	}
	ID3D12Device_GetCopyableFootprints(abGPUi_D3D_Device, desc, 0, subresourceCount,
			0, abNull, abNull, abNull, &memoryUsage);
	return (unsigned int) memoryUsage;
}

unsigned int abGPU_Image_CalculateMemoryUsage(abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format) {
	D3D12_RESOURCE_DESC desc;
	abGPUi_D3D_Image_FillTextureDesc(type, dimensions, w, h, d, mips, format, &desc);
	return abGPUi_D3D_Image_CalculateMemoryUsageForDesc(&desc);
}

static void abGPUi_D3D_Image_GetDataLayout(const D3D12_RESOURCE_DESC *desc,
		unsigned int *mipOffset, unsigned int *mipRowStride, unsigned int *layerStride) {
	unsigned int mip, mips = desc->MipLevels;
	bool isArray = (desc->Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D && desc->DepthOrArraySize > 1);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[D3D12_REQ_MIP_LEVELS + 1];
	ID3D12Device_GetCopyableFootprints(abGPUi_D3D_Device, desc, 0, mips + isArray,
			0, footprints, abNull, abNull, abNull);
	for (mip = 0; mip < mips; ++mip) {
		mipOffset[mip] = (unsigned int) footprints[mip].Offset;
		mipRowStride[mip] = footprints[mip].Footprint.RowPitch;
	}
	*layerStride = (isArray ? (unsigned int) (footprints[mips].Offset - footprints[0].Offset) : 0);
}

bool abGPU_Image_Init(abGPU_Image *image, abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format,
		abGPU_Image_Usage initialUsage, const abGPU_Image_Texel *clearValue) {
	D3D12_RESOURCE_DESC desc;

	if (type & abGPU_Image_Type_Upload) {
		type = abGPU_Image_Type_Upload;
	}

	abGPU_Image_ClampSizeToMax(dimensions, &w, &h, &d, &mips);

	abGPUi_D3D_Image_FillTextureDesc(type, dimensions, w, h, d, mips, format, &desc);
	if (desc.Format == DXGI_FORMAT_UNKNOWN) {
		return false;
	}

	image->typeAndDimensions = (unsigned int) type | ((unsigned int) dimensions << abGPU_Image_DimensionsShift);
	image->w = w;
	image->h = h;
	image->d = d;
	image->mips = mips;
	image->format = format;
	image->memoryUsage = abGPUi_D3D_Image_CalculateMemoryUsageForDesc(&desc);
	image->i.dxgiFormat = desc.Format;
	abGPUi_D3D_Image_GetDataLayout(&desc, image->i.mipOffset, image->i.mipRowStride, &image->i.layerStride);

	{
		D3D12_HEAP_PROPERTIES heapProperties = { 0 };
		D3D12_RESOURCE_STATES initialStates;
		if (type & abGPU_Image_Type_Upload) {
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			desc.Width = image->memoryUsage;
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			initialStates = D3D12_RESOURCE_STATE_GENERIC_READ;
		} else {
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			initialStates = abGPUi_D3D_Image_UsageToStates(initialUsage);
		}
		// TODO: Clear value.
		return SUCCEEDED(ID3D12Device_CreateCommittedResource(abGPUi_D3D_Device,
				&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, initialStates, abNull,
				&IID_ID3D12Resource, &image->i.resource)) ? true : false;
	}
}

void abGPU_Image_Destroy(abGPU_Image *image) {
	ID3D12Resource_Release(image->i.resource);
}

#endif
