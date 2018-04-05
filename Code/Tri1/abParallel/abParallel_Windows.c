#include "abParallel.h"
#ifdef abPlatform_OS_Windows
#include "../abData/abText.h"
#include <process.h>
#include <stdlib.h>

typedef struct abParalleli_Windows_Thread_Parameters {
	char name[abParallel_Thread_MaxNameLength];
	abParallel_Thread_Entry entry;
	void * data;
} abParalleli_Windows_Thread_Parameters;

#pragma pack(push, 8)
typedef struct abParalleli_Windows_Thread_NameInfo {
	DWORD type;
	char const * name;
	DWORD threadID;
	DWORD flags;
} abParalleli_Windows_Thread_NameInfo;
#pragma pack(pop)

static void abParalleli_Windows_Thread_Entry(void * parametersPointer) {
	abParalleli_Windows_Thread_Parameters parameters = *((abParalleli_Windows_Thread_Parameters *) parametersPointer);
	free(parametersPointer);
	if (parameters.name[0u] != '\0') {
		abParalleli_Windows_Thread_NameInfo nameInfo = { .type = 0x1000u, .name = parameters.name, .threadID = (DWORD) -1, .flags = 0u };
		__try {
			RaiseException(0x406d1388u, 0u, sizeof(nameInfo) / sizeof(ULONG_PTR), (ULONG_PTR *) &nameInfo);
		} __except (EXCEPTION_EXECUTE_HANDLER) {}
	}
	parameters.entry(parameters.data);
}

abBool abParallel_Thread_Start(abParallel_Thread * thread, char const * name, abParallel_Thread_Entry entry, void * data) {
	abParalleli_Windows_Thread_Parameters * parameters = malloc(sizeof(abParalleli_Windows_Thread_Parameters));
	if (parameters == abNull) {
		return abFalse;
	}
	if (name != abNull) {
		abTextA_Copy(parameters->name, abArrayLength(parameters->name), name);
	} else {
		parameters->name[0u] = '\0';
	}
	parameters->entry = entry;
	parameters->data = data;
	uintptr_t handle = _beginthread(abParalleli_Windows_Thread_Entry, 0u, parameters);
	if (handle == -1l) {
		free(parameters);
		return abFalse;
	}
	*thread = (HANDLE) handle;
	return abTrue;
}

#endif
