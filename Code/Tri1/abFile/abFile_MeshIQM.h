#ifndef abInclude_abFile_MeshIQM
#define abInclude_abFile_MeshIQM
#include "../abCommon.h"

typedef struct abFile_MeshIQM_Header {
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
} abFile_MeshIQM_Header;

typedef struct abFile_MeshIQM_Part {
	uint32_t name;
	uint32_t material;
	uint32_t vertexFirst, vertexCount;
	uint32_t triangleFirst, triangleCount;
} abFile_MeshIQM_Part;

typedef uint32_t abFile_MeshIQM_VertexDataType;
enum {
	abFile_MeshIQM_VertexDataType_Position, // By default float32x3.
	abFile_MeshIQM_VertexDataType_TexCoord, // Float32x2.
	abFile_MeshIQM_VertexDataType_Normal, // Float32x3.
	abFile_MeshIQM_VertexDataType_Tangent, // Float32x4.
	abFile_MeshIQM_VertexDataType_BlendIndices, // UInt8x4.
	abFile_MeshIQM_VertexDataType_BlendWeights, // UNorm8x4.
	abFile_MeshIQM_VertexDataType_Color, // UNorm8x4.
		abFile_MeshIQM_VertexDataType_CustomNameOffset = 16u
};

typedef uint32_t abFile_MeshIQM_VertexDataFormat;
enum {
	abFile_MeshIQM_VertexDataFormat_SInt8,
	abFile_MeshIQM_VertexDataFormat_UInt8,
	abFile_MeshIQM_VertexDataFormat_SInt16,
	abFile_MeshIQM_VertexDataFormat_UInt16,
	abFile_MeshIQM_VertexDataFormat_SInt32,
	abFile_MeshIQM_VertexDataFormat_UInt32,
	abFile_MeshIQM_VertexDataFormat_Float16,
	abFile_MeshIQM_VertexDataFormat_Float32,
	abFile_MeshIQM_VertexDataFormat_Float64
};

typedef struct abFile_MeshIQM_VertexData {
	abFile_MeshIQM_VertexDataType type;
	uint32_t flags;
	abFile_MeshIQM_VertexDataFormat format;
	uint32_t size;
	uint32_t offset;
};

#endif
