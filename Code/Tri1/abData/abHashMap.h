#ifndef abInclude_abData_abHashMap
#define abInclude_abData_abHashMap
#include "../abMath/abBit.h"
#include "../abMemory/abMemory.h"

/*
 * Heavily inspired by btHashMap of Bullet.
 * Keys and values are stored sequentially, allowing for easier iteration.
 *
 * Bullet Continuous Collision Detection and Physics Library
 * Copyright (c) 2003-2009 Erwin Coumans http://bulletphysics.org
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it freely,
 * subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
 *    If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

typedef struct abHashMap_KeyLocator {
	uint32_t (* hashKey)(void const * key, unsigned int keySize);
	abBool (* compareKeys)(void const * storedKey, void const * newKey, unsigned int keySize);
	uint32_t (* copyKey)(void const * location, void const * key, unsigned int keySize);
} abHashMap_KeyLocator;

typedef struct abHashMap {
	abMemory_Tag * memoryTag;
	uint8_t * memory; // Storage for all the used tables.
	abHashMap_KeyLocator const * keyLocator;
	unsigned int keySize, valueSize;
	unsigned int capacity, minimumCapacity;
	unsigned int count;
} abHashMap;

#define abHashMap_InvalidIndex UINT_MAX

// 0 minimum capacity to let the implementation choose.
abForceInline void abHashMap_Init(abHashMap * hashMap, abMemory_Tag * memoryTag, abBool align16,
		abHashMap_KeyLocator const * keyLocator, size_t keySize, size_t valueSize, unsigned int minimumCapacity) {
	hashMap->memoryTag = memoryTag;
	hashMap->keyLocator = keyLocator;
	hashMap->keySize = (unsigned int) keySize;
	hashMap->valueSize = (unsigned int) valueSize;
	hashMap->capacity = 0u;
	minimumCapacity = abClamp(minimumCapacity, 8u, (unsigned int) INT_MAX + 1u);
	if ((minimumCapacity & (minimumCapacity - 1u)) != 0u) { // Round up to a power of 2.
		minimumCapacity = 1u << ((unsigned int) abBit_HighestOne32(minimumCapacity) + 1u);
	}
	hashMap->minimumCapacity = minimumCapacity;
	hashMap->count = 0u;
}

abForceInline void * abHashMap_GetKeys(abHashMap * hashMap) {
	return hashMap->memory + (size_t) hashMap->capacity * (2u * sizeof(uint32_t)); // Minimum capacity is enough to 16-align the keys anyway.
}

abForceInline void * abHashMap_GetValues(abHashMap * hashMap) {
	return (uint8_t *) abHashMap_GetKeys(hashMap) + abAlign((size_t) hashMap->capacity * hashMap->keySize, (size_t) 16u);
}
abForceInline void * abHashMap_GetValue(abHashMap * hashMap, unsigned int index) {
	return (uint8_t *) abHashMap_GetValues(hashMap) + (size_t) index * hashMap->valueSize;
}

unsigned int abHashMap_FindIndexRead(abHashMap * hashMap, void const * key);
inline void * abHashMap_FindRead(abHashMap * hashMap, void const * key) {
	unsigned int index = abHashMap_FindIndexRead(hashMap, key);
	return (index != abHashMap_InvalidIndex) ? abHashMap_GetValue(hashMap, index) : abNull;
}

unsigned int abHashMap_FindIndexWrite(abHashMap * hashMap, void const * key, /* optional */ abBool * isNew);
inline void * abHashMap_FindWrite(abHashMap * hashMap, void const * key, /* optional */ abBool * isNew) {
	return abHashMap_GetValue(hashMap, abHashMap_FindIndexWrite(hashMap, key, isNew));
}

void abHashMap_RemoveIndex(abHashMap * hashMap, unsigned int index);
abBool abHashMap_Remove(abHashMap * hashMap, void const * key);

abForceInline abHashMap_Destroy(abHashMap * hashMap) {
	abMemory_Free(hashMap->memory);
}

#endif
