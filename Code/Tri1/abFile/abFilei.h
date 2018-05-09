#ifndef abInclude_abFilei
#define abInclude_abFilei
#include "abFile.h"
#include "../abData/abHashMap.h"
#include "../abMemory/abMemory.h"

extern abMemory_Tag * abFilei_MemoryTag;

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
	// The type and the name is the key in the global asset map!
	abFilei_Asset_State state;
	void * data; // abFile_Mesh, etc., depending on asset type.
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

#endif
