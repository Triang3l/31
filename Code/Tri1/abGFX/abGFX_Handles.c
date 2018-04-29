#include "abGFX.h"
#include "../abMath/abBit.h"
#include "../abFeedback/abFeedback.h"

abGPU_HandleStore abGFX_Handles_Store;

/*
 * An allocator for power-of-2-sized blocks.
 * Each power of 2 is called a level, with the first (0) level containing 2 blocks, and the last with MinBlockSize handles.
 *
 * Maintaining lists of free halves (nodes which are free, but have their ^1 sibling allocated) ensures that
 * the worst case allocation time will be logarithmic. Without such list, a linear search (recursive or horizontal)
 * will be required to find a free level 2 block in this case:
 *
 *   +---------------+---------------+
 * 0 |       S       |       S       |
 *   |---------------|---------------|
 * 1 |   S   |   S   |   S   |   S   |
 *   |-------|-------|-------|-------|
 * 2 | A | A | A | F | F | F | A | A |
 *   +---------------+---------------+
 *
 * The free half lists allow for cases that are trivially resolvable:
 *
 *   +---------------+---------------+
 * 0 |       S       |       S       |
 *   |---------------|---------------|
 * 1 |  (H)  |   A   |   A   |   H   |
 *   |-------|-------|-------|-------|
 * 2 |(F)| F | - | - | - | - | F | F |
 *   +---------------+---------------+
 *
 * Or:
 *
 *   +---------------+---------------+
 * 0 |       S       |       S       |
 *   |---------------|---------------|
 * 1 |   S   |   A   |   A   |   H   |
 *   |-------|-------|-------|-------|
 * 2 |(H)| A | - | - | - | - | F | F |
 *   +---------------+---------------+
 *
 * If there's free space, and the tree is not empty, there will be a free half on the needed level or shallower.
 */

typedef uint32_t abGFXi_Handles_Block;

enum {
	// How many handles the smallest allocated block contains.
	abGFXi_Handles_MinBlockSizePower = 2u,
	abGFXi_Handles_MinBlockSize = 1u << abGFXi_Handles_MinBlockSizePower,

	// The number of levels of blocks.
	abGFXi_Handles_LevelCount = 14u,

	// Total allocation block count.
	abGFXi_Handles_BlockCount = (1u << (abGFXi_Handles_LevelCount + 1u)) - 2u,

	// Total handle count.
	abGFXi_Handles_HandleCount = 1u << (abGFXi_Handles_LevelCount + abGFXi_Handles_MinBlockSizePower),

	// The ^1 block is also free (so the higher-level block is free).
	abGFXi_Handles_Block_Type_Free = 0u,
	// The ^1 block is occupied.
	// In <<abGFXi_Handles_Block_NextFreeHalfShift, the index of the free half within its level is located.
	abGFXi_Handles_Block_Type_FreeHalf,
	abGFXi_Handles_Block_Type_Split,
	abGFXi_Handles_Block_Type_Allocated,
	abGFXi_Handles_Block_TypeMask = 3u,

	// For no previous/next, write the index of the same block (saves a couple of bits).
	abGFXi_Handles_Block_FreeHalfPrevShift = 2u,
	abGFXi_Handles_Block_FreeHalfNextShift = abGFXi_Handles_Block_FreeHalfPrevShift + abGFXi_Handles_LevelCount,
	abGFXi_Handles_Block_FreeHalfLinkMask = (1u << abGFXi_Handles_LevelCount) - 1u,

	abGFXi_Handles_Block_TotalBitCount = abGFXi_Handles_Block_FreeHalfNextShift + abGFXi_Handles_LevelCount,

	abGFXi_Handles_FreeHalfFirstNone = UINT_MAX,

	abGFXi_Handles_MaxSingleAreas = abGFXi_Handles_HandleCount >> 5u,
	abGFXi_Handles_SingleAreaIndexNone = UINT_MAX
};

// Starting from the top level (with 2 blocks), then the level with 4 blocks.
static abGFXi_Handles_Block abGFXi_Handles_Blocks[abGFXi_Handles_BlockCount];

static unsigned int abGFXi_Handles_FirstFreeHalves[abGFXi_Handles_LevelCount];

