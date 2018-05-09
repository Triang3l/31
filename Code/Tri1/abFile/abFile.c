#include "abFilei.h"
#include "abFilei_GPUUpload.h"

abMemory_Tag * abFilei_MemoryTag;

void abFile_Init() {
	abFilei_MemoryTag = abMemory_Tag_Create("File");

	abFilei_Asset_InitSystem();

	abFilei_GPUUpload_Init();
}

void abFile_Update() {
	abFilei_GPUUpload_Update();
}

void abFile_Shutdown() {
	abFilei_GPUUpload_Shutdown();

	abFilei_Asset_ShutdownSystem();

	abMemory_Tag_Destroy(abFilei_MemoryTag);
}
