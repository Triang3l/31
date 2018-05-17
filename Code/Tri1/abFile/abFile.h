#ifndef abInclude_abFile
#define abInclude_abFile
#include "../abCommon.h"

/*
 * The asset loader has two fundamental entities.
 *
 * An Asset roughly represents one file (though in some cases one asset may be built from multiple actual files).
 * It's the most low-level entity, usage tracking tasks aren't performed on assets directly.
 * Other systems access the actual data via the `data` pointer when the asset is in the usable state.
 *
 * A Watch is some kind of a reference to objects, such as a group of assets that need to be loaded fully.
 * It serves a two-way purpose:
 * -> It's the interface through which asset loading is actually triggered.
 *    Watches maintain two kinds of references to asset: passive and active.
 *    Loading of an asset is triggered once the number of active references becomes non-zero
 *    (it's not always possible to create an active reference - a load queue entry and possibly a GPU copier must be available).
 *    When the number of active references becomes zero, the asset is destroyed as quickly as possible (not necessarily instantly).
 *    This means that it's a responsibility of watches to, for instance, destroy unneeded world LODs when free memory is low.
 * <- It provides callbacks for asset state changes.
 *    One asset may be, for example, attached to multiple watches, so when it's loaded, all watches must know this fact.
 *
 * Both assets and watches are not game objects, they are internal to the loader.
 *
 * Assets expose their availability and data to the game, and are registered by the game (with initial watch reference count of 0).
 *
 * Watches, on the other hand, are created in a way opaque to the game, through other entities such as groups.
 * Some internal properties of watches are actually exposed to the game for various purposes such as memory allocation.
 * The game may create asset groups that contain abFile_WatchAsset instances, for example.
 */

typedef unsigned int abFile_AssetHandle;

typedef enum abFile_Asset_Type {
	abFile_Asset_Type_Mesh,
		abFile_Asset_Type_Count
} abFile_Asset_Type;

#define abFile_Asset_MaxPathSize 96u // Can't be larger than 256 due to name length being stored as a char.

typedef struct abFile_WatchAsset {
	// Contains linked lists of watches for each asset.
	// Asset index is the identifier of the WatchAsset within the previous/next watch, for O(1) iteration.

	// Pointers.
	struct abFilei_Watch * i_watchOlder, * i_watchNewer;

	// Integers.
	abFile_AssetHandle asset; // Must be set (externally) before starting tracking.
	unsigned int i_watchOlderAssetIndex, i_watchNewerAssetIndex;

	// Booleans.
	abBool i_referenceActive;
} abFile_WatchAsset;

void abFile_Init();
void abFile_Update();
void abFile_Shutdown();

// Returns the new path length, always <= original length. Both arguments may point to the same string.
unsigned int abFile_Asset_ClearPath(char const * path, char pathClean[abFile_Asset_MaxPathSize]);
abFile_AssetHandle abFile_Asset_Register(abFile_Asset_Type type, char const * path);

#endif
