#ifndef abInclude_abGFX
#define abInclude_abGFX
#include "../abGPU/abGPU.h"

// abGFXm and abGFXim functions are to be called from the main thread.

void abGFXm_InitPreFile();
void abGFXm_InitPostFile();
void abGFXm_ShutdownPreFile();
void abGFXm_ShutdownPostFile();

/*
 * Texture/buffer handles.
 */
extern abGPU_HandleStore abGFX_Handles_Store;
unsigned int abGFXm_Handles_Alloc(unsigned int count);
void abGFXm_Handles_Free(unsigned int handleIndex);

#endif
