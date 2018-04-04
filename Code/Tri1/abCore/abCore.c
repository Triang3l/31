#include "abCore.h"
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

	while (!abCorei_QuitRequested) {
		abPlatform_Window_ProcessEvents();
	}

	abGPU_Shutdown();

	abMemory_Shutdown();
}

void abCore_RequestQuit(abBool immediate) {
	abCorei_QuitRequested = abTrue;
}
