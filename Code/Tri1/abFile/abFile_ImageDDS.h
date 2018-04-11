#ifndef abInclude_abFile_ImageDDS
#define abInclude_abFile_ImageDDS
#include "../abGPU/abGPU.h"

typedef uint32_t abFile_ImageDDS_Flags;
enum {
	abFile_ImageDDS_Flags_Caps = 1u << 0u,
	abFile_ImageDDS_Flags_Height = 1u << 1u,
	abFile_ImageDDS_Flags_Width = 1u << 2u,
	abFile_ImageDDS_Flags_Pitch = 1u << 3u,
	abFile_ImageDDS_Flags_Format = 1u << 12u,
	abFile_ImageDDS_Flags_MipMapCount = 1u << 17u,
	abFile_ImageDDS_Flags_LinearSize = 1u << 19u,
	abFile_ImageDDS_Flags_Depth = 1u << 23u
};

typedef uint32_t abFile_ImageDDS_FormatFlags;
enum {
	abFile_ImageDDS_FormatFlags_AlphaPixels = 1u << 0u,
	abFile_ImageDDS_FormatFlags_Alpha = 1u << 1u,
	abFile_ImageDDS_FormatFlags_FourCC = 1u << 2u,
	abFile_ImageDDS_FormatFlags_RGB = 1u << 6u,
	abFile_ImageDDS_FormatFlags_YUV = 1u << 9u,
	abFile_ImageDDS_FormatFlags_Luminance = 1u << 17u
};

typedef uint32_t abFile_ImageDDS_Caps;
enum {
	abFile_ImageDDS_Caps_Complex = 1u << 3u,
	abFile_ImageDDS_Caps_Mipmap = 1u << 22u,
	abFile_ImageDDS_Caps_Texture = 1u << 12u
};

typedef uint32_t abFile_ImageDDS_Caps2;
enum {
	abFile_ImageDDS_Caps2_Cubemap = 1u << 9u,
	abFile_ImageDDS_Caps2_CubemapXP = 1u << 10u,
	abFile_ImageDDS_Caps2_CubemapXN = 1u << 11u,
	abFile_ImageDDS_Caps2_CubemapYP = 1u << 12u,
	abFile_ImageDDS_Caps2_CubemapYN = 1u << 13u,
	abFile_ImageDDS_Caps2_CubemapZP = 1u << 14u,
	abFile_ImageDDS_Caps2_CubemapZN = 1u << 15u,
	abFile_ImageDDS_Caps2_Volume = 1u << 21u
};

typedef struct abFile_ImageDDS_Header {
	uint32_t headerSize;
	abFile_ImageDDS_Flags flags;
	uint32_t height;
	uint32_t width;
	uint32_t pitchOrLinearSize;
	uint32_t depth;
	uint32_t mipMapCount;
	uint32_t reserved[11u];
	uint32_t formatStructSize;
	abFile_ImageDDS_FormatFlags formatFlags;
	uint32_t formatFourCC;
	uint32_t formatRGBBitCount;
	uint32_t formatBitMasks[4u];
	abFile_ImageDDS_Caps caps;
	abFile_ImageDDS_Caps2 caps2;
	uint32_t caps3;
	uint32_t caps4;
	uint32_t reserved2;
} abFile_ImageDDS_Header;

typedef struct abFile_ImageDDS_HeaderDXT10 {
	uint32_t dxgiFormat;
	uint32_t dimension;
	uint32_t miscFlags;
	uint32_t arraySize;
	uint32_t miscFlags2;
} abFile_ImageDDS_HeaderDXT10;

// All outputs are optional. The data is an array (for cubemaps, 6 times as many elements) of mips, with unaligned rows.
abBool abFile_ImageDDS_Read(void const * dds, uint32_t ddsSize, abGPU_Image_Options * dimensionOptions,
		unsigned int * w, unsigned int * h, unsigned int * d, unsigned int * mips,
		abGPU_Image_Format * format, unsigned int * dataOffset);

#endif
