#include "abCore.h"
#include "../abFeedback/abFeedback.h"
#include "../abMemory/abMemory.h"
#include "../abWindow/abWindow.h"

static abCorei_QuitRequested = abFalse;

void abCore_Run() {
	abMemory_Init();

	if (!abWindow_Init(1600u, 900u)) {
		abFeedback_Crash("abCore_Run", "Failed to initialize the game window.");
	}

	while (!abCorei_QuitRequested) {
		abWindow_ProcessEvents();
	}

	abMemory_Shutdown();
}

void abCore_RequestQuit(abBool immediate) {
	abCorei_QuitRequested = abTrue;
}
