#ifndef abInclude_abGFX
#define abInclude_abGFX
#include "../abGPU/abGPU.h"

void abGFX_InitPreFile();
void abGFX_ShutdownPostFile();

/*
 * Texture/buffer handles.
 */
extern abGPU_HandleStore abGFX_Handles_Store;
#define abGFX_Handles_Alloc_Failed UINT_MAX
unsigned int abGFX_Handles_Alloc(unsigned int count);
// Internal.
void abGFXi_Handles_Init();
void abGFXi_Handles_Shutdown();

#endif