// Single handle areas - blocks of 32 handles used for allocating individual handles that are not in sets (for effects).
typedef struct abGFXi_Handles_SingleArea {
	uint32_t handlesAllocated;
	uint32_t blockIndex;
} abGFXi_Handles_SingleArea;
abGFXi_Handles_SingleArea abGFXi_Handles_SingleAreas[abGFXi_Handles_MaxSingleAreas]; // Available followed by full.
unsigned int abGFXi_Handles_SingleAreasAvailable, abGFXi_Handles_SingleAreasUsed;
static unsigned int abGFXi_Handles_SingleAreaIndices[abGFXi_Handles_MaxSingleAreas]; // Mapping of handle indices to single handle areas.

void abGFXim_Handles_Init() {
	if (!abGPU_HandleStore_Init(&abGFX_Handles_Store, "abGFX_Handles_Store", abGFXi_Handles_HandleCount)) {
		abFeedback_Crash("abGFX_Handles_Init", "Failed to create the store for %u texture/buffer handles.", abGFXi_Handles_HandleCount);
	}
	memset(abGFXi_Handles_Blocks, 0u, sizeof(abGFXi_Handles_Blocks));
	memset(abGFXi_Handles_FirstFreeHalves, 255u, sizeof(abGFXi_Handles_FirstFreeHalves));
	abGFXi_Handles_SingleAreasAvailable = abGFXi_Handles_SingleAreasUsed = 0u;
	memset(abGFXi_Handles_SingleAreaIndices, 255u, sizeof(abGFXi_Handles_SingleAreaIndices));
}

static abForceInline unsigned int abGFXim_Handles_LevelBlockOffset(unsigned int level) {
	return (1u << (level + 1u)) - 2u;
}

static void abGFXim_Handles_MakeFreeHalf(unsigned int level, unsigned int blockIndex) {
	unsigned int blockOffset = abGFXim_Handles_LevelBlockOffset(level);
	abGFXi_Handles_Block * block = &abGFXi_Handles_Blocks[blockOffset + blockIndex];
	unsigned int * firstFreeHalf = &abGFXi_Handles_FirstFreeHalves[level];
	if (*firstFreeHalf != abGFXi_Handles_FreeHalfFirstNone) {
		unsigned int secondBlockIndex = *firstFreeHalf;
		abGFXi_Handles_Block * secondBlock = &abGFXi_Handles_Blocks[blockOffset + secondBlockIndex];
		*secondBlock &= ~(abGFXi_Handles_Block_FreeHalfLinkMask << abGFXi_Handles_Block_FreeHalfPrevShift);
		*secondBlock |= blockIndex << abGFXi_Handles_Block_FreeHalfPrevShift;
		*block = abGFXi_Handles_Block_Type_FreeHalf |
				(blockIndex << abGFXi_Handles_Block_FreeHalfPrevShift) |
				(secondBlockIndex << abGFXi_Handles_Block_FreeHalfNextShift);
	} else {
		*block = abGFXi_Handles_Block_Type_FreeHalf |
				(blockIndex << abGFXi_Handles_Block_FreeHalfPrevShift) |
				(blockIndex << abGFXi_Handles_Block_FreeHalfNextShift);
	}
	*firstFreeHalf = blockIndex;
}

static void abGFXim_Handles_UnlinkFreeHalf(unsigned int level, unsigned int blockIndex) {
	unsigned int blockOffset = abGFXim_Handles_LevelBlockOffset(level);
	abGFXi_Handles_Block block = abGFXi_Handles_Blocks[blockOffset + blockIndex];
	unsigned int prevBlockIndex = (block >> abGFXi_Handles_Block_FreeHalfPrevShift) & abGFXi_Handles_Block_FreeHalfLinkMask;
	unsigned int nextBlockIndex = (block >> abGFXi_Handles_Block_FreeHalfNextShift) & abGFXi_Handles_Block_FreeHalfLinkMask;

	if (prevBlockIndex != blockIndex) {
		abGFXi_Handles_Block * prevBlock = &abGFXi_Handles_Blocks[blockOffset + prevBlockIndex];
		*prevBlock &= ~(abGFXi_Handles_Block_FreeHalfLinkMask << abGFXi_Handles_Block_FreeHalfNextShift);
		*prevBlock |= ((nextBlockIndex != blockIndex) ? nextBlockIndex : prevBlockIndex) << abGFXi_Handles_Block_FreeHalfNextShift;
	} else {
		abGFXi_Handles_FirstFreeHalves[level] = ((nextBlockIndex != blockIndex) ? nextBlockIndex : abGFXi_Handles_FreeHalfFirstNone);
	}

	if (nextBlockIndex != blockIndex) {
		abGFXi_Handles_Block * nextBlock = &abGFXi_Handles_Blocks[blockOffset + nextBlockIndex];
		*nextBlock &= ~(abGFXi_Handles_Block_FreeHalfLinkMask << abGFXi_Handles_Block_FreeHalfPrevShift);
		*nextBlock |= ((prevBlockIndex != blockIndex) ? prevBlockIndex : nextBlockIndex) << abGFXi_Handles_Block_FreeHalfPrevShift;
	}
}

