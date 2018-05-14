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
	if (value != 0u) { _BitScanForward((unsigned long *) &index, value); }
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
	if (value != 0ull) { _BitScanForward64((unsigned long *) &index, value); }
	return (int) index;
	#endif
}
abForceInline int abBit_HighestOne32(uint32_t value) {
	long index = -1;
	if (value != 0u) { _BitScanReverse((unsigned long *) &index, value); }
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
	if (value != 0ull) { _BitScanReverse64((unsigned long *) &index, value); }
	return (int) index;
	#endif
}

#else
#error No bitwise math functions for the current compiler.
#endif

abForceInline unsigned int abBit_OneCount32(uint32_t value) {
	value -= (value >> 1u) & 0x55555555u;
	value = (value & 0x33333333u) + ((value >> 2u) & 0x33333333u);
	return (((value + (value >> 4u)) & 0x0f0f0f0fu) * 0x01010101u) >> 24u;
}

abForceInline unsigned int abBit_OneCount64(uint64_t value) {
	#ifdef abPlatform_CPU_32Bit
	return abBit_OneCount32((uint32_t) value) + abBit_OneCount32((uint32_t) (value >> 32u));
	#else
	value -= (value >> 1u) & 0x5555555555555555ull;
	value = (value & 0x3333333333333333ull) + ((value >> 2u) & 0x3333333333333333ull);
	return (((value + (value >> 4u)) & 0x0f0f0f0f0f0f0f0full) * 0x0101010101010101ull) >> 56u;
	#endif
}

abForceInline abBool abBit_IsPO2U32(uint32_t value) { return (value & (value - 1u)) == 0u; }
abForceInline abBool abBit_IsPO2U64(uint64_t value) { return (value & (value - 1u)) == 0u; }

// Returns 0 for zero, since there's no way to express zero with an defined 1u << shift.

abForceInline unsigned int abBit_NextPO2SaturatedShiftU32(uint32_t value) {
	if (value == 0u) { return 0u; }
	unsigned int shift = (unsigned int) abBit_HighestOne32(value);
	if ((value & (value - 1u)) == 0u && shift < 31u) { ++shift; }
	return shift;
}

abForceInline unsigned int abBit_NextPO2SaturatedShiftU64(uint64_t value) {
	if (value == 0u) { return 0u; }
	unsigned int shift = (unsigned int) abBit_HighestOne64(value);
	if ((value & (value - 1u)) == 0u && shift < 63u) { ++shift; }
	return shift;
}

// This is different than 1u << NextPO2SaturatedShift for zero - actually returns 0!

abForceInline uint32_t abBit_ToNextPO2SaturatedU32(uint32_t value) {
	if ((value & (value - 1u)) == 0u) { return value; }
	unsigned int shift = (unsigned int) abBit_HighestOne32(value);
	return 1u << (shift + ((shift >> 5u) ^ 1u)); // Rather than a conditional min.
}

abForceInline uint64_t abBit_ToNextPO2SaturatedU64(uint64_t value) {
	if ((value & (value - 1u)) == 0u) { return value; }
	unsigned int shift = (unsigned int) abBit_HighestOne64(value);
	return 1ull << (shift + ((shift >> 6u) ^ 1u)); // Rather than a conditional min.
}

#endif
