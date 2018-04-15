#include "abFilei_GPUUpload.h"
#include "../abData/abText.h"
#include "../abFeedback/abFeedback.h"
#include "../abParallel/abParallel.h"

abFilei_GPUUpload_Uploader abFilei_GPUUpload_Uploaders[abFilei_GPUUpload_UploaderCount];

static unsigned int abFilei_GPUUpload_SmallImageSize;

void abFilei_GPUUpload_Init() {
	// Make the persistent upload image large enough to fit a 2048x2048 S3TC image or a 1024x1024 RGBA8 image.
	unsigned int smallImageSizeS3TC = abGPU_Image_CalculateMemoryUsage(abGPU_Image_Options_Upload,
			2048u, 2048u, 1u, 12u, abGPU_Image_Format_S3TC_A8);
	unsigned int smallImageSizeRGBA = abGPU_Image_CalculateMemoryUsage(abGPU_Image_Options_Upload,
			1024u, 1024u, 1u, 11u, abGPU_Image_Format_R8G8B8A8);
	unsigned int smallImageMips;
	abGPU_Image_Format smallImageFormat;
	if (smallImageSizeS3TC >= smallImageSizeRGBA) {
		smallImageMips = 12u;
		smallImageFormat = abGPU_Image_Format_S3TC_A8;
		abFilei_GPUUpload_SmallImageSize = smallImageSizeS3TC;
	} else {
		smallImageMips = 11u;
		smallImageFormat = abGPU_Image_Format_R8G8B8A8;
		abFilei_GPUUpload_SmallImageSize = smallImageSizeRGBA;
	}
	unsigned int smallImageWH = 1u << (smallImageMips - 1u);

	char name[64u];
	size_t namePrefixLength = abTextA_Copy(name, abArrayLength(name), "abFilei_GPUUpload_Uploaders[0].");
	char * nameSuffix = name + namePrefixLength;
	size_t nameSuffixSize = abArrayLength(name) - namePrefixLength;
	for (unsigned int uploaderIndex = 0u; uploaderIndex < abFilei_GPUUpload_UploaderCount; ++uploaderIndex) {
		abFilei_GPUUpload_Uploader * uploader = &abFilei_GPUUpload_Uploaders[uploaderIndex];
		name[namePrefixLength - 3u] = '0' + uploaderIndex;

		abTextA_Copy(nameSuffix, nameSuffixSize, "i_fence");
		if (!abGPU_Fence_Init(&uploader->i_fence, name, abGPU_CmdQueue_Copy)) {
			abFeedback_Crash("abFilei_GPUUpload_Init", "Failed to create the GPU copy fence.");
		}

		abTextA_Copy(nameSuffix, nameSuffixSize, "cmdList");
		if (!abGPU_CmdList_Init(&uploader->cmdList, name, abGPU_CmdQueue_Copy)) {
			abFeedback_Crash("abFilei_GPUUpload_Init", "Failed to create the GPU copy command list.");
		}

		uploader->i_bufferUsed = abFalse;

		abTextA_Copy(nameSuffix, nameSuffixSize, "i_smallImage");
		if (!abGPU_Image_Init(&uploader->i_smallImage, name, abGPU_Image_Options_Upload,
				smallImageWH, smallImageWH, 1u, smallImageMips, smallImageFormat, abGPU_Image_Usage_Upload, abNull)) {
			abFeedback_Crash("abFilei_GPUUpload_Init", "Failed to create the small GPU copy upload image.");
		}
		uploader->i_largeImageUsed = abFalse;
	}
}

static void abFilei_GPUUpload_MakeAvailable(abFilei_GPUUpload_Uploader * uploader) {
	// Assuming the main thread already can see up-to-date state of the upload buffers.
	if (uploader->i_bufferUsed) {
		uploader->i_bufferUsed = abFalse;
		abGPU_Buffer_Destroy(&uploader->i_buffer);
	}
	if (uploader->i_largeImageUsed) {
		uploader->i_largeImageUsed = abFalse;
		abGPU_Image_Destroy(&uploader->i_largeImage);
	}
	uploader->i_state = abFilei_GPUUpload_Uploader_State_Available;
}

void abFilei_GPUUpload_Update() {
	for (unsigned int uploaderIndex = 0u; uploaderIndex < abFilei_GPUUpload_UploaderCount; ++uploaderIndex) {
		abFilei_GPUUpload_Uploader * uploader = &abFilei_GPUUpload_Uploaders[uploaderIndex];
		if (uploader->i_state == abFilei_GPUUpload_Uploader_State_Submitted) {
			if (abGPU_Fence_IsCrossed(&uploader->i_fence)) {
				abFilei_GPUUpload_MakeAvailable(uploader);
			}
		}
	}
}

