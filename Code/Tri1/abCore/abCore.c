#include "abCore.h"
#include "../abFile/abFile.h"
#include "../abFeedback/abFeedback.h"
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

	abFile_Init();

	while (!abCorei_QuitRequested) {
		abPlatform_Window_ProcessEvents();

		abFile_Update();
	}

	abFile_Shutdown();

	abGPU_Shutdown();

	abMemory_Shutdown();
}

void abCore_RequestQuit(abBool immediate) {
	abCorei_QuitRequested = abTrue;
}
