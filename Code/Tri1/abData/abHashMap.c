#include "abHashMap.h"
#include "../abFeedback/abFeedback.h"

// Returns the table, indexed with the hash value, with the indices of the entries.
static abForceInline uint32_t * abHashMapi_GetIndices(abHashMap * hashMap) {
	return (uint32_t *) hashMap->memory;
}

// Returns the collision resolution list, indexed with entry indices.
static abForceInline uint32_t * abHashMapi_GetNextIndices(abHashMap * hashMap) {
	return (uint32_t *) hashMap->memory + hashMap->capacity;
}

unsigned int abHashMap_FindIndexRead(abHashMap * hashMap, void const * key) {
	if (hashMap->count == 0u) {
		return abHashMap_InvalidIndex;
	}
	abHashMap_KeyLocator const * keyLocator = hashMap->keyLocator;
	uint32_t hash = hashMap->keyLocator->hashKey(key, hashMap->keySize);
	unsigned int index = abHashMapi_GetIndices(hashMap)[hash & (hashMap->capacity - 1u)];
	if (index == abHashMap_InvalidIndex) {
		return abHashMap_InvalidIndex;
	}
	void const * keys = abHashMap_GetKeys(hashMap);
	unsigned int keySize = hashMap->keySize;
	uint32_t const * nextIndices = abHashMapi_GetNextIndices(hashMap);
	while (index != abHashMap_InvalidIndex) {
		if (keyLocator->compareKeys((uint8_t const *) keys + (size_t) index * keySize, key, keySize)) {
			return index;
		}
		index = nextIndices[index];
	}
	return index;
}

static void abHashMapi_ChangeCapacity(abHashMap * hashMap, unsigned int newCapacity) {
	abFeedback_Assert((newCapacity & (newCapacity - 1u)) == 0u, "abHashMapi_ChangeCapacity",
			"New capacity must be a power of 2 (tried to change to %u from %u).", newCapacity, hashMap->capacity);
	size_t newCapacityAsSize = (size_t) newCapacity;
	uint8_t * newMemory = abMemory_Alloc(hashMap->memoryTag, newCapacityAsSize * (2u * sizeof(unsigned int)) +
			abAlign(newCapacity * hashMap->keySize, (size_t) 16u) + newCapacity * hashMap->valueSize, abTrue);
	memset(newMemory, 0xff, newCapacityAsSize * (2u * sizeof(unsigned int))); // Reset indices and next links.

	uint8_t * oldMemory = hashMap->memory;
	void * oldKeys = abHashMap_GetKeys(hashMap), * oldValues = abHashMap_GetValues(hashMap);
	hashMap->memory = newMemory;
	hashMap->capacity = newCapacity;

	unsigned int count = hashMap->count;
	if (count != 0u) {
		abHashMap_KeyLocator const * keyLocator = hashMap->keyLocator;
		unsigned int keySize = hashMap->keySize;
		uint8_t * newKeys = (uint8_t *) abHashMap_GetKeys(hashMap);
		memcpy(newKeys, oldKeys, (size_t) count * keySize);

		memcpy(abHashMap_GetValues(hashMap), oldValues, (size_t) count * hashMap->valueSize);

		unsigned int hashMask = newCapacity - 1u;
		unsigned int * newIndices = abHashMapi_GetIndices(hashMap);
		unsigned int * newNextIndices = abHashMapi_GetNextIndices(hashMap);
		for (unsigned int index = 0u; index < count; ++index) {
			unsigned int * indexLocation = &newIndices[keyLocator->hashKey(newKeys + (size_t) index * keySize, keySize) & hashMask];
			newNextIndices[index] = *indexLocation;
			*indexLocation = index;
		}
	}

	abMemory_Free(oldMemory);
}

unsigned int abHashMap_FindIndexWrite(abHashMap * hashMap, void const * key, abBool * isNew) {
	if (hashMap->capacity == 0u) {
		abHashMapi_ChangeCapacity(hashMap, hashMap->minimumCapacity);
	}

	abHashMap_KeyLocator const * keyLocator = hashMap->keyLocator;
	void const * keys = abHashMap_GetKeys(hashMap);
	unsigned int keySize = hashMap->keySize;

	uint32_t hash = hashMap->keyLocator->hashKey(key, keySize);
	unsigned int index = abHashMapi_GetIndices(hashMap)[hash & (hashMap->capacity - 1u)];
	while (index != abHashMap_InvalidIndex) {
		if (keyLocator->compareKeys((uint8_t const *) keys + (size_t) index * keySize, key, keySize)) {
			if (isNew != abNull) {
				*isNew = abFalse;
			}
			return index;
		}
	}

	if (hashMap->count >= hashMap->capacity) {
		if (hashMap->capacity >= (unsigned int) INT_MAX + 1u) {
			abFeedback_Crash("abHashMap_FindIndexWrite", "Too many entries in a hash map.");
		}
		abHashMapi_ChangeCapacity(hashMap->capacity, hashMap->capacity << 1u);
	}

	index = hashMap->count++;
	keyLocator->copyKey((uint8_t *) abHashMap_GetKeys(hashMap) + (size_t) index * keySize, key, keySize);
	unsigned int * indexLocation = &(abHashMapi_GetIndices(hashMap)[hash & (hashMap->capacity - 1u)]);
	abHashMapi_GetNextIndices(hashMap)[index] = *indexLocation;
	*indexLocation = index;
	if (isNew != abNull) {
		*isNew = abTrue;
	}
	return index;
}
