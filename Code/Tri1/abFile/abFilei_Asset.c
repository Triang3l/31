#include "abFilei.h"
#include "../abData/abHash.h"
#include "../abData/abText.h"
#include "../abFeedback/abFeedback.h"

abHashMap abFilei_Asset_Map;

// Dynamically allocated list of asset paths, for more compact storage.
// Strings are immutable and never deallocated.
// There may be duplicates if assets of different types have the same local path.

typedef struct abFilei_Asset_PathList_Item {
	// Paths are prefixed with 2 bytes: asset type and path length (for reverse comparison, as many assets can be in the same directory).
	char paths[128u * abFile_Asset_MaxPathSize];
	struct abFilei_Asset_PathList_Item * next;
} abFilei_Asset_PathList_Item;

static abFilei_Asset_PathList_Item * abFilei_Asset_PathList = abNull;
unsigned int abFilei_Asset_PathList_UsedInCurrent;

uint32_t abFilei_Asset_Map_KeyLocator_Hash(void const * key, unsigned int keySize) {
	return abHash_FNV_TextA(*((char const * const *) key) + 2u);
}

abBool abFilei_Asset_Map_KeyLocator_Compare(void const * storedKey, void const * newKey, unsigned int keySize) {
	uint8_t const * storedKeyBytes = *((uint8_t const * const *) storedKey);
	uint8_t const * newKeyBytes = *((uint8_t const * const *) newKey);
	if (storedKeyBytes[0u] != newKeyBytes[0u]) {
		return abFalse;
	}
	unsigned int length = storedKeyBytes[1u];
	if (length != newKeyBytes[1u]) {
		return abFalse;
	}
	// Comparing from the end because many assets, especially meshes and images, may have a common prefix.
	while (length != 0u) {
		if (storedKeyBytes[length + (2u - 1u)] != newKeyBytes[length + (2u - 1u)]) {
			return abFalse;
		}
		--length;
	}
	return abTrue;
}

void abFilei_Asset_Map_KeyLocator_Copy(void * location, void const * key, unsigned int keySize) {
	*((uint8_t const * *) location) = *((uint8_t const * const *) key);
}

static abHashMap_KeyLocator const abFilei_Asset_Map_KeyLocator = {
	.hash = abFilei_Asset_Map_KeyLocator_Hash,
	.compare = abFilei_Asset_Map_KeyLocator_Compare,
	.copy = abFilei_Asset_Map_KeyLocator_Copy
};

void abFilei_Asset_InitSystem() {
	// The initial capacity should preferably be bigger than the total number of assets in the game. Expansion will cause a MASSIVE rehash.
	abHashMap_Init(&abFilei_Asset_Map, abFilei_MemoryTag, abFalse, &abFilei_Asset_Map_KeyLocator,
			sizeof(uint8_t *), sizeof(abFilei_Asset), 1024u);
	abFilei_Asset_PathList = abNull;
}

void abFilei_Asset_ShutdownSystem() {
	abHashMap_Destroy(&abFilei_Asset_Map);
	while (abFilei_Asset_PathList != abNull) {
		abFilei_Asset_PathList_Item * nextPathListItem = abFilei_Asset_PathList;
		abMemory_Free(abFilei_Asset_PathList);
		abFilei_Asset_PathList = nextPathListItem;
	}
}

unsigned int abFile_Asset_ClearPath(char const * path, char pathClean[abFile_Asset_MaxPathSize]) {
	// - No slashes in the beginning or the end.
	// - Only one forward slash as a path delimiter.
	// - No dots - prevent .., also dots have a reserved meaning - purely for file extensions or sub-extension.
	char const * source = path;
	char character;
	unsigned int written = 0u;
	abBool writeSlash = abFalse;
	while ((character = *(source++)) != '\0') {
		if (character == '.') {
			continue;
		}
		if (character == '/' || character == '\\') {
			if (written != 0u) {
				writeSlash = abTrue;
			}
			continue;
		}
		if (character >= 'A' && character <= 'Z') {
			character += 'a' - 'A';
		}
		if (writeSlash) {
			if (written >= (abFile_Asset_MaxPathSize - 3u)) {
				break;
			}
			pathClean[written++] = '/';
		}
		pathClean[written++] = character;
		if (written >= (abFile_Asset_MaxPathSize - 1u)) {
			break;
		}
	}
	pathClean[written] = '\0';
	return written;
}

abFile_AssetHandle abFile_Asset_Register(abFile_Asset_Type type, char const * path) {
	if ((unsigned int) type >= abFile_Asset_Type_Count) {
		abFeedback_Crash("abFile_Asset_Register", "Tried to register asset %s of unknown type %u.", path, (unsigned int) type);
	}

	// The key stored in the map is a POINTER to the path, not the path itself!
	// So a temporary pointer is created for hashing and comparison, and later replaced with the real one.
	char pathPrefixed[abFile_Asset_MaxPathSize + 2u], * pathTemporaryPointer = pathPrefixed;
	pathPrefixed[0u] = (char) type + 1;
	unsigned int pathCleanLength = abFile_Asset_ClearPath(path, pathPrefixed + 2u);
	pathPrefixed[1u] = (char) pathCleanLength;

	abBool assetIsNew;
	abFile_AssetHandle handle = abHashMap_FindIndexWrite(&abFilei_Asset_Map, &pathTemporaryPointer, &assetIsNew);
	if (!assetIsNew) {
		return handle;
	}

	abFeedback_StaticAssert(sizeof(abFilei_Asset_PathList->paths) >= sizeof(pathPrefixed),
			"Asset path list items must be large enough to contain at least one prefixed asset path.");
	if (abFilei_Asset_PathList == abNull) {
		abFilei_Asset_PathList = abMemory_Alloc(abFilei_MemoryTag, sizeof(abFilei_Asset_PathList_Item), abFalse);
		abFilei_Asset_PathList->next = abNull;
		abFilei_Asset_PathList_UsedInCurrent = 0u;
	}
	unsigned int pathSizeToAdd = pathCleanLength + 3u;
	if (pathSizeToAdd > (abArrayLength(abFilei_Asset_PathList->paths) - abFilei_Asset_PathList_UsedInCurrent)) {
		abFilei_Asset_PathList_Item * newItem = abMemory_Alloc(abFilei_MemoryTag, sizeof(abFilei_Asset_PathList_Item), abFalse);
		newItem->next = abFilei_Asset_PathList;
		abFilei_Asset_PathList = newItem;
		abFilei_Asset_PathList_UsedInCurrent = 0u;
	}
	char * pathPersistentPointer = abFilei_Asset_PathList->paths + abFilei_Asset_PathList_UsedInCurrent;
	memcpy(pathPersistentPointer, pathPrefixed, pathSizeToAdd);
	abFilei_Asset_PathList_UsedInCurrent += pathSizeToAdd;
	abHashMap_SetPersistentKey(&abFilei_Asset_Map, handle, &pathPersistentPointer);

	abFilei_Asset * asset = abHashMap_GetValue(&abFilei_Asset_Map, handle);
	asset->state = abFilei_Asset_State_Unloaded;
	asset->data = abNull;

	return handle;
}
