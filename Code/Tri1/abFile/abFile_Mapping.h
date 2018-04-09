#ifndef abInclude_abFile_Mapping
#define abInclude_abFile_Mapping
#include "../abData/abText.h"
#if defined(abPlatform_OS_Windows)
#include <Windows.h>
#endif

// This is a public interface for reading files via mapping, can be used anywhere, not only in the loader.

typedef struct abFile_Mapping {
	uint8_t const * data;
	size_t size;

	#if defined(abPlatform_OS_Windows)
	HANDLE i_file, i_mapping;
	#endif
} abFile_Mapping;

abBool abFile_Mapping_Open(abFile_Mapping * mapping, abTextU8 const * path, abBool randomAccess, abBool allowOver4GB);
void abFile_Mapping_Close(abFile_Mapping * mapping);

#endif
