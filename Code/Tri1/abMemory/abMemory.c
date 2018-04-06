#include "abMemory.h"
#include "../abData/abText.h"
#include "../abFeedback/abFeedback.h"
#include <stdlib.h>

abParallel_Mutex abMemory_TagList_Mutex;
abMemory_Tag * abMemory_TagList_First, * abMemory_TagList_Last;

void abMemory_Init() {
	if (!abParallel_Mutex_Init(&abMemory_TagList_Mutex)) {
		abFeedback_Crash("abMemory_Init", "Failed to initialize the tag list mutex.");
	}
	abMemory_TagList_First = abMemory_TagList_Last = abNull;
}

abMemory_Tag * abMemory_Tag_Create(char const * name) {
	abMemory_Tag * tag = (abMemory_Tag *) malloc(sizeof(abMemory_Tag));
	if (tag == abNull) {
		abFeedback_Crash("abMemory_Tag_Init", "Failed to allocate memory for tag %s.", name);
	}
	if (!abParallel_Mutex_Init(&tag->mutex)) {
		abFeedback_Crash("abMemory_Tag_Init", "Failed to initialize the allocation list mutex for tag %s.", name);
	}
	abTextA_Copy(tag->name, abArrayLength(tag->name), name);
	tag->allocationFirst = tag->allocationLast = abNull;
	tag->totalAllocated = 0u;

	abParallel_Mutex_Lock(&abMemory_TagList_Mutex);
	tag->tagPrevious = abMemory_TagList_Last;
	tag->tagNext = abNull;
	if (abMemory_TagList_Last != abNull) {
		abMemory_TagList_Last->tagNext = tag;
	} else {
		abMemory_TagList_First = tag;
	}
	abMemory_TagList_Last = tag;
	abParallel_Mutex_Unlock(&abMemory_TagList_Mutex);

	return tag;
}

void abMemory_Tag_Destroy(abMemory_Tag * tag) {
	abParallel_Mutex_Lock(&abMemory_TagList_Mutex);
	if (tag->tagPrevious != abNull) {
		tag->tagPrevious->tagNext = tag->tagNext;
	} else {
		abMemory_TagList_First = tag->tagNext;
	}
	if (tag->tagNext != abNull) {
		tag->tagNext->tagPrevious = tag->tagPrevious;
	} else {
		abMemory_TagList_Last = tag->tagPrevious;
	}
	abParallel_Mutex_Unlock(&abMemory_TagList_Mutex);

	abParallel_Mutex_Lock(&tag->mutex);
	abMemory_Allocation * allocation, * nextAllocation;
	for (allocation = tag->allocationLast; allocation != abNull; allocation = nextAllocation) {
		nextAllocation = allocation->inTagPrevious;
		free(allocation);
	}
	abParallel_Mutex_Unlock(&tag->mutex);

	abParallel_Mutex_Destroy(&tag->mutex);
	free(tag); // Because it's created with Create, not Init, and Create does malloc.
}

void * abMemory_DoAlloc(abMemory_Tag * tag, size_t size, abBool align16, char const * fileName, unsigned int fileLine) {
	abMemory_Allocation * allocation;
	// Malloc gives 16-aligned blocks on 64-bit platforms, 8-aligned on 32-bit.
	// Also add some padding, for instance, for uint64_t/vec4 writes.
	#ifdef abPlatform_CPU_64Bit
	allocation = malloc(sizeof(abMemory_Allocation) + abAlign(size, (size_t) 16u));
	#else
	allocation = malloc(sizeof(abMemory_Allocation) + (align16 ? 8u : 0u) + abAlign(size, (size_t) 16u));
	#endif
	if (allocation == abNull) {
		abFeedback_Crash("abMemory_DoAlloc", "Failed to allocate %zu bytes with tag %s at %s:%u.", size, tag->name, fileName, fileLine);
	}
	allocation->tag = tag;
	allocation->size = size;
	allocation->fileName = fileName;
	allocation->fileLine = (uint16_t) fileLine;
	allocation->locationMark = (align16 ? abMemory_Allocation_LocationMark_Here16 : abMemory_Allocation_LocationMark_Here8);

	void * data = allocation + 1u;
	#ifndef abPlatform_CPU_64Bit
	if (align16 && ((size_t) data & 8u) != 0u) {
		// Add a padding between the allocation info and the data.
		((abMemory_Allocation *) ((uint8_t *) allocation + 8u))->locationMark = abMemory_Allocation_LocationMark_Back8Bytes;
		data = (uint8_t *) data + 8u;
	}
	#endif

	abParallel_Mutex_Lock(&tag->mutex);
	allocation->inTagPrevious = tag->allocationLast;
	allocation->inTagNext = abNull;
	if (tag->allocationLast != abNull) {
		tag->allocationLast->inTagNext = allocation;
	} else {
		tag->allocationFirst = allocation;
	}
	tag->allocationLast = allocation;
	tag->totalAllocated += size;
	abParallel_Mutex_Unlock(&tag->mutex);

	return data;
}

