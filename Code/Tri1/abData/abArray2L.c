#include "abArray2L.h"
#include "../abFeedback/abFeedback.h"

void abArray2L_Reserve(abArray2L * array2L, unsigned int newMinimumElementCount) {
	unsigned int pagesRequired = (newMinimumElementCount >> array2L->pageIndexShift) +
			((newMinimumElementCount & array2L->pageElementIndexMask) != 0u);
	if (array2L->i_currentPageListCapacity < pagesRequired) {
		unsigned int newPageListCapacity = abBit_ToNextPO2SaturatedU32(pagesRequired);
		if (array2L->i_currentPageListCapacity <= abArrayLength(array2L->i_initialPages)) {
			// Statically allocated - allocate dynamically and copy.
			array2L->pages = abMemory_Alloc(array2L->i_memoryTag, newPageListCapacity * sizeof(void *), abFalse);
			memcpy(array2L->pages, array2L->i_initialPages, array2L->i_currentPageCount * sizeof(void *));
		} else {
			// Dynamically allocated - expand.
			abMemory_Realloc((void * *) &array2L->pages, newPageListCapacity * sizeof(void *));
		}
		array2L->i_currentPageListCapacity = newPageListCapacity;
	}
	if (array2L->i_currentPageCount < pagesRequired) {
		size_t pageSize = (size_t) array2L->elementSize * (array2L->pageElementIndexMask + 1u);
		while (array2L->i_currentPageCount < pagesRequired) {
			array2L->pages[array2L->i_currentPageCount++] = abMemory_Alloc(array2L->i_memoryTag, pageSize, abTrue);
		}
	}
}

void abArray2L_RemoveMovingLast(abArray2L * array2L, unsigned int elementIndex) {
	abFeedback_Assert(elementIndex < array2L->elementCount, "abArray2L_RemoveMovingLast",
			"Tried to remove element %u from array of size %u.", elementIndex, array2L->elementCount);
	unsigned int lastElementIndex = array2L->elementCount - 1u;
	if (elementIndex != lastElementIndex) {
		memcpy(abArray2L_Get(array2L, elementIndex), abArray2L_Get(array2L, lastElementIndex), array2L->elementSize);
	}
	--array2L->elementCount;
}

void abArray2L_Trim(abArray2L * array2L, abBool trimPageList) {
	unsigned int neededPageCount = (array2L->elementCount >> array2L->pageIndexShift) +
			((array2L->elementCount & array2L->pageElementIndexMask) != 0u);
	while (array2L->i_currentPageCount > neededPageCount) {
		abMemory_Free(array2L->pages[--array2L->i_currentPageCount]);
	}
	if (trimPageList) {
		unsigned int neededPageListCapacity = abBit_ToNextPO2SaturatedU32(neededPageCount);
		neededPageListCapacity = abMax(abArrayLength(array2L->i_initialPages), neededPageListCapacity);
		if (array2L->i_currentPageListCapacity > neededPageListCapacity) {
			if (neededPageListCapacity > abArrayLength(array2L->i_initialPages)) {
				abMemory_Realloc((void * *) &array2L->pages, neededPageListCapacity * sizeof(void *));
			} else {
				abMemory_Free(array2L->pages);
				array2L->pages = array2L->i_initialPages;
			}
			array2L->i_currentPageListCapacity = neededPageListCapacity;
		}
	}
}

void abArray2L_Destroy(abArray2L * array2L) {
	unsigned int pageCount = array2L->i_currentPageCount;
	while (pageCount != 0u) {
		abMemory_Free(array2L->pages[--pageCount]);
	}
	if (array2L->i_currentPageListCapacity > abArrayLength(array2L->i_initialPages)) {
		abMemory_Free(array2L->pages);
	}
}
