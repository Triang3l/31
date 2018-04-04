#include "abPlatform.h"
#ifdef abPlatform_OS_Windows
#include "../abFeedback/abFeedback.h"
#include <Windows.h>

static LARGE_INTEGER abPlatformi_Time_Windows_Frequency, abPlatformi_Time_Windows_Origin;

void abPlatformi_OS_Windows_Init() {
	if (!QueryPerformanceFrequency(&abPlatformi_Time_Windows_Frequency)) {
		abFeedback_Crash("abPlatformi_OS_Windows_Init", "Failed to obtain the high-resolution timer frequency.");
	}
	QueryPerformanceCounter(&abPlatformi_Time_Windows_Origin);
}

long long abPlatform_Time_Usec() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return ((counter.QuadPart - abPlatformi_Time_Windows_Origin.QuadPart) * 1000000) / abPlatformi_Time_Windows_Frequency.QuadPart;
}

#endif
