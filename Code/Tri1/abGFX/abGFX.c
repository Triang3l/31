#include "abGFXi.h"

void abGFXm_InitPreFile() {
	abGFXim_Handles_Init();
}

void abGFXm_InitPostFile() {
	abGFXim_RT_Init();
}

void abGFXm_ShutdownPreFile() {
	abGFXim_RT_Shutdown();
}

void abGFXm_ShutdownPostFile() {
	abGFXim_Handles_Shutdown();
}
