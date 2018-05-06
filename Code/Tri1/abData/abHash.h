#ifndef abInclude_abData_abHash
#define abInclude_abData_abHash
#include "../abCommon.h"

#define abHash_FNV_Base 0x811c9dc5u
#define abHash_FNV_Prime 0x1000193u

abForceInline uint32_t abHash_FNV_Iteration(uint32_t hash, uint8_t byte) {
	return (hash ^ byte) * abHash_FNV_Prime;
}

inline uint32_t abHash_FNV_Raw(uint8_t const * data, size_t size) {
	uint32_t hash = abHash_FNV_Base;
	while (size > 0u) {
		hash = abHash_FNV_Iteration(hash, *(data++));
	}
	return hash;
}

inline uint32_t abHash_FNV_TextA(char const * text) {
	uint32_t hash = abHash_FNV_Base;
	while (*text != '\0') {
		hash = abHash_FNV_Iteration(hash, (uint8_t) *(text++));
	}
	return hash;
}

inline uint32_t abHash_FNV_TextACaseless(char const * text) {
	uint32_t hash = abHash_FNV_Base;
	char character;
	while ((character = *(text++)) != '\0') {
		if (character >= 'A' && character <= 'Z') {
			character += 'a' - 'A';
		}
		hash = abHash_FNV_Iteration(hash, (uint8_t) character);
	}
	return hash;
}

#endif
