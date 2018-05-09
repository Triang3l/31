#ifndef abInclude_abFile
#define abInclude_abFile
#include "../abCommon.h"

typedef unsigned int abFile_AssetHandle;

typedef enum abFile_Asset_Type {
	abFile_Asset_Type_Mesh,
		abFile_Asset_Type_Count
} abFile_Asset_Type;

#define abFile_Asset_MaxPathSize 96u // Can't be larger than 255 due to name length being stored as a char.

void abFile_Init();
void abFile_Update();
void abFile_Shutdown();

// Returns the new path length, always <= original length. Both arguments may point to the same string.
unsigned int abFile_Asset_ClearPath(char const * path, char pathClean[abFile_Asset_MaxPathSize]);
abFile_AssetHandle abFile_Asset_Register(abFile_Asset_Type type, char const * path);

#endif
