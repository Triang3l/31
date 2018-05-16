#ifndef abInclude_abFilei
#define abInclude_abFilei
#include "abFile.h"
#include "../abData/abArray2L.h"
#include "../abData/abHashMap.h"
#include "../abMemory/abMemory.h"

extern abMemory_Tag * abFilei_MemoryTag;

/*********
 * Assets
 *********/

// Don't touch when the state is Ext_ (externally owned)!
typedef enum abFilei_Asset_State {
	abFilei_Asset_State_Unloaded, // Totally not loaded - data may or may not be null
	abFilei_Asset_State_Corrupt, // Corrupt - don't try to load again.
	abFilei_Asset_State_Ext_Loading, // Loading on a separate thread.
	abFilei_Asset_State_UsageNotSwitched, // Needs GPU usage switch from copy to graphics.
	abFilei_Asset_State_Ext_UsageSwitching, // GPU usage switching - don't touch!
	abFilei_Asset_State_Loaded // Fully usable.
} abFilei_Asset_State;

typedef struct abFilei_Asset {
	void * data; // abFile_Mesh, etc., depending on asset type.
	abFilei_Asset_State state; // The type and the name is the key in the global asset map!

	abFile_GroupHandle groupNewest; // Beginning of the linked list of group references.
	unsigned int groupNewestAssetIndex; // Index of the GroupAsset in groupNewest.
} abFilei_Asset;

extern abHashMap abFilei_Asset_Map; // Prefixed path pointer (NOT path itself) -> asset structure.

abForceInline abFilei_Asset * abFilei_Asset_Get(abFile_AssetHandle handle) {
	return (abFilei_Asset *) abHashMap_GetValue(&abFilei_Asset_Map, handle);
}
abForceInline abFile_Asset_Type abFilei_Asset_GetType(abFile_AssetHandle handle) {
	return (abFile_Asset_Type) (**((uint8_t const * const *) abHashMap_GetKey(&abFilei_Asset_Map, handle)) - 1u);
}
abForceInline char const * abFilei_Asset_GetPath(abFile_AssetHandle handle) {
	return *((char const * const *) abHashMap_GetKey(&abFilei_Asset_Map, handle)) + 2u;
}
abForceInline abFile_AssetHandle abFilei_Asset_GetHandle(abFilei_Asset const * asset) {
	return (abFile_AssetHandle) (asset - (abFilei_Asset const *) abHashMap_GetValues(&abFilei_Asset_Map));
}

void abFilei_Asset_InitSystem();
void abFilei_Asset_ShutdownSystem();

/*********
 * Groups
 *********/

typedef enum abFilei_Group_State {
	abFilei_Group_State_Inactive,
	abFilei_Group_State_Activating,
	abFilei_Group_State_Active // Ready for use (though some assets may be missing).
} abFilei_Group_State;

typedef struct abFilei_Group {
	abFile_GroupAsset * assets; // Allocated externally, but must be persistent as long as the group is registered!
	unsigned int assetCount;

	abFile_GroupHandle handle; // Back reference to the handle.

	abFilei_Group_State state;
	unsigned int assetFailedCount; // If Active or Activating, that's the number of assets that have failed to load.

	union {
		abFile_GroupHandle freeNext; // Next handle in the free list (or an invalid handle).
	} special;
} abFilei_Group;

extern abArray2L abFilei_Group_Array;

abForceInline abFilei_Group * abFilei_Group_Get(abFile_GroupHandle handle) {
	return (abFilei_Group *) abArray2L_Get(&abFilei_Group_Array, handle);
}

// To be called by tracking functions - very low-level, basically resetting the structure and linking to assets.
abFile_GroupHandle abFilei_Group_Register(abFile_GroupAsset * assets, unsigned int assetCount);
void abFilei_Group_Unregister(abFile_GroupHandle handle);

/*****************
 * Loader threads
 *****************/

void abFilei_Loader_Init();
void abFilei_Loader_Shutdown();
unsigned int abFilei_Loader_GetFreeRequestCount();
abBool abFilei_Loader_RequestAssetLoad(abFile_AssetHandle handle, unsigned int gpuUploaderIndex);

#endif