unsigned int abFilei_GPUUpload_RequestUploader() {
	for (unsigned int uploaderIndex = 0u; uploaderIndex < abFilei_GPUUpload_UploaderCount; ++uploaderIndex) {
		abFilei_GPUUpload_Uploader * uploader = &abFilei_GPUUpload_Uploaders[uploaderIndex];
		if (uploader->i_state != abFilei_GPUUpload_Uploader_State_Available) {
			continue;
		}
		uploader->i_state = abFilei_GPUUpload_Uploader_State_Recording;
		abGPU_CmdList_Record(&uploader->cmdList);
		abGPU_Cmd_CopyingBegin(&uploader->cmdList);
	}
	return abFilei_GPUUpload_RequestUploader_NoneAvailable;
}

void abFilei_GPUUpload_Shutdown() {
	for (int uploaderIndex = (int) abFilei_GPUUpload_UploaderCount - 1; uploaderIndex >= 0; --uploaderIndex) {
		abFilei_GPUUpload_Uploader * uploader = &abFilei_GPUUpload_Uploaders[uploaderIndex];
		abFeedback_Assert(uploader->i_state != abFilei_GPUUpload_Uploader_State_Recording,
				"abFilei_GPUUpload_Shutdown", "An uploader must exit the recording state before shutting down.");
		if (uploader->i_state == abFilei_GPUUpload_Uploader_State_Submitted) {
			abGPU_Fence_Await(&uploader->i_fence);
			abFilei_GPUUpload_MakeAvailable(uploader);
		}
		abGPU_Image_Destroy(&uploader->i_smallImage);
		abGPU_CmdList_Destroy(&uploader->cmdList);
		abGPU_Fence_Destroy(&uploader->i_fence);
	}
}

/*
 * Loader thread interface.
 */

abGPU_Buffer * abFilei_GPUUpload_LoaderRequestBuffer(abFilei_GPUUpload_Uploader * uploader, unsigned int size) {
	abFeedback_Assert(!uploader->i_bufferUsed, "abFilei_GPUUpload_LoaderRequestBuffer", "Buffer requested twice.");
	if (uploader->i_bufferUsed) {
		abGPU_Buffer_Destroy(&uploader->i_buffer);
	}
	char name[] = "abFilei_GPUUpload_Uploaders[0].i_buffer";
	name[abArrayLength("abFilei_GPUUpload_Uploaders[") - 1u] = '0' + (unsigned int) (uploader - abFilei_GPUUpload_Uploaders);
	if (!abGPU_Buffer_Init(&uploader->i_buffer, name, abGPU_Buffer_Access_Upload, size, abFalse, abGPU_Buffer_Usage_CPUWrite)) {
		abFeedback_Crash("abFilei_GPUUpload_LoaderRequestBuffer", "Failed to create a GPU upload buffer with size %u bytes.", size);
	}
	uploader->i_bufferUsed = abTrue;
	return &uploader->i_buffer;
}

abGPU_Image * abFilei_GPUUpload_LoaderRequestImage(abFilei_GPUUpload_Uploader * uploader, abGPU_Image_Options dimensionOptions,
		unsigned int w, unsigned int h, unsigned int d, unsigned int mips, abGPU_Image_Format format) {
	if (abGPU_Image_RespecifyUploadBuffer(&uploader->i_smallImage, dimensionOptions, w, h, d, mips, format)) {
		return &uploader->i_smallImage;
	}
	abFeedback_Assert(!uploader->i_largeImageUsed, "abFilei_GPUUpload_LoaderRequestImage", "Large image requested twice.");
	if (uploader->i_largeImageUsed) {
		abGPU_Image_Destroy(&uploader->i_largeImage);
	}
	char name[] = "abFilei_GPUUpload_Uploaders[0].i_largeImage";
	name[abArrayLength("abFilei_GPUUpload_Uploaders[") - 1u] = '0' + (unsigned int) (uploader - abFilei_GPUUpload_Uploaders);
	if (!abGPU_Image_Init(&uploader->i_largeImage, name, abGPU_Image_Options_Upload | dimensionOptions, w, h, d, mips, format,
			abGPU_Image_Usage_Upload, abNull)) {
		abFeedback_Crash("abFilei_GPUUpload_LoaderRequestImage", "Failed to create a %ux%ux%u GPU upload image.", w, h, d);
	}
	uploader->i_largeImageUsed = abTrue;
	return &uploader->i_largeImage;
}
