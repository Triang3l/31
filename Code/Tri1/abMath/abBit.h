#ifndef abInclude_abMath_abBit
#define abInclude_abMath_abBit
#include "../abCommon.h"

/*
 * Number bit sequence operations.
 */

#if defined(abPlatform_Compiler_MSVC)
#include <intrin.h>

// Shifts of the lowest/highest set bits, or -1 if there are none.
abForceInline int abBit_LowestOne32(uint32_t value) {
	long index = -1;
	if (value != 0u) _BitScanForward((unsigned long *) &index, value);
	return (int) index;
}
abForceInline int abBit_LowestOne64(uint64_t value) {
	long index = -1;
	if (value != 0ull) _BitScanForward64((unsigned long *) &index, value);
	return (int) index;
}
abForceInline int abBit_HighestOne32(uint32_t value) {
	long index = -1;
	if (value != 0u) _BitScanReverse((unsigned long *) &index, value);
	return (int) index;
}
abForceInline int abBit_HighestOne64(uint64_t value) {
	long index = -1;
	if (value != 0ull) _BitScanReverse64((unsigned long *) &index, value);
	return (int) index;
}

#else
#error No bitwise math functions for the current compiler.
#endif

#endif
