#ifndef abInclude_abFile_IQM
#define abInclude_abFile_IQM
#include "../abCommon.h"

typedef struct abFile_IQM_Header {
	char identifier[16u];
	uint32_t version;
	uint32_t fileSize;
	uint32_t flags;
	uint32_t textSize, textOffset;
	uint32_t partCount, partOffset;
	uint32_t vertexDataCount, vertexCount, vertexDataOffset;
	uint32_t triangleCount, triangleOffset, adjacencyOffset;
	uint32_t jointCount, jointOffset;
	uint32_t poseCount, poseOffset;
	uint32_t animationCount, animationOffset;
	uint32_t frameCount, frameChannelCount, frameOffset, boundsOffset;
	uint32_t commentSize, commentOffset;
	uint32_t extensionCount, extensionOffset;
} abFile_IQM_Header;

char const abFile_IQM_Header_Identifier[16u];

typedef struct abFile_IQM_Part {
	uint32_t name;
	uint32_t material;
	uint32_t vertexFirst, vertexCount;
	uint32_t triangleFirst, triangleCount;
} abFile_IQM_Part;

typedef uint32_t abFile_IQM_VertexDataType;
enum {
	abFile_IQM_VertexDataType_Position, // By default float32x3.
	abFile_IQM_VertexDataType_TexCoord, // Float32x2.
	abFile_IQM_VertexDataType_Normal, // Float32x3.
	abFile_IQM_VertexDataType_Tangent, // Float32x4.
	abFile_IQM_VertexDataType_BlendIndices, // UInt8x4.
	abFile_IQM_VertexDataType_BlendWeights, // UNorm8x4.
	abFile_IQM_VertexDataType_Color, // UNorm8x4.
		abFile_IQM_VertexDataType_CustomNameOffset = 16u
};

typedef uint32_t abFile_IQM_VertexDataFormat;
enum {
	abFile_IQM_VertexDataFormat_SInt8,
	abFile_IQM_VertexDataFormat_UInt8,
	abFile_IQM_VertexDataFormat_SInt16,
	abFile_IQM_VertexDataFormat_UInt16,
	abFile_IQM_VertexDataFormat_SInt32,
	abFile_IQM_VertexDataFormat_UInt32,
	abFile_IQM_VertexDataFormat_Float16,
	abFile_IQM_VertexDataFormat_Float32,
	abFile_IQM_VertexDataFormat_Float64
};

typedef struct abFile_IQM_VertexData {
	abFile_IQM_VertexDataType type;
	uint32_t flags;
	abFile_IQM_VertexDataFormat format;
	uint32_t size;
	uint32_t offset;
} abFile_IQM_VertexData;

abBool abFile_IQM_Validate(void const * fileData, size_t fileSize);

#endif
