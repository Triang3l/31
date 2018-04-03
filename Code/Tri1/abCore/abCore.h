#ifndef abInclude_abCore
#define abInclude_abCore
#include "../abCommon.h"

void abCore_Run(); // Entry point.
void abCore_RequestQuit(abBool immediate); // May be called from anywhere on the core thread. If not immediate, menu may be shown.

#endif
