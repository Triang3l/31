#ifndef abInclude_abFile
#define abInclude_abFile
#include "../abCommon.h"

typedef unsigned int abFile_AssetHandle;
typedef unsigned int abFile_GroupHandle;

#define abFile_GroupHandle_Invalid UINT_MAX

typedef enum abFile_Asset_Type {
	abFile_Asset_Type_Mesh,
		abFile_Asset_Type_Count
} abFile_Asset_Type;

#define abFile_Asset_MaxPathSize 96u // Can't be larger than 255 due to name length being stored as a char.

typedef struct abFile_GroupAsset {
	abFile_AssetHandle asset; // Set before registering a group.

	// Linked list of groups for each asset. Asset index indicates where the GroupAsset is located in the previous/next group.
	abFile_GroupHandle i_groupOlder, i_groupNewer;
	unsigned int i_groupOlderAssetIndex, i_groupNewerAssetIndex;
} abFile_GroupAsset;

void abFile_Init();
void abFile_Update();
void abFile_Shutdown();

// Returns the new path length, always <= original length. Both arguments may point to the same string.
unsigned int abFile_Asset_ClearPath(char const * path, char pathClean[abFile_Asset_MaxPathSize]);
abFile_AssetHandle abFile_Asset_Register(abFile_Asset_Type type, char const * path);

#endif
