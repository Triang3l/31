#include "abHashMap.h"
#include "abHash.h"
#include "../abData/abText.h"
#include "../abFeedback/abFeedback.h"

// Returns the table, indexed with the hash value, with the beginnings of the collision resolution lists.
static abForceInline uint32_t * abHashMapi_GetFirstIndices(abHashMap * hashMap) {
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

	// Get the beginning of the list and check if there's any value with such hash.
	unsigned int index = abHashMapi_GetFirstIndices(hashMap)[keyLocator->hash(key, hashMap->keySize) & (hashMap->capacity - 1u)];
	if (index == abHashMap_InvalidIndex) {
		return abHashMap_InvalidIndex;
	}

	uint32_t const * nextIndices = abHashMapi_GetNextIndices(hashMap);
	uint8_t const * keys = (uint8_t const *) abHashMap_GetKeys(hashMap);
	unsigned int keySize = hashMap->keySize;

	// Resolve collisions.
	while (index != abHashMap_InvalidIndex && !keyLocator->compare(keys + (size_t) index * keySize, key, keySize)) {
		index = nextIndices[index];
	}
	return index;
}

static void abHashMapi_ChangeCapacity(abHashMap * hashMap, unsigned int newCapacity) {
	abFeedback_Assert((newCapacity & (newCapacity - 1u)) == 0u, "abHashMapi_ChangeCapacity",
			"New capacity must be a power of 2 (tried to change to %u from %u).", newCapacity, hashMap->capacity);
	uint8_t * newMemory = abMemory_Alloc(hashMap->memoryTag, (size_t) newCapacity * (2u * sizeof(unsigned int)) +
			abAlign(newCapacity * hashMap->keySize, (size_t) 16u) + newCapacity * hashMap->valueSize, abTrue);
	memset(newMemory, 0xff, (size_t) newCapacity * (2u * sizeof(unsigned int))); // Reset indices and next links.

	uint8_t * oldMemory = hashMap->memory;
	void * oldKeys = abHashMap_GetKeys(hashMap), * oldValues = abHashMap_GetValues(hashMap);
	hashMap->memory = newMemory;
	hashMap->capacity = newCapacity;

	unsigned int count = hashMap->count;
	if (count != 0u) {
		abHashMap_KeyLocator const * keyLocator = hashMap->keyLocator;
		unsigned int * newFirstIndices = abHashMapi_GetFirstIndices(hashMap), * newNextIndices = abHashMapi_GetNextIndices(hashMap);
		uint8_t * newKeys = (uint8_t *) abHashMap_GetKeys(hashMap);
		unsigned int keySize = hashMap->keySize;

		memcpy(newKeys, oldKeys, (size_t) count * keySize);
		memcpy(abHashMap_GetValues(hashMap), oldValues, (size_t) count * hashMap->valueSize);

		unsigned int hashMask = newCapacity - 1u;
		for (unsigned int index = 0u; index < count; ++index) {
			unsigned int * newFirstIndexLocation = &(newFirstIndices[keyLocator->hash(newKeys + (size_t) index * keySize, keySize) & hashMask]);
			newNextIndices[index] = *newFirstIndexLocation;
			*newFirstIndexLocation = index;
		}
	}

	abMemory_Free(oldMemory);
}

unsigned int abHashMap_FindIndexWrite(abHashMap * hashMap, void const * key, abBool * isNew) {
	if (hashMap->capacity == 0u) {
		abHashMapi_ChangeCapacity(hashMap, hashMap->minimumCapacity);
	}

	abHashMap_KeyLocator const * keyLocator = hashMap->keyLocator;
	unsigned int * firstIndices = abHashMapi_GetNextIndices(hashMap), * nextIndices = abHashMapi_GetNextIndices(hashMap);
	void const * keys = abHashMap_GetKeys(hashMap);
	unsigned int keySize = hashMap->keySize;

	// Find the existing entry if there's any.
	uint32_t hash = hashMap->keyLocator->hash(key, keySize);
	unsigned int index = firstIndices[hash & (hashMap->capacity - 1u)];
	while (index != abHashMap_InvalidIndex) {
		if (keyLocator->compare((uint8_t const *) keys + (size_t) index * keySize, key, keySize)) {
			if (isNew != abNull) {
				*isNew = abFalse;
			}
			return index;
		}
		index = nextIndices[index];
	}

	// If no free entry, expand the storage.
	if (hashMap->count >= hashMap->capacity) {
		if (hashMap->capacity >= (unsigned int) INT_MAX + 1u) {
			abFeedback_Crash("abHashMap_FindIndexWrite", "Too many entries in a hash map.");
		}
		abHashMapi_ChangeCapacity(hashMap, hashMap->capacity << 1u);
		firstIndices = abHashMapi_GetFirstIndices(hashMap);
		nextIndices = abHashMapi_GetNextIndices(hashMap);
	}

	// Add a new entry.
	index = hashMap->count++;
	keyLocator->copy((uint8_t *) abHashMap_GetKeys(hashMap) + (size_t) index * keySize, key, keySize);
	unsigned int * firstIndexLocation = &(firstIndices[hash & (hashMap->capacity - 1u)]);
	nextIndices[index] = *firstIndexLocation;
	*firstIndexLocation = index;
	if (isNew != abNull) {
		*isNew = abTrue;
	}
	return index;
}

