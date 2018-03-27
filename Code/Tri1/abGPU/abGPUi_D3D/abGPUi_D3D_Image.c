#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

static DXGI_FORMAT abGPUi_D3D_Image_FormatToResourceMap[abGPU_Image_Format_Count] = {
	[abGPU_Image_Format_R8G8B8A8] = DXGI_FORMAT_R8G8B8A8_UNORM,
	[abGPU_Image_Format_R8G8B8A8_sRGB] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	[abGPU_Image_Format_R8G8B8A8_Signed] = DXGI_FORMAT_R8G8B8A8_SNORM,
	[abGPU_Image_Format_S3TC_A1] = DXGI_FORMAT_BC1_UNORM,
	[abGPU_Image_Format_S3TC_A1_sRGB] = DXGI_FORMAT_BC1_UNORM_SRGB,
	[abGPU_Image_Format_S3TC_A4] = DXGI_FORMAT_BC2_UNORM,
	[abGPU_Image_Format_S3TC_A4_sRGB] = DXGI_FORMAT_BC2_UNORM_SRGB,
	[abGPU_Image_Format_S3TC_A8] = DXGI_FORMAT_BC3_UNORM,
	[abGPU_Image_Format_S3TC_A8_sRGB] = DXGI_FORMAT_BC3_UNORM_SRGB,
	[abGPU_Image_Format_3Dc_X] = DXGI_FORMAT_BC4_UNORM,
	[abGPU_Image_Format_3Dc_X_Signed] = DXGI_FORMAT_BC4_SNORM,
	[abGPU_Image_Format_3Dc_XY] = DXGI_FORMAT_BC5_UNORM,
	[abGPU_Image_Format_3Dc_XY_Signed] = DXGI_FORMAT_BC5_SNORM,
	[abGPU_Image_Format_D32] = DXGI_FORMAT_R32_TYPELESS,
	[abGPU_Image_Format_D24S8] = DXGI_FORMAT_R24G8_TYPELESS
	// 0 is DXGI_FORMAT_UNKNOWN.
};

DXGI_FORMAT abGPUi_D3D_Image_FormatToResource(abGPU_Image_Format format) {
	return ((unsigned int) format < abArrayLength(abGPUi_D3D_Image_FormatToResourceMap) ?
			abGPUi_D3D_Image_FormatToResourceMap[format] : DXGI_FORMAT_UNKNOWN);
}

