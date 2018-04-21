#ifndef abInclude_abGFX
#define abInclude_abGFX
#include "../abGPU/abGPU.h"

void abGFX_InitPreFile();
void abGFX_ShutdownPostFile();

/*
 * Texture/buffer handles.
 */
extern abGPU_HandleStore abGFX_Handles_Store;
unsigned int abGFX_Handles_Alloc(unsigned int count);
void abGFX_Handles_Free(unsigned int handleIndex);
// Internal.
void abGFXi_Handles_Init();
void abGFXi_Handles_Shutdown();

#endif
