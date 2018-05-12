#include "abFile_IQM.h"
#include "../abMath/abVec.h"
// Benchmarking.
#include "../abFeedback/abFeedback.h"
#include "../abPlatform/abPlatform.h"
#include <stdlib.h>

char const abFile_IQM_Header_Identifier[16u] = "INTERQUAKEMODEL";

unsigned int abFile_IQM_VertexDataFormat_ComponentSize(abFile_IQM_VertexDataFormat format) {
	switch (format) {
	case abFile_IQM_VertexDataFormat_SInt8:
	case abFile_IQM_VertexDataFormat_UInt8:
		return 1u;
	case abFile_IQM_VertexDataFormat_SInt16:
	case abFile_IQM_VertexDataFormat_UInt16:
	case abFile_IQM_VertexDataFormat_Float16:
		return 2u;
	case abFile_IQM_VertexDataFormat_SInt32:
	case abFile_IQM_VertexDataFormat_UInt32:
	case abFile_IQM_VertexDataFormat_Float32:
		return 4u;
	// case abFile_IQM_VertexDataFormat_Float64: // Not supported.
		// return 8u;
	}
	return 0u;
}

char const * abFile_IQM_ErrorText(abFile_IQM_Error error) {
	switch (error) {
	case abFile_IQM_Error_None: return abNull;
	case abFile_IQM_Error_SizeInvalid: return "IQM size invalid.";
	case abFile_IQM_Error_HeaderWrong: return "IQM header identification or version incorrect.";
	case abFile_IQM_Error_HeaderOffsetInvalid: return "IQM header contains invalid section offsets or sizes.";
	case abFile_IQM_Error_TextCorrupt: return "IQM text section doesn't start with an empty string or isn't terminated.";
	case abFile_IQM_Error_PartOffsetInvalid: return "IQM parts contain invalid offsets or sizes.";
	case abFile_IQM_Error_VertexCountTooBig: return "IQM contains too many vertices.";
	case abFile_IQM_Error_VertexDataTypeInvalid: return "IQM contains vertex data of unknown types.";
	case abFile_IQM_Error_VertexDataFormatUnsupported: return "IQM contains vertex data with unsupported format.";
	case abFile_IQM_Error_VertexDataOffsetInvalid: return "IQM vertex data contains invalid offsets or sizes.";
	case abFile_IQM_Error_TriangleCountTooBig: return "IQM contains too many triangles.";
	case abFile_IQM_Error_TriangleIndexOutOfRange: return "IQM contains out of range vertex indices.";
	}
	return "IQM unknown error.";
}

