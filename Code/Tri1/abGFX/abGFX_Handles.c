#include "abGFX.h"
#include "../abMath/abBit.h"
#include "../abFeedback/abFeedback.h"

abGPU_HandleStore abGFX_Handles_Store;

/*
 * An allocator for power-of-2-sized blocks.
 * Each power of 2 is called a level, with the first (0) level containing 2 blocks, and the last with MinBlockSize handles.
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

	abGFXi_Handles_FreeHalfFirstNone = UINT_MAX
};

// Starting from the top level (with 2 blocks), then the level with 4 blocks.
static abGFXi_Handles_Block abGFXi_Handles_Blocks[abGFXi_Handles_BlockCount];

static unsigned int abGFXi_Handles_FirstFreeHalves[abGFXi_Handles_LevelCount];

void abGFXi_Handles_Init() {
	if (!abGPU_HandleStore_Init(&abGFX_Handles_Store, "abGFX_Handles_Store", abGFXi_Handles_HandleCount)) {
		abFeedback_Crash("abGFX_Handles_Init", "Failed to create the store for %u texture/buffer handles.", abGFXi_Handles_HandleCount);
	}
	memset(abGFXi_Handles_Blocks, 0u, sizeof(abGFXi_Handles_Blocks));
	memset(abGFXi_Handles_FirstFreeHalves, 255u, sizeof(abGFXi_Handles_FirstFreeHalves));
}

abForceInline unsigned int abGFX_Handles_LevelBlockOffset(unsigned int level) {
	return (1u << (level + 1u)) - 2u;
}

unsigned int abGFX_Handles_Alloc(unsigned int count) {
	abFeedback_StaticAssert(abGFXi_Handles_Block_TotalBitCount <= sizeof(abGFXi_Handles_Block) * 8u,
			"Not enough bits in abGFXi_Handles_Block to store the type and two links for free halves.");

	if (count == 0u) {
		return abGFXi_Handles_HandleCount; // A more or less valid range (definitely not a valid index though).
	}
	if (count > (abGFXi_Handles_HandleCount >> 1u)) {
		return abGFX_Handles_Alloc_Failed;
	}

	unsigned int requestedLevel = (abGFXi_Handles_LevelCount - 1u) - ((unsigned int) abBit_HighestOne32(
			abAlign(count, abGFXi_Handles_MinBlockSize)) - abGFXi_Handles_MinBlockSizePower);
	if (count > abGFXi_Handles_MinBlockSize && (count & (count - 1u)) != 0u) {
		--requestedLevel;
	}

	/*
	 * Maintaining lists of free halves (nodes which are free, but have their ^1 sibling allocated)
	 * ensures that the worst case allocation time will be logarithmic. Without such list, a linear
	 * search (recursive or horizontal) will be required to find a free level 2 block in this case:
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
			return abGFX_Handles_Alloc_Failed;
		}
		blockIndex = 0u;
		freeHalfLevel = 0;
		abGFXi_Handles_Blocks[1u] = abGFXi_Handles_Block_Type_FreeHalf |
				(1u << abGFXi_Handles_Block_FreeHalfPrevShift) | (1u << abGFXi_Handles_Block_FreeHalfNextShift);
		abGFXi_Handles_FirstFreeHalves[0u] = 1u;
	} else {
		unsigned int blockOffset = abGFX_Handles_LevelBlockOffset(freeHalfLevel);
		blockIndex = abGFXi_Handles_FirstFreeHalves[freeHalfLevel];
		abGFXi_Handles_Block freeHalf = abGFXi_Handles_Blocks[blockOffset + blockIndex];
		unsigned int nextFreeHalfIndex = (freeHalf >> abGFXi_Handles_Block_FreeHalfNextShift) & abGFXi_Handles_Block_FreeHalfLinkMask;
		if (nextFreeHalfIndex != blockIndex) {
			abGFXi_Handles_Block * secondFreeHalf = &abGFXi_Handles_Blocks[blockOffset + nextFreeHalfIndex];
			*secondFreeHalf &= ~(abGFXi_Handles_Block_FreeHalfLinkMask << abGFXi_Handles_Block_FreeHalfPrevShift);
			*secondFreeHalf |= nextFreeHalfIndex << abGFXi_Handles_Block_FreeHalfPrevShift;
			abGFXi_Handles_FirstFreeHalves[freeHalfLevel] = nextFreeHalfIndex;
		} else {
			abGFXi_Handles_FirstFreeHalves[freeHalfLevel] = abGFXi_Handles_FreeHalfFirstNone;
		}
	}

	// Bridge with split nodes, then mark as allocated.
	for (unsigned int level = freeHalfLevel; level <= requestedLevel; ++level) {
		unsigned int blockOffset = abGFX_Handles_LevelBlockOffset(level);
		abGFXi_Handles_Blocks[blockOffset + blockIndex] = ((level == requestedLevel) ?
				abGFXi_Handles_Block_Type_Allocated : abGFXi_Handles_Block_Type_Split);
		if (level != freeHalfLevel) {
			unsigned int newFreeHalfIndex = blockIndex ^ 1u;
			unsigned int * levelFirstFreeHalfIndex = &abGFXi_Handles_FirstFreeHalves[level];
			if (*levelFirstFreeHalfIndex != abGFXi_Handles_FreeHalfFirstNone) {
				// Link with the next one - update the previous in the next.
				abGFXi_Handles_Block * secondFreeHalf = &abGFXi_Handles_Blocks[blockOffset + *levelFirstFreeHalfIndex];
				*secondFreeHalf &= ~(abGFXi_Handles_Block_FreeHalfLinkMask << abGFXi_Handles_Block_FreeHalfPrevShift);
				*secondFreeHalf |= newFreeHalfIndex << abGFXi_Handles_Block_FreeHalfPrevShift;
				abGFXi_Handles_Blocks[blockOffset + newFreeHalfIndex] = abGFXi_Handles_Block_Type_FreeHalf |
						(newFreeHalfIndex << abGFXi_Handles_Block_FreeHalfPrevShift) |
						(*levelFirstFreeHalfIndex << abGFXi_Handles_Block_FreeHalfNextShift);
			} else {
				abGFXi_Handles_Blocks[blockOffset + newFreeHalfIndex] = abGFXi_Handles_Block_Type_FreeHalf |
					(newFreeHalfIndex << abGFXi_Handles_Block_FreeHalfPrevShift) |
					(newFreeHalfIndex << abGFXi_Handles_Block_FreeHalfNextShift);
			}
			*levelFirstFreeHalfIndex = newFreeHalfIndex;
		}
		if (level != requestedLevel) {
			blockIndex <<= 1u;
		}
	}

	return blockIndex << ((abGFXi_Handles_LevelCount - 1u) - requestedLevel + abGFXi_Handles_MinBlockSizePower);
}

void abGFXi_Handles_Shutdown() {
	abGPU_HandleStore_Destroy(&abGFX_Handles_Store);
}
