#ifndef abInclude_abGFXi
#define abInclude_abGFXi
#include "abGFX.h"

/*************************
 * Texture/buffer handles
 *************************/

void abGFXim_Handles_Init();
void abGFXim_Handles_Shutdown();

/*****************
 * Render targets
 *****************/

typedef enum abGFX_RT_Color {
	abGFX_RT_Color_WindowStart,
		abGFX_RT_Color_WindowEnd = abGFX_RT_Color_WindowStart + abGPU_DisplayChain_MaxImages - 1u,

		abGFX_RT_Color_Count
} abGFX_RT_Color;

typedef enum abGFX_RT_Depth {
	abGFX_RT_Depth_Main,
		abGFX_RT_Depth_Count
} abGFX_RT_Depth;

extern abGPU_RTStore abGFXi_RT_Store;

extern abGPU_DisplayChain abGFXi_RT_WindowDisplayChain;

void abGFXim_RT_Init();
void abGFXim_RT_Shutdown();

#endif
