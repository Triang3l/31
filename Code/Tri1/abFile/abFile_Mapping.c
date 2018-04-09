#include "abFile_Mapping.h"

#if defined(abPlatform_OS_Windows)
abBool abFile_Mapping_Open(abFile_Mapping * mapping, abTextU8 const * path, abBool randomAccess, abBool allowOver4GB) {
	size_t pathU16Size = abTextU8_LengthInU16(path) + 1u;
	abTextU16 * pathU16 = abStackAlloc(pathU16Size * sizeof(abTextU16));
	abTextU16_FromU8(pathU16, pathU16Size, path);

	HANDLE fileHandle = CreateFileW((WCHAR const *) pathU16, GENERIC_READ, FILE_SHARE_READ, abNull, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | (randomAccess ? FILE_FLAG_RANDOM_ACCESS : FILE_FLAG_SEQUENTIAL_SCAN), abNull);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		return abFalse;
	}

	#ifdef abPlatform_CPU_32Bit
	allowOver4GB = abFalse;
	#endif
	FILE_STANDARD_INFO fileInfo;
	if (!GetFileInformationByHandleEx(fileHandle, FileStandardInfo, &fileInfo, sizeof(fileInfo)) ||
			fileInfo.EndOfFile.QuadPart <= 0ll || (!allowOver4GB && fileInfo.EndOfFile.HighPart != 0)) {
		CloseHandle(fileHandle);
		return abFalse;
	}
	size_t size = (size_t) fileInfo.EndOfFile.QuadPart;

	HANDLE mappingHandle = CreateFileMappingW(fileHandle, abNull, PAGE_READONLY, 0u, 0u, abNull);
	if (mappingHandle == abNull) {
		CloseHandle(fileHandle);
		return abFalse;
	}

	void * data = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0u, 0u, size);
	if (data == abNull) {
		CloseHandle(mappingHandle);
		CloseHandle(fileHandle);
		return abFalse;
	}

	mapping->data = (uint8_t const *) data;
	mapping->size = size;
	mapping->i_file = fileHandle;
	mapping->i_mapping = mappingHandle;
	return abTrue;
}

void abFile_Mapping_Close(abFile_Mapping * mapping) {
	UnmapViewOfFile(mapping->data);
	CloseHandle(mapping->i_mapping);
	CloseHandle(mapping->i_file);
}

#else
#error No abFile_Mapping for the target OS.
#endif
