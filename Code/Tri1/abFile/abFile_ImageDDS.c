#include "abFile_ImageDDS.h"

static abGPU_Image_Format const abFilei_ImageDDS_DXGIFormatToImageFormatMap[abFile_ImageDDS_DXGIFormat_CountKnown] = {
	[abFile_ImageDDS_DXGIFormat_R8G8B8A8_UNorm] = abGPU_Image_Format_R8G8B8A8,
	[abFile_ImageDDS_DXGIFormat_R8G8B8A8_UNorm_sRGB] = abGPU_Image_Format_R8G8B8A8_sRGB,
	[abFile_ImageDDS_DXGIFormat_R8G8B8A8_SNorm] = abGPU_Image_Format_R8G8B8A8_Signed,
	[abFile_ImageDDS_DXGIFormat_R8G8_UNorm] = abGPU_Image_Format_R8G8,
	[abFile_ImageDDS_DXGIFormat_R8_UNorm] = abGPU_Image_Format_R8,
	[abFile_ImageDDS_DXGIFormat_BC1_UNorm] = abGPU_Image_Format_S3TC_A1,
	[abFile_ImageDDS_DXGIFormat_BC1_UNorm_sRGB] = abGPU_Image_Format_S3TC_A1_sRGB,
	[abFile_ImageDDS_DXGIFormat_BC2_UNorm] = abGPU_Image_Format_S3TC_A4,
	[abFile_ImageDDS_DXGIFormat_BC2_UNorm_sRGB] = abGPU_Image_Format_S3TC_A4_sRGB,
	[abFile_ImageDDS_DXGIFormat_BC3_UNorm] = abGPU_Image_Format_S3TC_A8,
	[abFile_ImageDDS_DXGIFormat_BC3_UNorm_sRGB] = abGPU_Image_Format_S3TC_A8_sRGB,
	[abFile_ImageDDS_DXGIFormat_BC4_UNorm] = abGPU_Image_Format_3Dc_X,
	[abFile_ImageDDS_DXGIFormat_BC4_SNorm] = abGPU_Image_Format_3Dc_X_Signed,
	[abFile_ImageDDS_DXGIFormat_BC5_UNorm] = abGPU_Image_Format_3Dc_XY,
	[abFile_ImageDDS_DXGIFormat_BC5_SNorm] = abGPU_Image_Format_3Dc_XY_Signed,
	[abFile_ImageDDS_DXGIFormat_B5G6R5_UNorm] = abGPU_Image_Format_B5G6R5,
	[abFile_ImageDDS_DXGIFormat_B5G5R5A1_UNorm] = abGPU_Image_Format_B5G5R5A1,
	[abFile_ImageDDS_DXGIFormat_B8G8R8A8_UNorm] = abGPU_Image_Format_B8G8R8A8,
	[abFile_ImageDDS_DXGIFormat_B8G8R8A8_UNorm_sRGB] = abGPU_Image_Format_B8G8R8A8_sRGB
};

