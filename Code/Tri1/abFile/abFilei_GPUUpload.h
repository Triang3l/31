#ifndef abInclude_abFilei_GPUUpload
#define abInclude_abFilei_GPUUpload
#include "../abGPU/abGPU.h"

typedef enum abFilei_GPUUpload_Uploader_State {
	abFilei_GPUUpload_Uploader_State_Available,
	abFilei_GPUUpload_Uploader_State_Recording,
	abFilei_GPUUpload_Uploader_State_Submitted
} abFilei_GPUUpload_Uploader_State;

typedef struct abFilei_GPUUpload_Uploader {
	abFilei_GPUUpload_Uploader_State i_state;

	abGPU_Fence i_fence;

	abGPU_CmdList cmdList; // Accessible by loaders for writing commands (when the copier is recording).

	// This is temporary - a persistent (or expanding) buffer should be used instead.
	abGPU_Buffer i_buffer;
	abBool i_bufferUsed;

	abGPU_Image i_smallImage; // Persistent.
	abGPU_Image i_largeImage; // Temporary.
	abBool i_largeImageUsed;
} abFilei_GPUUpload_Uploader;

#define abFilei_GPUUpload_UploaderCount 3u
extern abFilei_GPUUpload_Uploader abFilei_GPUUpload_Uploaders[abFilei_GPUUpload_UploaderCount];

// Main thread interface.
void abFilei_GPUUpload_Init();
void abFilei_GPUUpload_Update();
#define abFilei_GPUUpload_RequestUploader_NoneAvailable UINT_MAX
unsigned int abFilei_GPUUpload_RequestUploader();
void abFilei_GPUUpload_Shutdown();

// Loader thread interface.
abGPU_Buffer * abFilei_GPUUpload_LoaderRequestBuffer(abFilei_GPUUpload_Uploader * uploader, unsigned int size);
abGPU_Image * abFilei_GPUUpload_LoaderRequestImage(abFilei_GPUUpload_Uploader * uploader, abGPU_Image_Options dimensionOptions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format);

#endif
