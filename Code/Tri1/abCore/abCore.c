#include "abCore.h"
#include "../abFile/abFile.h"
#include "../abFeedback/abFeedback.h"
#include "../abGFX/abGFX.h"
#include "../abGPU/abGPU.h"
#include "../abMemory/abMemory.h"
#include "../abPlatform/abPlatform.h"

static abCorei_QuitRequested = abFalse;

void abCore_Run() {
	abMemory_Init();

	if (!abPlatform_Window_Init(1600u, 900u)) {
		abFeedback_Crash("abCore_Run", "Failed to initialize the game window.");
	}

	#ifdef abFeedback_DebugBuild
	abGPU_Init(abTrue);
	#else
	abGPU_Init(abFalse);
	#endif

	abGFXm_InitPreFile();

	abFile_Init();

	while (!abCorei_QuitRequested) {
		abPlatform_Window_ProcessEvents();

		abFile_Update();
	}

	abFile_Shutdown();

	abGFXm_ShutdownPostFile();

	abGPU_Shutdown();

	abMemory_Shutdown();
}

void abCore_RequestQuit(abBool immediate) {
	abCorei_QuitRequested = abTrue;
}
