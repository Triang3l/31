#ifndef abInclude_abData_abArray2L
#define abInclude_abData_abArray2L
#include "../abMath/abBit.h"
#include "../abMemory/abMemory.h"

// A two-level dynamic array for re-allocating without copying.
// Pointers to elements are persistent as long as the elements exist (removal moves the last element into the free space).
// NOT relocatable due to i_initialPages, this is not nice!

typedef struct abArray2L {
	// Really public numbers.
	unsigned int elementCount; // Current number of elements.

	// These numbers may be used externally for direct access.
	unsigned int elementSize; // Size of a single element.
	unsigned int pageIndexShift; // log2(per-page element count).
	unsigned int pageElementIndexMask; // Per-page element count - 1u, or (1u << pageIndexShift) - 1u.

	// Really internal.
	unsigned int i_currentPageCount;
	unsigned int i_currentPageListCapacity;

	// Pointers.
	void * * pages; // May be used externally for direct access.
	void * i_initialPages[4u];
	abMemory_Tag * i_memoryTag; // Pretty much internal.
} abArray2L;

abForceInline void abArray2L_Init(abArray2L * array2L, abMemory_Tag * memoryTag, unsigned int elementSize, unsigned int elementsPerPage) {
	array2L->elementCount = 0u;
	array2L->elementSize = elementSize;
	array2L->pageIndexShift = abBit_NextPO2SaturatedShiftU32(elementsPerPage);
	array2L->pageElementIndexMask = (1u << array2L->pageIndexShift) - 1u;
	array2L->i_currentPageCount = 0u;
	array2L->i_currentPageListCapacity = abArrayLength(array2L->i_initialPages);
	array2L->pages = array2L->i_initialPages;
	array2L->i_memoryTag = memoryTag;
}

abForceInline void * abArray2L_Get(abArray2L const * array2L, unsigned int elementIndex) {
	return (uint8_t *) array2L->pages[elementIndex >> array2L->pageIndexShift] +
			((elementIndex & array2L->pageElementIndexMask) * array2L->elementSize);
}

void abArray2L_Reserve(abArray2L * array2L, unsigned int newMinimumElementCount);
abForceInline void abArray2L_Resize(abArray2L * array2L, unsigned int newElementCount) {
	abArray2L_Reserve(array2L, newElementCount);
	array2L->elementCount = newElementCount;
}
abForceInline void * abArray2L_AddElement(abArray2L * array2L) {
	abArray2L_Resize(array2L, array2L->elementCount + 1u);
	return abArray2L_Get(array2L, array2L->elementCount - 1u);
}

void abArray2L_RemoveMovingLast(abArray2L * array2L, unsigned int elementIndex);

void abArray2L_Trim(abArray2L * array2L, abBool trimPageList);

void abArray2L_Destroy(abArray2L * array2L);

#endif
