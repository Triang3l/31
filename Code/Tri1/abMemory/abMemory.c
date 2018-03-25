#include "abMemory.h"
#include "../abFeedback/abFeedback.h"
#include "../abText/abText.h"
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
	abTextA_Copy(name, tag->name, abArrayLength(tag->name));
	tag->allocationFirst = tag->allocationLast = abNull;
	tag->totalAllocated = 0u;

	abParallel_Mutex_Lock(&abMemory_TagList_Mutex);
	tag->tagPrevious = abMemory_TagList_Last;
	tag->tagNext = abNull;
	if (abMemory_TagList_Last != abNull) {
		abMemory_TagList_Last->tagNext = tag;
	} else {
		abFeedback_Assert(abMemory_TagList_First == abNull, "abMemory_Tag_Create", "If there's no last tag, there mustn't be the first one.");
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
		abFeedback_Assert(abMemory_TagList_First == tag, "abMemory_Tag_Destroy", "If a tag has no previous, it must be the first.");
		abMemory_TagList_First = tag->tagNext;
	}
	if (tag->tagNext != abNull) {
		tag->tagNext->tagPrevious = tag->tagPrevious;
	} else {
		abFeedback_Assert(abMemory_TagList_Last == tag, "abMemory_Tag_Destroy", "If a tag has no next, it must be the last.");
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

void * abMemory_DoAlloc(abMemory_Tag * tag, size_t size, char const * fileName, unsigned int fileLine) {
	abMemory_Allocation * allocation;
	// Malloc gives 16-aligned blocks on 64-bit platforms, 8-aligned on 32-bit.
	#ifdef abPlatform_CPU_64Bit
	allocation = malloc(sizeof(abMemory_Allocation) + size);
	#else
	allocation = malloc(sizeof(abMemory_Allocation) + 8u + size);
	#endif
	if (allocation == abNull) {
		abFeedback_Crash("abMemory_DoAlloc", "Failed to allocate %zu bytes with tag %s at %s:%u.", size, tag->name, fileName, fileLine);
	}

	#ifdef abPlatform_CPU_64Bit
	if (((size_t) allocation & 15u) != 0u) {
		abFeedback_Crash("abMemory_DoAlloc", "Got a pointer from malloc that is not 16-aligned, this totally shouldn't happen!");
	}
	#else
	if (((size_t) allocation & 7u) != 0u) {
		abFeedback_Crash("abMemory_DoAlloc", "Got a pointer from malloc that is not 8-aligned, this totally shouldn't happen!");
	}
	#endif
	allocation->size = size;
	allocation->fileName = fileName;
	allocation->fileLine = (uint16_t) fileLine;
	allocation->locationMark = abMemory_Allocation_LocationMark_Here;

	void * data = allocation + 1u;
	#ifndef abPlatform_CPU_64Bit
	if (((size_t) data & 8u) != 0u) {
		// Add a padding between the allocation info and the data.
		((abMemory_Allocation *) ((uint8_t *) allocation + 8u))->locationMark = abMemory_Allocation_LocationMark_Back8Bytes;
		abFeedback_Break();
		data = (uint8_t *) data + 8u;
	}
	#endif

	allocation->tag = tag;
	abParallel_Mutex_Lock(&tag->mutex);
	allocation->inTagPrevious = tag->allocationLast;
	allocation->inTagNext = abNull;
	if (tag->allocationLast != abNull) {
		tag->allocationLast->inTagNext = allocation;
	} else {
		abFeedback_Assert(tag->allocationFirst == abNull, "abMemory_DoAlloc", "If a tag has no last allocation, it mustn't have the first one.");
		tag->allocationFirst = allocation;
	}
	tag->allocationLast = allocation;
	tag->totalAllocated += size;
	abParallel_Mutex_Unlock(&tag->mutex);

	return data;
}

void abMemory_Free(void * memory) {
	if (memory == abNull) {
		return;
	}

	abMemory_Allocation * allocation = ((abMemory_Allocation *) memory - 1u);
	#ifndef abPlatform_CPU_64Bit
	if (allocation->locationMark == abMemory_Allocation_LocationMark_Back8Bytes) {
		// There's alignment padding between the actual allocation info structure and the data.
		allocation = (abMemory_Allocation *) ((uint8_t *) allocation - 8u);
	}
	#endif
	if (allocation->locationMark != abMemory_Allocation_LocationMark_Here) {
		abFeedback_Crash("abMemory_Free", "Tried to free memory that wasn't allocated with abMemory_Alloc.");
	}

	abMemory_Tag * tag = allocation->tag;
	abParallel_Mutex_Lock(&tag->mutex);
	tag->totalAllocated -= allocation->size;
	if (allocation->inTagPrevious != abNull) {
		allocation->inTagPrevious->inTagNext = allocation->inTagNext;
	} else {
		abFeedback_Assert(tag->allocationFirst == allocation, "abMemory_Free", "If an allocation has no previous, it must be the first in tag.");
		tag->allocationFirst = allocation->inTagNext;
	}
	if (allocation->inTagNext != abNull) {
		allocation->inTagNext->inTagPrevious = allocation->inTagPrevious;
	} else {
		abFeedback_Assert(tag->allocationLast == allocation, "abMemory_Free", "If an allocation has no next, it must be the last in tag.");
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