static unsigned int abGFXim_Handles_AllocSingle() {
	if (abGFXi_Handles_SingleAreasAvailable == 0u) {
		if (abGFXi_Handles_SingleAreasUsed >= abGFXi_Handles_MaxSingleAreas) {
			abFeedback_Crash("abGFXim_Handles_AllocSingle", "No free space for 1 texture/buffer handle.");
		}
		unsigned int newAreaHandleIndex = abGFXm_Handles_Alloc(32u);
		if (abGFXi_Handles_SingleAreasUsed != 0u) {
			// First full -> last full, to free space for a new available area.
			abGFXi_Handles_SingleAreaIndices[abGFXi_Handles_SingleAreas[0u].blockIndex] = abGFXi_Handles_SingleAreasUsed;
			abGFXi_Handles_SingleAreas[abGFXi_Handles_SingleAreasUsed] = abGFXi_Handles_SingleAreas[0u];
		}
		++abGFXi_Handles_SingleAreasAvailable;
		++abGFXi_Handles_SingleAreasUsed;
		abGFXi_Handles_SingleAreaIndices[newAreaHandleIndex >> 5u] = 0u;
		abGFXi_Handles_SingleAreas[0u].blockIndex = newAreaHandleIndex >> 5u;
		abGFXi_Handles_SingleAreas[0u].handlesAllocated = 1u;
		return newAreaHandleIndex;
	}
	// Take the last available area, adjacent to the full areas.
	abGFXi_Handles_SingleArea * area = &abGFXi_Handles_SingleAreas[abGFXi_Handles_SingleAreasAvailable - 1u];
	unsigned int handleOffsetInArea = (unsigned int) abBit_LowestOne32(~(area->handlesAllocated));
	if ((area->handlesAllocated |= ((uint32_t) 1u << handleOffsetInArea)) == UINT32_MAX) {
		--abGFXi_Handles_SingleAreasAvailable;
	}
	return (area->blockIndex << 5u) + handleOffsetInArea;
}