static abFile_IQM_Error abFilei_IQM_ValidateVertexData(abFile_IQM_VertexData const * vertexData, uint32_t elementCount,
		uint32_t vertexCount, uint32_t iqmSize, uint32_t textSize) {
	// The last check is needed so stream size can be safely calculated from vertex count (max 4 components of at most 4 bytes).
	if (vertexCount > (UINT32_MAX >> 4u)) { return abFile_IQM_Error_VertexCountTooBig; }
	abFile_IQM_VertexDataType lastType = (abFile_IQM_VertexDataType) 0u;
	for (uint32_t elementIndex = 0u; elementIndex < elementCount; ++elementIndex) {
		abFile_IQM_VertexData const * element = &vertexData[elementIndex];
		if (elementIndex != 0u && element->type <= lastType) { return abFile_IQM_Error_VertexDataTypeInvalid; }
		lastType = element->type;
		switch (element->type) {
		case abFile_IQM_VertexDataType_Position:
			if (element->format != abFile_IQM_VertexDataFormat_Float32 || element->size < 3u) { return abFile_IQM_Error_VertexDataFormatUnsupported; }
			break;
		case abFile_IQM_VertexDataType_TexCoord:
			if (element->format != abFile_IQM_VertexDataFormat_Float32 || element->size != 2u) { return abFile_IQM_Error_VertexDataFormatUnsupported; }
			break;
		case abFile_IQM_VertexDataType_Normal:
			if ((element->format != abFile_IQM_VertexDataFormat_Float32 && element->format != abFile_IQM_VertexDataFormat_SInt16) ||
					element->size < 3u) { return abFile_IQM_Error_VertexDataFormatUnsupported; }
			break;
		case abFile_IQM_VertexDataType_Tangent:
			if ((element->format != abFile_IQM_VertexDataFormat_Float32 && element->format != abFile_IQM_VertexDataFormat_SInt16) ||
					element->size < 4u) { return abFile_IQM_Error_VertexDataFormatUnsupported; }
			break;
		case abFile_IQM_VertexDataType_BlendIndices:
		case abFile_IQM_VertexDataType_BlendWeights:
		case abFile_IQM_VertexDataType_Color:
			if (element->format != abFile_IQM_VertexDataFormat_UInt8 || element->size != 4u) { return abFile_IQM_Error_VertexDataFormatUnsupported; }
			break;
		default:
			if (element->type < abFile_IQM_VertexDataType_CustomNameOffset ||
					(element->type - abFile_IQM_VertexDataType_CustomNameOffset) >= textSize) { return abFile_IQM_Error_VertexDataTypeInvalid; }
			break;
		}
		if (element->size > 4u) { return abFile_IQM_Error_VertexDataFormatUnsupported; }
		uint32_t vertexSize = element->size * abFile_IQM_VertexDataFormat_ComponentSize(element->format);
		// Non-4-aligned elements not supported.
		if (vertexSize == 0u || (vertexSize & 3u)) { return abFile_IQM_Error_VertexDataFormatUnsupported; }
		if ((element->offset & 3u) || element->offset > iqmSize ||
				(vertexCount * vertexSize) > (iqmSize - element->offset)) { return abFile_IQM_Error_VertexDataOffsetInvalid; }
	}
	return abFile_IQM_Error_None;
}

static abFile_IQM_Error abFilei_IQM_ValidateTriangles(uint32_t const * triangles, uint32_t triangleCount, uint32_t vertexCount) {
	if (triangleCount == 0u) {
		return abFile_IQM_Error_None;
	}
	if (triangleCount > (UINT32_MAX / 3u)) {
		return abFile_IQM_Error_TriangleCountTooBig;
	}
	if (vertexCount == 0u) {
		return abFile_IQM_Error_TriangleIndexOutOfRange;
	}
	uint32_t indexCount = triangleCount * 3u;
	while (((size_t) triangles & 15u) && indexCount != 0u) {
		if (*(triangles++) >= vertexCount) {
			return abFile_IQM_Error_TriangleIndexOutOfRange;
		}
		--indexCount;
	}
	if (indexCount >= 16u) { // Small values are very unlikely though. An arbitrary value based on operation count.
		#ifdef abVec4u_Comparison_Available
		abVec4u32 maxIndices = abVec4u32_LoadX4(vertexCount - 1u);
		abVec4u32 anyOutside = abVec4u32_Zero;
		while (indexCount >= 4u) {
			anyOutside = abVec4u32_Or(anyOutside, abVec4u32_Greater(abVec4u32_LoadAligned(triangles), maxIndices));
			triangles += 4u;
			indexCount -= 4u;
		}
		anyOutside = abVec4u32_Or(anyOutside, abVec4u32_ZWXY(anyOutside));
		anyOutside = abVec4u32_Or(anyOutside, abVec4u32_YXWZ(anyOutside));
		if (abVec4u32_GetX(anyOutside) != 0u) {
			return abFile_IQM_Error_TriangleIndexOutOfRange;
		}
		#else
		abVec4s32 maxIndices = abVec4s32_LoadX4((int32_t) vertexCount - 1);
		abVec4s32 anyOutside = abVec4s32_Zero;
		abVec4s32 const zeros = abVec4s32_Zero;
		while (indexCount >= 4u) {
			abVec4s32 fourIndices = abVec4s32_LoadAligned((int32_t const *) triangles);
			anyOutside = abVec4s32_Or(anyOutside, abVec4s32_Less(fourIndices, zeros));
			anyOutside = abVec4s32_Or(anyOutside, abVec4s32_Greater(fourIndices, maxIndices));
			triangles += 4u;
			indexCount -= 4u;
		}
		anyOutside = abVec4s32_Or(anyOutside, abVec4s32_ZWXY(anyOutside));
		anyOutside = abVec4s32_Or(anyOutside, abVec4s32_YXWZ(anyOutside));
		if (abVec4s32_GetX(anyOutside) != 0) {
			return abFile_IQM_Error_TriangleIndexOutOfRange;
		}
		#endif
	}
	while (indexCount != 0u) {
		if (*(triangles++) >= vertexCount) {
			return abFile_IQM_Error_TriangleIndexOutOfRange;
		}
		--indexCount;
	}
	return abFile_IQM_Error_None;
}