DXGI_FORMAT abGPUi_D3D_Image_FormatToShaderResource(abGPU_Image_Format format) {
	switch (format) {
	case abGPU_Image_Format_D32:
		return DXGI_FORMAT_R32_FLOAT;
	case abGPU_Image_Format_D24S8:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	return abGPUi_D3D_Image_FormatToResource(format);
}

DXGI_FORMAT abGPUi_D3D_Image_FormatToDepthStencil(abGPU_Image_Format format) {
	switch (format) {
	case abGPU_Image_Format_D32:
		return DXGI_FORMAT_D32_FLOAT;
	case abGPU_Image_Format_D24S8:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	return abGPUi_D3D_Image_FormatToResource(format);
}

static D3D12_RESOURCE_STATES abGPUi_D3D_Image_UsageToStatesMap[abGPU_Image_Usage_Count] = {
	[abGPU_Image_Usage_Texture] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	[abGPU_Image_Usage_TextureNonPixelStage] = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
	[abGPU_Image_Usage_TextureAnyStage] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
	[abGPU_Image_Usage_RenderTarget] = D3D12_RESOURCE_STATE_RENDER_TARGET,
	[abGPU_Image_Usage_Display] = D3D12_RESOURCE_STATE_PRESENT,
	[abGPU_Image_Usage_DepthWrite] = D3D12_RESOURCE_STATE_DEPTH_WRITE,
	[abGPU_Image_Usage_DepthTest] = D3D12_RESOURCE_STATE_DEPTH_READ,
	[abGPU_Image_Usage_DepthTestAndTexture] = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	[abGPU_Image_Usage_Edit] = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
	[abGPU_Image_Usage_CopySource] = D3D12_RESOURCE_STATE_COPY_SOURCE,
	[abGPU_Image_Usage_CopyDestination] = D3D12_RESOURCE_STATE_COPY_DEST,
	[abGPU_Image_Usage_CopyQueue] = D3D12_RESOURCE_STATE_COMMON,
	[abGPU_Image_Usage_Upload] = D3D12_RESOURCE_STATE_GENERIC_READ
	// 0 is D3D12_RESOURCE_STATE_COMMON.
};

D3D12_RESOURCE_STATES abGPUi_D3D_Image_UsageToStates(abGPU_Image_Usage usage) {
	return ((unsigned int) usage < abArrayLength(abGPUi_D3D_Image_UsageToStatesMap) ?
			abGPUi_D3D_Image_UsageToStatesMap[usage] : D3D12_RESOURCE_STATE_COMMON);
}

static void abGPUi_D3D_Image_FillTextureDesc(abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips,
		abGPU_Image_Format format, D3D12_RESOURCE_DESC * desc) {
	desc->Dimension = (abGPU_Image_Dimensions_Are3D(dimensions) ?
			D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D);
	desc->Alignment = 0;
	desc->Width = w;
	desc->Height = h;
	desc->DepthOrArraySize = d;
	if (abGPU_Image_Dimensions_AreCube(dimensions)) {
		desc->DepthOrArraySize *= 6u;
	}
	desc->MipLevels = mips;
	desc->Format = abGPUi_D3D_Image_FormatToResource(format);
	desc->SampleDesc.Count = 1u;
	desc->SampleDesc.Quality = 0u;
	desc->Flags = D3D12_RESOURCE_FLAG_NONE;
	if (type & abGPU_Image_Type_Renderable) {
		desc->Flags |= (abGPU_Image_Format_IsDepth(format) ?
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	}
	if (type & abGPU_Image_Type_Editable) {
		desc->Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
}

void abGPU_Image_GetMaxSize(abGPU_Image_Dimensions dimensions, unsigned int * wh, unsigned int * d) {
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
		maxD = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / 6u;
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

static unsigned int abGPUi_D3D_Image_CalculateMemoryUsageForDesc(D3D12_RESOURCE_DESC const * desc) {
	unsigned int subresourceCount = desc->MipLevels;
	if (desc->Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
		subresourceCount *= desc->DepthOrArraySize;
	}
	if (desc->Format == DXGI_FORMAT_R24G8_TYPELESS || desc->Format == DXGI_FORMAT_R32G8X24_TYPELESS) {
		subresourceCount *= 2u; // Depth and stencil planes.
	}
	uint64_t memoryUsage;
	ID3D12Device_GetCopyableFootprints(abGPUi_D3D_Device, desc, 0u, subresourceCount,
			0ull, abNull, abNull, abNull, &memoryUsage);
	return (unsigned int) memoryUsage;
}

unsigned int abGPU_Image_CalculateMemoryUsage(abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format) {
	D3D12_RESOURCE_DESC desc;
	abGPUi_D3D_Image_FillTextureDesc(type, dimensions, w, h, d, mips, format, &desc);
	return abGPUi_D3D_Image_CalculateMemoryUsageForDesc(&desc);
}

static void abGPUi_D3D_Image_GetDataLayout(D3D12_RESOURCE_DESC const * desc, DXGI_FORMAT * format,
		unsigned int * mipOffset, unsigned int * mipRowStride, unsigned int * layerStride) {
	unsigned int mips = desc->MipLevels;
	bool isArray = (desc->Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D && desc->DepthOrArraySize > 1u);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[D3D12_REQ_MIP_LEVELS + 1u];
	ID3D12Device_GetCopyableFootprints(abGPUi_D3D_Device, desc, 0u, mips + isArray,
			0ull, footprints, abNull, abNull, abNull);
	*format = footprints[0u].Footprint.Format;
	for (unsigned int mip = 0u; mip < mips; ++mip) {
		mipOffset[mip] = (unsigned int) footprints[mip].Offset;
		mipRowStride[mip] = footprints[mip].Footprint.RowPitch;
	}
	*layerStride = (isArray ? (unsigned int) (footprints[mips].Offset - footprints[0u].Offset) : 0u);
}

bool abGPU_Image_Init(abGPU_Image * image, abGPU_Image_Type type, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format,
		abGPU_Image_Usage initialUsage, abGPU_Image_Texel const * clearValue) {
	if (type & abGPU_Image_Type_Upload) {
		type = abGPU_Image_Type_Upload;
		initialUsage = abGPU_Image_Usage_Upload;
	}

	abGPU_Image_ClampSizeToMax(dimensions, &w, &h, &d, &mips);

	D3D12_RESOURCE_DESC desc;
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
	abGPUi_D3D_Image_GetDataLayout(&desc, &image->i_copyFormat,
			image->i_mipOffset, image->i_mipRowStride, &image->i_layerStride);

	D3D12_HEAP_PROPERTIES heapProperties = { 0 };
	D3D12_CLEAR_VALUE optimizedClearValue, * optimizedClearValuePointer = abNull;
	if (type & abGPU_Image_Type_Upload) {
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = image->memoryUsage;
		desc.Height = 1u;
		desc.DepthOrArraySize = 1u;
		desc.MipLevels = 1u;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1u;
		desc.SampleDesc.Quality = 0u;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	} else {
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		if (clearValue != abNull) {
			if (abGPU_Image_Format_IsDepth(format)) {
				optimizedClearValue.Format = abGPUi_D3D_Image_FormatToDepthStencil(format);
				optimizedClearValue.DepthStencil.Depth = clearValue->ds.depth;
				optimizedClearValue.DepthStencil.Stencil = clearValue->ds.stencil;
			} else {
				optimizedClearValue.Format = desc.Format;
				optimizedClearValue.Color[0u] = clearValue->color[0u];
				optimizedClearValue.Color[1u] = clearValue->color[1u];
				optimizedClearValue.Color[2u] = clearValue->color[2u];
				optimizedClearValue.Color[3u] = clearValue->color[3u];
			}
			optimizedClearValuePointer = &optimizedClearValue;
		}
	}
	return SUCCEEDED(ID3D12Device_CreateCommittedResource(abGPUi_D3D_Device,
			&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, abGPUi_D3D_Image_UsageToStates(initialUsage),
			optimizedClearValuePointer, &IID_ID3D12Resource, &image->i_resource)) ? true : false;
}

bool abGPU_Image_RespecifyUploadBuffer(abGPU_Image * image, abGPU_Image_Dimensions dimensions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format) {
	abGPU_Image_ClampSizeToMax(dimensions, &w, &h, &d, &mips);
	D3D12_RESOURCE_DESC desc;
	abGPUi_D3D_Image_FillTextureDesc(abGPU_Image_Type_Upload, dimensions, w, h, d, mips, format, &desc);
	if (desc.Format == DXGI_FORMAT_UNKNOWN || abGPUi_D3D_Image_CalculateMemoryUsageForDesc(&desc) > image->memoryUsage) {
		return false;
	}
	image->w = w;
	image->h = h;
	image->d = d;
	image->mips = mips;
	image->format = format;
	abGPUi_D3D_Image_GetDataLayout(&desc, &image->i_copyFormat,
			image->i_mipOffset, image->i_mipRowStride, &image->i_layerStride);
	return true;
}

void * abGPU_Image_UploadBegin(abGPU_Image * image, abGPU_Image_Slice slice) {
	if (!(image->typeAndDimensions & abGPU_Image_Type_Upload)) {
		return abNull;
	}
	void * mapping;
	return SUCCEEDED(ID3D12Resource_Map(image->i_resource,
			abGPUi_D3D_Image_SliceToSubresource(image, slice), abNull, &mapping)) ? mapping : abNull;
}

void abGPU_Image_Upload(abGPU_Image * image, abGPU_Image_Slice slice,
		unsigned int x, unsigned int y, unsigned int z, unsigned int w, unsigned int h, unsigned int d,
		unsigned int yStride, unsigned int zStride, void * mapping, void const * data) {
	if (!(image->typeAndDimensions & abGPU_Image_Type_Upload)) {
		return;
	}

	unsigned int mip = abGPU_Image_SliceMip(slice), mipW, mipH, mipD;
	abGPU_Image_GetMipSize(image, mip, &mipW, &mipH, &mipD);
	x = abMin(x, mipW);
	w = abMin(w, mipW - x);
	y = abMin(y, mipH);
	h = abMin(h, mipH - y);
	if (abGPU_Image_Dimensions_Are3D(abGPU_Image_GetDimensions(image))) {
		z = abMin(z, mipD);
		d = abMin(d, mipD - z);
	} else {
		z = 0u;
		d = 1u;
	}
	if (w == 0u || h == 0u || d == 0u) {
		return;
	}

	unsigned int mappedSubresource;
	if (mapping == abNull) {
		mappedSubresource = abGPUi_D3D_Image_SliceToSubresource(image, slice);
		if (FAILED(ID3D12Resource_Map(image->i_resource, mappedSubresource, abNull, &mapping))) {
			return;
		}
	} else {
		mappedSubresource = UINT_MAX;
	}

	if (abGPU_Image_Format_Is4x4(image->format)) {
		x >>= 2u;
		y >>= 2u;
		w = abAlign(w, 4u) >> 2u;
		h = abAlign(h, 4u) >> 2u;
		mipH = abAlign(mipH, 4u) >> 2u;
		// 3D compressed formats are not supported (yet) - not doing anything in this case.
	}

	unsigned int formatSize = abGPU_Image_Format_GetSize(image->format);
	unsigned int rowLength = w * formatSize;
	unsigned int targetYStride = image->i_mipRowStride[mip];
	unsigned int targetZStride = mipH * targetYStride;
	unsigned int targetOffset = z * targetZStride + y * targetYStride + x * formatSize;

	for (unsigned int uploadZ = 0u; uploadZ < d; ++uploadZ) {
		uint8_t const * source = (uint8_t const *) data + uploadZ * zStride;
		uint8_t * target = (uint8_t *) mapping + targetOffset + uploadZ * targetZStride;
		for (unsigned int uploadY = 0u; uploadY < h; ++uploadY) {
			memcpy(target, source, rowLength);
			source += yStride;
			target += targetYStride;
		}
	}

	if (mappedSubresource != UINT_MAX) {
		D3D12_RANGE writtenRange;
		writtenRange.Begin = targetOffset;
		writtenRange.End = targetOffset + (d - 1u) * targetZStride + (h - 1u) * targetYStride + rowLength;
		ID3D12Resource_Unmap(image->i_resource, mappedSubresource, &writtenRange);
	}
}

void abGPU_Image_UploadEnd(abGPU_Image * image, abGPU_Image_Slice slice,
		void * mapping, unsigned int const writtenOffsetAndSize[2]) {
	if (!(image->typeAndDimensions & abGPU_Image_Type_Upload)) {
		return;
	}
	D3D12_RANGE writtenRange;
	if (writtenOffsetAndSize != abNull) {
		writtenRange.Begin = writtenOffsetAndSize[0u];
		writtenRange.End = writtenOffsetAndSize[0u] + writtenOffsetAndSize[1u];
	}
	ID3D12Resource_Unmap(image->i_resource, abGPUi_D3D_Image_SliceToSubresource(image, slice),
			writtenOffsetAndSize != abNull ? &writtenRange : abNull);
}

void abGPU_Image_Destroy(abGPU_Image * image) {
	ID3D12Resource_Release(image->i_resource);
}

#endif