abBool abFile_ImageDDS_Read(void const * dds, size_t ddsSize, abGPU_Image_Options * dimensionOptions,
		unsigned int * w, unsigned int * h, unsigned int * d, unsigned int * mips,
		abGPU_Image_Format * format, unsigned int * dataOffset) {
	// Header.
	if (ddsSize < (sizeof(uint32_t) + sizeof(abFile_ImageDDS_Header))) {
		return abFalse;
	}
	if (*((uint32_t const *) dds) != 0x20534444u) {
		return abFalse;
	}
	abFile_ImageDDS_Header const * header = (abFile_ImageDDS_Header const *) ((uint32_t *) dds + 1u);
	if (header->headerSize != sizeof(abFile_ImageDDS_Header)) {
		return abFalse;
	}

	// Format.
	abGPU_Image_Format imageFormat;
	if (header->formatStructSize != 32u) {
		return abFalse;
	}
	if (header->formatFlags & (abFile_ImageDDS_FormatFlags_YUV | abFile_ImageDDS_FormatFlags_Luminance)) {
		return abFalse;
	}
	uint32_t formatFourCC = ((header->formatFlags & abFile_ImageDDS_FormatFlags_FourCC) ? header->formatFourCC : 0u);
	abFile_ImageDDS_HeaderDXT10 const * header10;
	if (formatFourCC == 0x30315844u) {
		if (ddsSize < (sizeof(uint32_t) + sizeof(abFile_ImageDDS_Header) + sizeof(abFile_ImageDDS_HeaderDXT10))) {
			return abFalse;
		}
		header10 = (abFile_ImageDDS_HeaderDXT10 const *) (header + 1u);
	} else {
		header10 = abNull;
	}
	if (header10 != abNull) {
		if (header10->dxgiFormat >= abArrayLength(abFilei_ImageDDS_DXGIFormatToImageFormatMap)) {
			return abFalse;
		}
		imageFormat = abFilei_ImageDDS_DXGIFormatToImageFormatMap[header10->dxgiFormat];
		if (imageFormat == abGPU_Image_Format_Invalid) {
			return abFalse;
		}
	} else if (formatFourCC != 0u) {
		switch (formatFourCC) {
		case 0x31545844u:
			imageFormat = abGPU_Image_Format_S3TC_A1; break;
		case 0x32545844u:
		case 0x33545844u:
			imageFormat = abGPU_Image_Format_S3TC_A4; break;
		case 0x34545844u:
		case 0x35545844u:
			imageFormat = abGPU_Image_Format_S3TC_A8; break;
		default:
			return abFalse;
		}
	} else {
		if (!(header->formatFlags & abFile_ImageDDS_FormatFlags_RGB)) {
			return abFalse; // A8 is not supported in all graphics APIs.
		}
		uint32_t r = header->formatBitMasks[0u], g = header->formatBitMasks[1u], b = header->formatBitMasks[2u];
		uint32_t a = ((header->formatFlags & abFile_ImageDDS_FormatFlags_AlphaPixels) ? header->formatBitMasks[3u] : 0u);
		switch (header->formatRGBBitCount) {
		case 32u:
			if (r == 0x000000ffu && g == 0x0000ff00u && b == 0x00ff0000u) {
				imageFormat = abGPU_Image_Format_R8G8B8A8;
			} else if (r == 0x00ff0000u && g == 0x0000ff00u && b == 0x000000ffu) {
				imageFormat = abGPU_Image_Format_B8G8R8A8;
			} else {
				return abFalse;
			}
			break;
		case 16u:
			if (a != 0u) {
				return abFalse;
			}
			if (r == 0x000000ffu && g == 0x0000ff00u) {
				imageFormat = abGPU_Image_Format_R8G8;
			} else if (r == 0x00007c00u && g == 0x000003e0u && b == 0x0000001fu) {
				imageFormat = abGPU_Image_Format_B5G5R5A1;
			} else if (r == 0x0000f800u && g == 0x000007e0u && b == 0x0000001fu) {
				imageFormat = abGPU_Image_Format_B5G6R5;
			} else {
				return abFalse;
			}
		case 8u:
			if (r == 0x000000ffu && a == 0u) {
				imageFormat = abGPU_Image_Format_R8;
			} else {
				return abFalse;
			}
			break;
		default:
			return abFalse;
		}
	}
	abBool formatIs4x4 = abGPU_Image_Format_Is4x4(imageFormat);

	// Dimensions.
	const abFile_ImageDDS_Flags requiredFlags = abFile_ImageDDS_Flags_Caps | abFile_ImageDDS_Flags_Height |
			abFile_ImageDDS_Flags_Width | abFile_ImageDDS_Flags_Format;
	if ((header->flags & requiredFlags) != requiredFlags) {
		return abFalse;
	}
	unsigned int height = abMax(header->height, 1u);
	unsigned int width = abMax(header->width, 1u);
	abBool is3D = ((header->caps2 & abFile_ImageDDS_Caps2_Volume) ? abTrue : abFalse);
	if (is3D && (formatIs4x4 || !(header->flags & abFile_ImageDDS_Flags_Depth))) {
		return abFalse;
	}
	unsigned int depth = (is3D ? abMax(header->depth, 1u) : 1u);
	unsigned int mipCount;
	if (header->caps & abFile_ImageDDS_Caps_Mipmap) {
		if (!(header->flags & abFile_ImageDDS_Flags_MipMapCount)) {
			return abFalse;
		}
		mipCount = abMax(header->mipMapCount, 1u);
		if (mipCount > abGPU_Image_CalculateMipCount(is3D ? abGPU_Image_Options_3D : abGPU_Image_Options_None, width, height, depth)) {
			return abFalse;
		}
	} else {
		mipCount = 1u;
	}
	abBool isCube = ((header->caps2 & abFile_ImageDDS_Caps2_Cubemap) ? abTrue : abFalse);
	if (isCube) {
		if (is3D) {
			return abFalse;
		}
		const abFile_ImageDDS_Caps2 requiredCubeCaps2 = abFile_ImageDDS_Caps2_CubemapXP | abFile_ImageDDS_Caps2_CubemapXN |
				abFile_ImageDDS_Caps2_CubemapYP | abFile_ImageDDS_Caps2_CubemapYN |
				abFile_ImageDDS_Caps2_CubemapZP | abFile_ImageDDS_Caps2_CubemapZN;
		if ((header->caps2 & requiredCubeCaps2) != requiredCubeCaps2) {
			return abFalse;
		}
	}
	if (header10 != abNull) {
		if (is3D) {
			if (header10->dimension != abFile_ImageDDS_Dimension_Texture3D || header10->arraySize > 1u) {
				return abFalse;
			}
		} else {
			if (header10->dimension != abFile_ImageDDS_Dimension_Texture1D && header10->dimension != abFile_ImageDDS_Dimension_Texture2D) {
				return abFalse;
			}
			depth = abMax(header10->arraySize, 1u);
		}
	}

	// Overflow prevention in size validation.
	if (is3D) {
		if (width > abGPU_Image_MaxAbsoluteSize3D || height > abGPU_Image_MaxAbsoluteSize3D || depth > abGPU_Image_MaxAbsoluteSize3D) {
			return abFalse;
		}
	} else {
		if (width > abGPU_Image_MaxAbsoluteSize2DCube || height > abGPU_Image_MaxAbsoluteSize2DCube ||
				depth > (isCube ? abGPU_Image_MaxAbsoluteArraySizeCube : abGPU_Image_MaxAbsoluteArraySize2D)) {
			return abFalse;
		}
	}

	// Size validation - DDS data is unaligned.
	uint32_t headersSize = sizeof(uint32_t) + sizeof(abFile_ImageDDS_Header);
	if (header10 != abNull) {
		headersSize += sizeof(abFile_ImageDDS_HeaderDXT10);
	}
	size_t dataSize = ddsSize - headersSize;
	unsigned int formatSize = abGPU_Image_Format_GetSize(imageFormat);
	uint64_t actualDataSize = 0ull;
	for (unsigned int mip = 0u; mip < mipCount; ++mip) {
		unsigned int mipW = abMax(width >> mip, 1u), mipH = abMax(height >> mip, 1u),
				mipD = (is3D ? abMax(depth >> mip, 1u) : 1u);
		if (formatIs4x4) {
			mipW = abAlign(mipW, 4u) >> 2u;
			mipH = abAlign(mipH, 4u) >> 2u;
		}
		actualDataSize += (uint64_t) mipD * mipH * mipW * formatSize;
	}
	actualDataSize *= (!is3D ? depth : 1u) * (isCube ? 6u : 1u);
	if (actualDataSize > dataSize) {
		return abFalse;
	}

	// Outputs.
	if (dimensionOptions != abNull) {
		if (is3D) {
			*dimensionOptions = abGPU_Image_Options_3D;
		} else {
			*dimensionOptions = (isCube ? abGPU_Image_Options_Cube : 0u) | (depth > 1u ? abGPU_Image_Options_Array : 0u);
		}
	}
	if (w != abNull) { *w = width; }
	if (h != abNull) { *h = height; }
	if (d != abNull) { *d = depth; }
	if (mips != abNull) { *mips = mipCount; }
	if (format != abNull) { *format = imageFormat; }
	if (dataOffset != abNull) { *dataOffset = headersSize; }
	return abTrue;
}
