#ifndef abInclude_abMemory
#define abInclude_abMemory
#include "../abParallel/abParallel.h"

/*
 * An abstraction for memory allocation, inspired by the Qfusion engine <3
 * Handles allocation failures (by crashing) and 16 byte alignment.
 * Also allows for destruction of all allocations at once by destroying a tag.
 */

void abMemory_Init();

#ifdef abPlatform_CPU_64Bit
typedef struct abAligned(16u) abMemory_Allocation {
#else
typedef struct abAligned(8u) abMemory_Allocation {
#endif
	struct abMemory_Tag * tag; // 4 (32-bit) / 8 (64-bit) bytes.
	struct abMemory_Allocation * inTagPrevious, * inTagNext; // 12/24 bytes.
	size_t size; // 16/32 bytes.
	char const * fileName; // Immutable. 20/40 bytes.
	uint16_t fileLine; // 22/42 bytes.
	uint16_t alignmentType; // 24/44 bytes. If Got8Aligned, there's a 8-byte padding before this structure in malloc block.
} abMemory_Allocation;

typedef struct abMemory_Tag {
	char name[64u];

	abParallel_Mutex mutex;
	// Protected by the tag's mutex.
	abMemory_Allocation * allocationFirst, * allocationLast;
	size_t totalAllocated;

	// Protected by abMemory_TagList_Mutex.
	struct abMemory_Tag * tagPrevious, * tagNext;
} abMemory_Tag;

// Using Create rather than the usual Init to make sure tags created dynamically won't depend on other tags.
abMemory_Tag * abMemory_Tag_Create(char const * name);
void abMemory_Tag_Destroy(abMemory_Tag * tag);

extern abParallel_Mutex abMemory_TagList_Mutex;
extern abMemory_Tag * abMemory_TagList_First, * abMemory_TagList_Last;

void * abMemory_DoAlloc(abMemory_Tag * tag, size_t size, char const * fileName, unsigned int fileLine);
#define abMemory_Alloc(tag, size) abMemory_DoAlloc((tag), (size), __FILE__, __LINE__)
void abMemory_Free(void * memory);

void abMemory_Shutdown();

#endif
