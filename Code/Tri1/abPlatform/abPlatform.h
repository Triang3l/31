#ifndef abInclude_abPlatform
#define abInclude_abPlatform
#include "../abCommon.h" // Defines some platform values as well.

abBool abPlatform_Window_Init(unsigned int width, unsigned int height);
void abPlatform_Window_ProcessEvents();

long long abPlatform_Time_Usec(); // Signed for easier subtracting (a newer value may even be below an older one).

#endif
