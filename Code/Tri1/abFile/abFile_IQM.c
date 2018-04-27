#include "abFile_IQM.h"
#include "../abMath/abVec.h"
// Benchmarking.
#include "../abFeedback/abFeedback.h"
#include "../abPlatform/abPlatform.h"
#include <stdlib.h>

char const abFile_IQM_Header_Identifier[16u] = "INTERQUAKEMODEL";

static abBool abFilei_IQM_ValidateTriangles(uint32_t const * triangles, uint32_t triangleCount, uint32_t vertexCount) {
	if (triangleCount == 0u) {
		return abTrue;
	}
	// The last check is needed because when unsigned vector comparisons are unavailable, signed check will be done.
	if ((triangleCount % 3u) != 0u || triangleCount > (UINT32_MAX / 3u) || vertexCount == 0u || vertexCount > (uint32_t) INT32_MAX) {
		return abFalse;
	}
	uint32_t indexCount = triangleCount * 3u;
	while (((size_t) triangles & 15u) && indexCount != 0u) {
		if (*(triangles++) >= vertexCount) {
			return abFalse;
		}
		--indexCount;
	}
	if (indexCount >= 16u) { // Small values are very unlikely though. An arbitrary value based on operation count.
		#ifdef abVec4u32_ComparisonAvailable
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
			return abFalse;
		}
		#else
		abVec4s32 maxIndices = abVec4s32_LoadX4((int32_t) vertexCount - 1);
		abVec4s32 anyOutside = abVec4s32_Zero;
		while (indexCount >= 4u) {
			abVec4s32 fourIndices = abVec4s32_LoadAligned((int32_t const *) triangles);
			anyOutside = abVec4s32_Or(anyOutside, abVec4s32_Less(fourIndices, abVec4s32_Zero));
			anyOutside = abVec4s32_Or(anyOutside, abVec4s32_Greater(fourIndices, maxIndices));
			triangles += 4u;
			indexCount -= 4u;
		}
		anyOutside = abVec4s32_Or(anyOutside, abVec4s32_ZWXY(anyOutside));
		anyOutside = abVec4s32_Or(anyOutside, abVec4s32_YXWZ(anyOutside));
		if (abVec4s32_GetX(anyOutside) != 0) {
			return abFalse;
		}
		#endif
	}
	while (indexCount != 0u) {
		if (*(triangles++) >= vertexCount) {
			return abFalse;
		}
		--indexCount;
	}
	return abTrue;
}

abBool abFile_IQM_Validate(void const * fileData, size_t fileSize) {
	if (fileSize < sizeof(abFile_IQM_Header)) {
		return abFalse;
	}

	// Basic file information.
	abFile_IQM_Header header = *((abFile_IQM_Header const *) fileData); // For less random access.
	if (memcmp(header.identifier, abFile_IQM_Header_Identifier, sizeof(header.identifier)) != 0) { return abFalse; }
	if (header.version != 2u) { return abFalse; }
	uint32_t iqmSize = header.fileSize;
	if (iqmSize > fileSize) { return abFalse; }
	uint8_t const * iqmData = fileData;

	// Top-level structure ranges.
	if (header.textOffset > iqmSize || header.textSize == 0u || header.textSize > (iqmSize - header.textOffset)) { return abFalse; }
	if (header.partOffset > iqmSize || (header.partOffset & 3u) ||
			header.partCount > ((iqmSize - header.partOffset) / sizeof(abFile_IQM_Part))) { return abFalse; }
	if (header.vertexDataOffset > iqmSize || (header.vertexDataOffset & 3u) ||
			header.vertexDataCount > ((iqmSize - header.vertexDataOffset) / sizeof(abFile_IQM_VertexData))) { return abFalse; }
	if (header.triangleOffset > iqmSize || (header.triangleOffset & 3u) ||
			header.triangleCount > ((iqmSize - header.triangleOffset) / sizeof(uint32_t))) { return abFalse; }

	// Text - there must be a terminator somewhere.
	if (iqmData[header.textOffset] != '\0' || iqmData[header.textOffset + header.textSize - 1u] != '\0') { return abFalse; }

	// Parts.
	abFile_IQM_Part const * parts = (abFile_IQM_Part const *) (iqmData + header.partOffset);
	for (unsigned int partIndex = 0u; partIndex < header.partCount; ++partIndex) {
		abFile_IQM_Part const * part = &parts[partIndex];
		if (part->name >= header.textSize || part->material >= header.textSize ||
				part->vertexFirst > header.vertexCount || part->vertexCount > (header.vertexCount - part->vertexFirst) ||
				part->triangleFirst > header.triangleCount || part->triangleCount > (header.triangleCount - part->triangleFirst)) {
			return abFalse;
		}
	}

	// Triangles.
	if (!abFilei_IQM_ValidateTriangles((uint32_t const *) (iqmData + header.triangleOffset),
			header.triangleCount, header.vertexCount)) { return abFalse; }

	return abTrue;
}
