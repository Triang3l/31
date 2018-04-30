#ifndef abInclude_abPlatform
#define abInclude_abPlatform
// Common defines some platform values as well.
#include "../abData/abText.h"

abBool abPlatform_Window_Init(unsigned int width, unsigned int height);
abBool abPlatform_Window_GetSize(/* optional */ unsigned int * width, /* optional */ unsigned int * height);
abBool abPlatform_Window_InitGPUDisplayChain(struct abGPU_DisplayChain * chain, abTextU8 const * name,
		unsigned int imageCount, enum abGPU_Image_Format format, unsigned int width, unsigned int height);
void abPlatform_Window_ProcessEvents();
void abPlatform_Window_Shutdown();

long long abPlatform_Time_Usec(); // Signed for easier subtracting (a newer value may even be below an older one).

#endif