abMemory_Allocation * abMemory_GetAllocation(void * memory) {
	if (memory == abNull) {
		return abNull;
	}
	abMemory_Allocation * allocation = ((abMemory_Allocation *) memory - 1u);
	#ifndef abPlatform_CPU_64Bit
	if (allocation->locationMark == abMemory_Allocation_LocationMark_Back8Bytes) {
		allocation = (abMemory_Allocation *) ((uint8_t *) allocation - 8u);
	}
	#endif
	if (allocation->locationMark != abMemory_Allocation_LocationMark_Here8 &&
			allocation->locationMark != abMemory_Allocation_LocationMark_Here16) {
		return abNull;
	}
	return allocation;
}

void * abMemory_DoRealloc(void * memory, size_t size, char const * fileName, unsigned int fileLine) {
	if (memory == abNull) {
		abFeedback_Crash("abMemory_DoRealloc", "Tried to reallocate null memory at %s:%u.", fileName, fileLine);
	}

	abMemory_Allocation * allocation = abMemory_GetAllocation(memory);
	if (allocation == abNull) {
		abFeedback_Crash("abMemory_DoRealloc",
				"Tried to reallocate memory that wasn't allocated with abMemory_Alloc at %s:%u.", fileName, fileLine);
	}

	#ifndef abPlatform_CPU_64Bit
	abBool align16 = (allocation->locationMark == abMemory_Allocation_LocationMark_Here16);
	abBool wasPadded = (((size_t) (allocation + 1u) & 15u) != 0u);
	#endif
	size_t oldSize = allocation->size;
	size_t alignedSize = abAlign(size, (size_t) 16u); // Same padding as in DoAlloc.
	if (alignedSize == abAlign(oldSize, (size_t) 16u)) {
		return memory; // Not resizing.
	}

	abMemory_Tag * tag = allocation->tag;
	abParallel_Mutex_Lock(&tag->mutex);
	#ifdef abPlatform_CPU_64Bit
	allocation = realloc(allocation, sizeof(abMemory_Allocation) + alignedSize);
	#else
	allocation = realloc(allocation, sizeof(abMemory_Allocation) + (align16 ? 8u : 0u) + alignedSize);
	#endif
	if (allocation == abNull) {
		abFeedback_Crash("abMemory_DoRealloc", "Failed to allocate %zu bytes with tag %s at %s:%u.", size, tag->name, fileName, fileLine);
	}
	allocation->size = size;
	allocation->fileName = fileName;
	allocation->fileLine = fileLine;
	// Relink as the pointer might have been changed.
	if (allocation->inTagPrevious != abNull) {
		allocation->inTagPrevious->inTagNext = allocation->inTagNext;
	} else {
		tag->allocationFirst = allocation;
	}
	if (allocation->inTagNext != abNull) {
		allocation->inTagNext = allocation->inTagPrevious->inTagNext;
	} else {
		tag->allocationLast = allocation;
	}
	abParallel_Mutex_Unlock(&tag->mutex);

	void * data = allocation + 1u;
	#ifndef abPlatform_CPU_64Bit
	if (align16) {
		// If the data was 16-aligned previously, but is 8-aligned now, realign it.
		if (((size_t) data & 8u) != 0u) {
			if (!wasPadded) {
				memmove((uint8_t *) data + 8u, data, abMin(oldSize, size));
				((abMemory_Allocation *) ((uint8_t *) allocation + 8u))->locationMark = abMemory_Allocation_LocationMark_Back8Bytes;
			}
			data = (uint8_t *) data + 8u;
		} else {
			if (wasPadded) {
				memmove(data, (uint8_t *) data + 8u, abMin(oldSize, size));
			}
		}
	}
	#endif
	return data;
}

void abMemory_Free(void * memory) {
	if (memory == abNull) {
		return;
	}

	abMemory_Allocation * allocation = abMemory_GetAllocation(memory);
	if (allocation == abNull) {
		abFeedback_Crash("abMemory_Free", "Tried to free memory that wasn't allocated with abMemory_Alloc.");
	}

	abMemory_Tag * tag = allocation->tag;
	abParallel_Mutex_Lock(&tag->mutex);
	tag->totalAllocated -= allocation->size;
	if (allocation->inTagPrevious != abNull) {
		allocation->inTagPrevious->inTagNext = allocation->inTagNext;
	} else {
		tag->allocationFirst = allocation->inTagNext;
	}
	if (allocation->inTagNext != abNull) {
		allocation->inTagNext->inTagPrevious = allocation->inTagPrevious;
	} else {
		tag->allocationLast = allocation->inTagPrevious;
	}
	abParallel_Mutex_Unlock(&tag->mutex);

	free(allocation);
}

void abMemory_Shutdown() {
	// No need to lock the mutex - at this point, any interactions with abMemory are invalid.
	while (abMemory_TagList_Last != abNull) {
		abMemory_Tag_Destroy(abMemory_TagList_Last);
	}
	abParallel_Mutex_Destroy(&abMemory_TagList_Mutex);
}