unsigned int abGFXm_Handles_Alloc(unsigned int count) {
	abFeedback_StaticAssert(abGFXi_Handles_Block_TotalBitCount <= sizeof(abGFXi_Handles_Block) * 8u,
			"Not enough bits in abGFXi_Handles_Block to store the type and two links for free halves.");

	if (count == 0u) {
		return abGFXi_Handles_HandleCount; // A more or less valid range (definitely not a valid index though).
	}
	if (count == 1u) {
		return abGFXim_Handles_AllocSingle(); // Special handling.
	}
	if (count > (abGFXi_Handles_HandleCount >> 1u)) {
		abFeedback_Crash("abGFXm_Handles_Alloc", "Too many texture/buffer handles requested: %u, maximum %u.",
				count, abGFXi_Handles_HandleCount >> 1u);
	}

	unsigned int requestedLevel = (abGFXi_Handles_LevelCount - 1u) - ((unsigned int) abBit_HighestOne32(
			abAlign(count, abGFXi_Handles_MinBlockSize)) - abGFXi_Handles_MinBlockSizePower);
	if (count > abGFXi_Handles_MinBlockSize && (count & (count - 1u)) != 0u) {
		--requestedLevel;
	}

	// Find the closest free half.
	int freeHalfLevel;
	for (freeHalfLevel = (int) requestedLevel; freeHalfLevel >= 0; --freeHalfLevel) {
		if (abGFXi_Handles_FirstFreeHalves[freeHalfLevel] != abGFXi_Handles_FreeHalfFirstNone) {
			break;
		}
	}

	// Choose the index of the free half to split or to mark as allocated (depending on whether it's on the requested level).
	unsigned int blockIndex;
	if (freeHalfLevel < 0) {
		// If there's no free half, the tree is either empty or there's no free space.
		// Only need to check one, if the first isn't free, the second can't be Free, only FreeHalf (but then it'd be found).
		if (abGFXi_Handles_Blocks[0u] != abGFXi_Handles_Block_Type_Free) {
			abFeedback_Crash("abGFXm_Handles_Alloc", "No free space for %u texture/buffer handles.", count);
		}
		blockIndex = 0u;
		freeHalfLevel = 0;
		abGFXi_Handles_Blocks[1u] = abGFXi_Handles_Block_Type_FreeHalf |
				(1u << abGFXi_Handles_Block_FreeHalfPrevShift) | (1u << abGFXi_Handles_Block_FreeHalfNextShift);
		abGFXi_Handles_FirstFreeHalves[0u] = 1u;
	} else {
		blockIndex = abGFXi_Handles_FirstFreeHalves[freeHalfLevel];
		abGFXim_Handles_UnlinkFreeHalf(freeHalfLevel, blockIndex);
	}

	// Bridge with split nodes, then mark as allocated.
	for (unsigned int level = freeHalfLevel; level <= requestedLevel; ++level) {
		unsigned int blockOffset = abGFXim_Handles_LevelBlockOffset(level);
		abGFXi_Handles_Blocks[blockOffset + blockIndex] = ((level == requestedLevel) ?
				abGFXi_Handles_Block_Type_Allocated : abGFXi_Handles_Block_Type_Split);
		if (level != freeHalfLevel) {
			abGFXim_Handles_MakeFreeHalf(level, blockIndex ^ 1u);
		}
		if (level != requestedLevel) {
			blockIndex <<= 1u;
		}
	}

	return blockIndex << ((abGFXi_Handles_LevelCount - 1u) - requestedLevel + abGFXi_Handles_MinBlockSizePower);
}

static abBool abGFXim_Handles_FreeSingle(unsigned int handleIndex) {
	unsigned int areaIndex = abGFXi_Handles_SingleAreaIndices[handleIndex >> 5u];
	if (areaIndex == abGFXi_Handles_SingleAreaIndexNone) {
		return abFalse;
	}
	abGFXi_Handles_SingleArea * area = &abGFXi_Handles_SingleAreas[areaIndex];
	uint32_t handleBit = (uint32_t) 1u << (handleIndex & 31u);
	if (!(area->handlesAllocated & handleBit)) {
		abFeedback_Crash("abGFXim_Handles_FreeSingle", "Tried to free unallocated handles starting with %u in a single-handle area.", handleIndex);
	}
	abBool wasFull = (area->handlesAllocated == UINT32_MAX);
	area->handlesAllocated &= ~handleBit;
	if (wasFull) {
		if (areaIndex != abGFXi_Handles_SingleAreasAvailable) {
			// This newly available <-> first full (because increasing SingleAreasAvailable will make the first full available).
			abGFXi_Handles_SingleArea * firstFullArea = &abGFXi_Handles_SingleAreas[abGFXi_Handles_SingleAreasAvailable];
			unsigned int blockIndex = area->blockIndex, firstFullBlockIndex = firstFullArea->blockIndex;
			abGFXi_Handles_SingleAreaIndices[blockIndex] = abGFXi_Handles_SingleAreasAvailable;
			abGFXi_Handles_SingleAreaIndices[firstFullBlockIndex] = areaIndex;
			area->blockIndex = firstFullBlockIndex;
			firstFullArea->blockIndex = blockIndex;
			uint32_t handlesAllocated = area->handlesAllocated;
			area->handlesAllocated = firstFullArea->handlesAllocated;
		}
		++abGFXi_Handles_SingleAreasAvailable;
	} else if (area->handlesAllocated == 0u) {
		--abGFXi_Handles_SingleAreasAvailable;
		--abGFXi_Handles_SingleAreasUsed;
		if (areaIndex != abGFXi_Handles_SingleAreasAvailable) {
			// Last available -> new free spot in the available indices.
			abGFXi_Handles_SingleArea const * areaSource = &abGFXi_Handles_SingleAreas[abGFXi_Handles_SingleAreasAvailable];
			abGFXi_Handles_SingleAreaIndices[areaSource->blockIndex] = areaIndex;
			*area = *areaSource;
		}
		if (abGFXi_Handles_SingleAreasUsed != abGFXi_Handles_SingleAreasAvailable) {
			// Last full -> newly created hole between available and full.
			abGFXi_Handles_SingleArea * areaTarget = &abGFXi_Handles_SingleAreas[abGFXi_Handles_SingleAreasAvailable];
			abGFXi_Handles_SingleArea const * areaSource = &abGFXi_Handles_SingleAreas[abGFXi_Handles_SingleAreasUsed];
			abGFXi_Handles_SingleAreaIndices[areaSource->blockIndex] = abGFXi_Handles_SingleAreasAvailable;
			*areaTarget = *areaSource;
		}
		abGFXi_Handles_SingleAreaIndices[handleIndex >> 5u] = abGFXi_Handles_SingleAreaIndexNone;
		abGFXm_Handles_Free(handleIndex & ~31u);
	}
	return abTrue;
}

