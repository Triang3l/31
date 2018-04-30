#include "abGFXi.h"
#include "../abFeedback/abFeedback.h"
#include "../abPlatform/abPlatform.h"

abGPU_RTStore abGFXi_RT_Store;

abGPU_DisplayChain abGFXi_RT_WindowDisplayChain;

void abGFXim_RT_Init() {
	if (!abGPU_RTStore_Init(&abGFXi_RT_Store, "abGFXi_RT_Store", abGFX_RT_Color_Count, abGFX_RT_Depth_Count)) {
		abFeedback_Crash("abGFXim_RT_Init", "Failed to create the render target store.");
	}

	unsigned int windowWidth, windowHeight;
	if (!abPlatform_Window_GetSize(&windowWidth, &windowHeight)) {
		abFeedback_Crash("abGFXim_RT_Init", "Failed to get window size for the display chain.");
	}
	if (!abPlatform_Window_InitGPUDisplayChain(&abGFXi_RT_WindowDisplayChain, "abGFXi_RT_WindowDisplayChain",
			3u, abGPU_Image_Format_R8G8B8A8_sRGB, windowWidth, windowHeight)) {
		abFeedback_Crash("abGFXim_RT_Init", "Failed to create the window display chain.");
	}
	for (unsigned int windowImageIndex = 0u; windowImageIndex < abGFXi_RT_WindowDisplayChain.imageCount; ++windowImageIndex) {
		abGPU_RTStore_SetColor(&abGFXi_RT_Store, abGFX_RT_Color_WindowEnd + windowImageIndex,
				&abGFXi_RT_WindowDisplayChain.images[windowImageIndex], abGPU_Image_Slice_Make(0u, 0u, 0u));
	}
}

void abGFXim_RT_Shutdown() {
	abGPU_DisplayChain_Destroy(&abGFXi_RT_WindowDisplayChain);
	abGPU_RTStore_Destroy(&abGFXi_RT_Store);
}
