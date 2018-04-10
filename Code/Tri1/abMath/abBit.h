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
	#ifdef abPlatform_CPU_32Bit
	unsigned long index;
	uint32_t part = (uint32_t) value; if (part != 0u) { _BitScanForward(&index, part); return (int) index; }
	part = (uint32_t) (value >> 32u); if (part != 0u) { _BitScanForward(&index, part); return (int) index + 32; }
	return -1;
	#else
	long index = -1;
	if (value != 0ull) _BitScanForward64((unsigned long *) &index, value);
	return (int) index;
	#endif
}
abForceInline int abBit_HighestOne32(uint32_t value) {
	long index = -1;
	if (value != 0u) _BitScanReverse((unsigned long *) &index, value);
	return (int) index;
}
abForceInline int abBit_HighestOne64(uint64_t value) {
	#ifdef abPlatform_CPU_32Bit
	unsigned long index;
	uint32_t part = (uint32_t) (value >> 32u); if (part != 0u) { _BitScanReverse(&index, part); return (int) index + 32; }
	part = (uint32_t) value; if (part != 0u) { _BitScanReverse(&index, part); return (int) index; }
	return -1;
	#else
	long index = -1;
	if (value != 0ull) _BitScanReverse64((unsigned long *) &index, value);
	return (int) index;
	#endif
}

#else
#error No bitwise math functions for the current compiler.
#endif

#endif