void abHashMap_RemoveIndex(abHashMap * hashMap, unsigned int index) {
	if (index >= hashMap->count) {
		return;
	}

	abHashMap_KeyLocator const * keyLocator = hashMap->keyLocator;
	unsigned int * firstIndices = abHashMapi_GetFirstIndices(hashMap), * nextIndices = abHashMapi_GetNextIndices(hashMap);

	// Unlink the entry.
	void * key = abHashMap_GetKey(hashMap, index);
	unsigned int * firstIndexLocation = &(firstIndices[keyLocator->hash(key, hashMap->keySize) & (hashMap->capacity - 1u)]);
	if (*firstIndexLocation == index) {
		*firstIndexLocation = nextIndices[index];
	} else {
		unsigned int previousIndex = *firstIndexLocation;
		while (nextIndices[previousIndex] != index) {
			previousIndex = nextIndices[previousIndex];
		}
		nextIndices[previousIndex] = nextIndices[index];
	}

	// Move the last entry to the free space.
	if (index + 1u < hashMap->count) {
		unsigned int lastIndex = hashMap->count - 1u;
		void * lastKey = abHashMap_GetKey(hashMap, lastIndex);
		memcpy(key, lastKey, hashMap->keySize);
		memcpy(abHashMap_GetValue(hashMap, index), abHashMap_GetValue(hashMap, lastIndex), hashMap->valueSize);
		firstIndexLocation = &(firstIndices[keyLocator->hash(lastKey, hashMap->keySize) & (hashMap->capacity - 1u)]);
		if (*firstIndexLocation == lastIndex) {
			*firstIndexLocation = index;
		} else {
			unsigned int previousIndex = *firstIndexLocation;
			while (nextIndices[previousIndex] != lastIndex) {
				previousIndex = nextIndices[previousIndex];
			}
			nextIndices[previousIndex] = index;
		}
	}

	--(hashMap->count);
}

abBool abHashMap_Remove(abHashMap * hashMap, void const * key) {
	if (hashMap->count == 0u) {
		return abFalse;
	}

	abHashMap_KeyLocator const * keyLocator = hashMap->keyLocator;
	unsigned int * firstIndices = abHashMapi_GetFirstIndices(hashMap), * nextIndices = abHashMapi_GetNextIndices(hashMap);
	uint8_t * keys = (uint8_t *) abHashMap_GetKeys(hashMap);
	unsigned int keySize = hashMap->keySize;

	// Find the entry and unlink it if found.
	unsigned int * firstIndexLocation = &(firstIndices[keyLocator->hash(key, hashMap->keySize) & (hashMap->capacity - 1u)]);
	unsigned int previousIndex = abHashMap_InvalidIndex, index = *firstIndexLocation;
	while (index != abHashMap_InvalidIndex) {
		if (keyLocator->compare(keys + (size_t) index * keySize, key, keySize)) {
			break;
		}
		previousIndex = index;
		index = nextIndices[index];
	}
	if (index == abHashMap_InvalidIndex) {
		return abFalse;
	}
	if (previousIndex != abHashMap_InvalidIndex) {
		*firstIndexLocation = nextIndices[index];
	} else {
		nextIndices[previousIndex] = nextIndices[index];
	}

	// Move the last entry to the free space.
	if (index + 1u < hashMap->count) {
		unsigned int lastIndex = hashMap->count - 1u;
		void * lastKey = abHashMap_GetKey(hashMap, lastIndex);
		memcpy(keys + (size_t) index * keySize, lastKey, hashMap->keySize);
		memcpy(abHashMap_GetValue(hashMap, index), abHashMap_GetValue(hashMap, lastIndex), hashMap->valueSize);
		firstIndexLocation = &(firstIndices[keyLocator->hash(lastKey, hashMap->keySize) & (hashMap->capacity - 1u)]);
		if (*firstIndexLocation == lastIndex) {
			*firstIndexLocation = index;
		} else {
			previousIndex = *firstIndexLocation;
			while (nextIndices[previousIndex] != lastIndex) {
				previousIndex = nextIndices[previousIndex];
			}
			nextIndices[previousIndex] = index;
		}
	}

	--(hashMap->count);
	return abTrue;
}

/***************
 * Key locators
 ***************/

uint32_t abHashMap_KeyLocator_Raw_Hash(void const * key, unsigned int keySize) {
	return abHash_FNV_Raw(key, keySize);
}

abBool abHashMap_KeyLocator_Raw_Compare(void const * storedKey, void const * newKey, unsigned int keySize) {
	return memcmp(storedKey, newKey, keySize) == 0;
}

void abHashMap_KeyLocator_Raw_Copy(void * location, void const * key, unsigned int keySize) {
	memcpy(location, key, keySize);
}

abHashMap_KeyLocator const abHashMap_KeyLocator_Raw = {
	.hash = abHashMap_KeyLocator_Raw_Hash,
	.compare = abHashMap_KeyLocator_Raw_Compare,
	.copy = abHashMap_KeyLocator_Raw_Copy
};

uint32_t abHashMap_KeyLocator_TextA_Hash(void const * key, unsigned int keySize) {
	char const * text = (char const *) key;
	uint32_t hash = abHash_FNV_Base;
	while ((*text != '\0') && ((unsigned int) (text - (char const *) key) < keySize)) {
		hash = abHash_FNV_Iteration(hash, (uint8_t) *(text++));
	}
	return hash;
}

abBool abHashMap_KeyLocator_TextA_Compare(void const * storedKey, void const * newKey, unsigned int keySize) {
	return abTextA_ComparePart((char const *) storedKey, (char const *) newKey, keySize);
}

void abHashMap_KeyLocator_TextA_Copy(void const * location, void const * key, unsigned int keySize) {
	abTextA_Copy((char *) location, keySize, (char const *) key);
}

abHashMap_KeyLocator const abHashMap_KeyLocator_TextA = {
	.hash = abHashMap_KeyLocator_TextA_Hash,
	.compare = abHashMap_KeyLocator_TextA_Compare,
	.copy = abHashMap_KeyLocator_TextA_Copy
};
