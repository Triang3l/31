#include "abFilei.h"

abArray2L abFilei_Group_Array;
static abFile_GroupHandle abFilei_Group_FreeFirst;

void abFilei_Group_InitSystem() {
	abArray2L_Init(&abFilei_Group_Array, abFilei_MemoryTag, sizeof(abFilei_Group), 128u);
	abFilei_Group_FreeFirst = abFile_GroupHandle_Invalid;
}

abFile_GroupHandle abFilei_Group_Register(abFile_GroupAsset * assets, unsigned int assetCount) {
	abFile_GroupHandle handle;
	abFilei_Group * group;
	if (abFilei_Group_FreeFirst != abFile_GroupHandle_Invalid) {
		handle = abFilei_Group_FreeFirst;
		group = abFilei_Group_Get(handle);
		abFilei_Group_FreeFirst = group->special.freeNext;
	} else {
		handle = abFilei_Group_Array.elementCount;
		abArray2L_Resize(&abFilei_Group_Array, handle + 1u);
		group = abFilei_Group_Get(handle);
	}

	group->assets = assets;
	group->assetCount = assetCount;
	group->handle = handle;

	group->state = abFilei_Group_State_Inactive;
	group->assetFailedCount = 0u;

	for (unsigned int assetIndex = 0u; assetIndex < group->assetCount; ++assetIndex) {
		abFile_GroupAsset * assetReference = &assets[assetIndex];
		abFilei_Asset * asset = abFilei_Asset_Get(assetReference->asset);
		assetReference->i_groupNewer = abFile_GroupHandle_Invalid;
		assetReference->i_groupOlder = asset->groupNewest;
		assetReference->i_groupOlderAssetIndex = asset->groupNewestAssetIndex;
		asset->groupNewest = handle;
		asset->groupNewestAssetIndex = assetIndex;
	}

	return handle;
}

void abFilei_Group_Unregister(abFile_GroupHandle handle) {
	abFilei_Group * group = abFilei_Group_Get(handle);

	abFile_GroupAsset * assetReferences = group->assets;
	unsigned int assetCount = group->assetCount;
	for (unsigned int assetIndex = 0u; assetIndex < group->assetCount; ++assetIndex) {
		abFile_GroupAsset * assetReference = &group->assets[assetIndex];
		if (assetReference->i_groupNewer != abFile_GroupHandle_Invalid) {
			abFile_GroupAsset * assetReferenceNewer = &abFilei_Group_Get(assetReference->i_groupNewer)
					->assets[assetReference->i_groupNewerAssetIndex];
			assetReferenceNewer->i_groupOlder = assetReference->i_groupOlder;
			assetReferenceNewer->i_groupOlderAssetIndex = assetReference->i_groupOlderAssetIndex;
		} else {
			abFilei_Asset * asset = abFilei_Asset_Get(assetReference->asset);
			asset->groupNewest = assetReference->i_groupOlder;
			asset->groupNewestAssetIndex = assetReference->i_groupOlderAssetIndex;
		}
		if (assetReference->i_groupOlder != abFile_GroupHandle_Invalid) {
			abFile_GroupAsset * assetReferenceOlder = &abFilei_Group_Get(assetReference->i_groupOlder)
					->assets[assetReference->i_groupOlderAssetIndex];
			assetReferenceOlder->i_groupNewer = assetReference->i_groupNewer;
			assetReferenceOlder->i_groupNewerAssetIndex = assetReference->i_groupNewerAssetIndex;
		}
	}

	group->special.freeNext = abFilei_Group_FreeFirst;
	abFilei_Group_FreeFirst = handle;
}
