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
	uint32_t (* hash)(void const * key, unsigned int keySize);
	abBool (* compare)(void const * storedKey, void const * newKey, unsigned int keySize);
	void (* copy)(void * location, void const * key, unsigned int keySize);
} abHashMap_KeyLocator;

typedef struct abHashMap {
	abMemory_Tag * memoryTag;
	uint8_t * memory; // Storage for all the used tables.
	size_t keyOffset, valueOffset; // Offsets of the keys and the values in the memory (relocatable).
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
	hashMap->memory = abNull;
	hashMap->keyOffset = hashMap->valueOffset = 0u;
	hashMap->keyLocator = keyLocator;
	hashMap->keySize = (unsigned int) keySize;
	hashMap->valueSize = (unsigned int) valueSize;
	hashMap->capacity = 0u;
	hashMap->minimumCapacity = abBit_ToNextPO2SaturatedU32(abMax(minimumCapacity, 8u));
	hashMap->count = 0u;
}

abForceInline void * abHashMap_GetKeys(abHashMap * hashMap) {
	return hashMap->memory + hashMap->keyOffset;
}
abForceInline void * abHashMap_GetKey(abHashMap * hashMap, unsigned int index) {
	return (uint8_t *) abHashMap_GetKeys(hashMap) + (size_t) index * hashMap->keySize;
}

abForceInline void * abHashMap_GetValues(abHashMap * hashMap) {
	return hashMap->memory + hashMap->valueOffset;
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

// Useful when the key is a pointer and need to add a new entry - can search with a stack-allocated key, but then make it persistent.
// The new key must have the same hash as the old one!
inline void abHashMap_SetPersistentKey(abHashMap * hashMap, unsigned int index, void const * newKey) {
	hashMap->keyLocator->copy(abHashMap_GetKey(hashMap, index), newKey, hashMap->keySize);
}

void abHashMap_RemoveIndex(abHashMap * hashMap, unsigned int index);
abBool abHashMap_Remove(abHashMap * hashMap, void const * key);

abForceInline abHashMap_Destroy(abHashMap * hashMap) {
	abMemory_Free(hashMap->memory);
}

/***************
 * Key locators
 ***************/

uint32_t abHashMap_KeyLocator_Raw_Hash(void const * key, unsigned int keySize);
abBool abHashMap_KeyLocator_Raw_Compare(void const * storedKey, void const * newKey, unsigned int keySize);
void abHashMap_KeyLocator_Raw_Copy(void * location, void const * key, unsigned int keySize);
extern abHashMap_KeyLocator const abHashMap_KeyLocator_Raw;

uint32_t abHashMap_KeyLocator_TextA_Hash(void const * key, unsigned int keySize);
abBool abHashMap_KeyLocator_TextA_Compare(void const * storedKey, void const * newKey, unsigned int keySize);
void abHashMap_KeyLocator_TextA_Copy(void * location, void const * key, unsigned int keySize);
extern abHashMap_KeyLocator const abHashMap_KeyLocator_TextA;

#endif