void abGFXm_Handles_Free(unsigned int handleIndex) {
	if (handleIndex == abGFXi_Handles_HandleCount) {
		// Special case for empty allocations.
		return;
	}
	if (handleIndex > abGFXi_Handles_HandleCount) {
		abFeedback_Crash("abGFXm_Handles_Free", "Tried to free handles starting with %u, which is out of range.", handleIndex);
	}
	if (abGFXim_Handles_FreeSingle(handleIndex)) {
		return;
	}
	if ((handleIndex & (abGFXi_Handles_MinBlockSize - 1u)) != 0u) {
		abFeedback_Crash("abGFXm_Handles_Free", "Tried to free handles starting with %u, which has invalid alignment.", handleIndex);
	}

	// Find the block containing the handle. Start from the level that actually may contain the allocation according to the alignment.
	unsigned int lastLevelBlockIndex = handleIndex >> abGFXi_Handles_MinBlockSizePower;
	int level = (lastLevelBlockIndex != 0u ? ((abGFXi_Handles_LevelCount - 1u) - abBit_LowestOne32(lastLevelBlockIndex)) : 0u);
	unsigned int blockIndex = 0u;
	for (; level < abGFXi_Handles_LevelCount; ++level) {
		blockIndex = lastLevelBlockIndex >> ((abGFXi_Handles_LevelCount - 1u) - level);
		if (abGFXi_Handles_Blocks[abGFXim_Handles_LevelBlockOffset(level) + blockIndex] == abGFXi_Handles_Block_Type_Allocated) {
			break;
		}
	}
	if (level >= abGFXi_Handles_LevelCount) {
		abFeedback_Crash("abGFXm_Handles_Free", "Tried to free a range of handles not allocated with abGFXm_Handles_Alloc (%u).", handleIndex);
	}

	// Mark the allocated block as free and ensure no split block contains 2 free blocks (in this case, it becomes free).
	for (; level >= 0; --level, blockIndex >>= 1u) {
		unsigned int blockOffset = abGFXim_Handles_LevelBlockOffset(level);
		abGFXi_Handles_Block * otherBlock = &abGFXi_Handles_Blocks[blockOffset + (blockIndex ^ 1u)];
		if (*otherBlock == abGFXi_Handles_Block_Type_Split || *otherBlock == abGFXi_Handles_Block_Type_Allocated) {
			abGFXim_Handles_MakeFreeHalf(level, blockIndex);
			break;
		}
		abGFXi_Handles_Blocks[abGFXim_Handles_LevelBlockOffset(level) + blockIndex] = abGFXi_Handles_Block_Type_Free;
		abGFXim_Handles_UnlinkFreeHalf(level, blockIndex ^ 1u);
		*otherBlock = abGFXi_Handles_Block_Type_Free;
	}
}

void abGFXim_Handles_Shutdown() {
	abGPU_HandleStore_Destroy(&abGFX_Handles_Store);
}
