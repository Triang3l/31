#ifndef abInclude_abMemory
#define abInclude_abMemory
#include "../abParallel/abParallel.h"

/*
 * An abstraction for memory allocation, inspired by the Qfusion engine <3
 * Handles allocation failures (by crashing), 16 byte alignment and zero sizes (gracefully, as different pointers).
 * Also allows for destruction of all allocations at once by destroying a tag.
 */

void abMemory_Init();

typedef enum abMemory_Allocation_LocationMark {
	abMemory_Allocation_LocationMark_Here8 = 0xab08u, // This is the actual allocation info structure for 8-aligned memory.
	abMemory_Allocation_LocationMark_Here16 = 0xab16u, // This is the actual allocation info structure for 16-aligned memory.
	abMemory_Allocation_LocationMark_Back8Bytes = 0xabb8u // The real structure is 8 bytes before, this is alignment padding.
} abMemory_Allocation_LocationMark;

#ifdef abPlatform_CPU_64Bit
typedef struct abAligned(16u) abMemory_Allocation {
#else
typedef struct abAligned(8u) abMemory_Allocation {
#endif
	struct abMemory_Tag * tag; // 4 (32-bit) / 8 (64-bit) bytes.
	struct abMemory_Allocation * inTagPrevious, * inTagNext; // 12/24 bytes, protected by the tag's mutex.
	size_t size; // 16/32 bytes, mutex-protected as this may be changed by realloc and thus may affect allocation list displaying.
	char const * fileName; // Immutable. 20/40 bytes, mutex-protected due to realloc.
	uint16_t fileLine; // 22/42 bytes, mutex-protected due to realloc.
	uint16_t locationMark; // 24/44 bytes, also acts as a sentinel. Must be the last field!
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

void * abMemory_DoAlloc(abMemory_Tag * tag, size_t size, abBool align16, char const * fileName, unsigned int fileLine);
#define abMemory_Alloc(tag, size, align16) abMemory_DoAlloc((tag), (size), (align16), __FILE__, __LINE__)
void * abMemory_DoRealloc(void * memory, size_t size, char const * fileName, unsigned int fileLine);
#define abMemory_Realloc(memory, size) abMemory_DoRealloc((memory), (size), __FILE__, __LINE__)
void abMemory_Free(void * memory);

void abMemory_Shutdown();

#endif