abFile_IQM_Error abFile_IQM_Validate(void const * fileData, size_t fileSize) {
	if (fileSize < sizeof(abFile_IQM_Header)) {
		return abFile_IQM_Error_SizeInvalid;
	}

	// Basic file information.
	abFile_IQM_Header header = *((abFile_IQM_Header const *) fileData); // For less random access.
	if (memcmp(header.identifier, abFile_IQM_Header_Identifier, sizeof(header.identifier)) != 0 || header.version != 2u) {
		return abFile_IQM_Error_HeaderWrong;
	}
	uint32_t iqmSize = header.fileSize;
	if (iqmSize > fileSize) {
		return abFile_IQM_Error_SizeInvalid;
	}
	uint8_t const * iqmData = fileData;

	// Top-level section ranges.
	if (header.textOffset > iqmSize || header.textSize == 0u || header.textSize > (iqmSize - header.textOffset)) {
		return abFile_IQM_Error_HeaderOffsetInvalid;
	}
	if (header.partOffset > iqmSize || (header.partOffset & 3u) || header.partCount > ((iqmSize - header.partOffset) / sizeof(abFile_IQM_Part))) {
		return abFile_IQM_Error_HeaderOffsetInvalid;
	}
	if (header.vertexDataOffset > iqmSize || (header.vertexDataOffset & 3u) ||
			header.vertexDataCount > ((iqmSize - header.vertexDataOffset) / sizeof(abFile_IQM_VertexData))) {
		return abFile_IQM_Error_HeaderOffsetInvalid;
	}
	if (header.triangleOffset > iqmSize || (header.triangleOffset & 3u) ||
			header.triangleCount > ((iqmSize - header.triangleOffset) / sizeof(uint32_t))) {
		return abFile_IQM_Error_HeaderOffsetInvalid;
	}

	// Text - there must be a terminator somewhere.
	if (iqmData[header.textOffset] != '\0' || iqmData[header.textOffset + header.textSize - 1u] != '\0') {
		return abFile_IQM_Error_TextCorrupt;
	}

	abFile_IQM_Error error;

	// Parts.
	abFile_IQM_Part const * parts = (abFile_IQM_Part const *) (iqmData + header.partOffset);
	for (unsigned int partIndex = 0u; partIndex < header.partCount; ++partIndex) {
		abFile_IQM_Part const * part = &parts[partIndex];
		if (part->name >= header.textSize || part->material >= header.textSize ||
				part->vertexFirst > header.vertexCount || part->vertexCount > (header.vertexCount - part->vertexFirst) ||
				part->triangleFirst > header.triangleCount || part->triangleCount > (header.triangleCount - part->triangleFirst)) {
			return abFile_IQM_Error_PartOffsetInvalid;
		}
	}

	// Vertex data.
	error = abFilei_IQM_ValidateVertexData((abFile_IQM_VertexData const *) (iqmData + header.vertexDataOffset),
			header.vertexDataCount, header.vertexCount, iqmSize, header.textSize);
	if (error != abFile_IQM_Error_None) {
		return error;
	}

	// Triangles.
	error = abFilei_IQM_ValidateTriangles((uint32_t const *) (iqmData + header.triangleOffset), header.triangleCount, header.vertexCount);
	if (error != abFile_IQM_Error_None) {
		return error;
	}

	return abFile_IQM_Error_None;
}
