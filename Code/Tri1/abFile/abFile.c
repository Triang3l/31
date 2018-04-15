#include "abFile.h"
#include "abFilei_GPUUpload.h"

void abFile_Init() {
	abFilei_GPUUpload_Init();
}

void abFile_Update() {
	abFilei_GPUUpload_Update();
}

void abFile_Shutdown() {
	abFilei_GPUUpload_Shutdown();
}
