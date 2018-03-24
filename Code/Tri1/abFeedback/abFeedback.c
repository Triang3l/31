#include "abFeedback.h"
#include "../abText/abText.h"

#if defined(abPlatform_OS_Windows)
/***************************
 * Windows feedback output
 ***************************/
#include <stdlib.h>
#include <Windows.h>

void abFeedback_DebugMessageForceV(char const * format, va_list arguments) {
	char message[1024u];
	abTextA_FormatV(message, abArrayLength(message), format, arguments);
	OutputDebugStringA(message);
}

#if defined(abPlatform_OS_WindowsDesktop)
void abFeedback_CrashV(bool isAssert, char const * functionName, char const * messageFormat, va_list messageArguments) {
	char message[1024u];
	size_t written = abTextA_Copy(functionName, message, abArrayLength(message));
	written += abTextA_CopyInto(isAssert ? " (assertion): " : ": ", message, abArrayLength(message), written);
	abTextA_FormatV(message + written, abArrayLength(message) - written, messageFormat, messageArguments);
	MessageBoxA(abNull, message, "Tri1 Fatal Error", MB_OK);
	abFeedback_Break();
	_exit(EXIT_FAILURE);
}
#else
#error No abFeedback_CrashV for the target Windows application model.
#endif

#else
#error No abFeedback output functions for the target OS.
#endif

/*********************************
 * Platform-independent functions
 *********************************/

void abFeedback_DebugMessageForce(char const * format, ...) {
	va_list arguments;
	va_start(arguments, format);
	abFeedback_DebugMessageForceV(format, arguments);
	va_end(arguments);
}

void abFeedback_Crash(char const * functionName, char const * messageFormat, ...) {
	va_list messageArguments;
	va_start(messageArguments, messageFormat);
	abFeedback_CrashV(false, functionName, messageFormat, messageArguments);
	va_end(messageArguments);
}

void abFeedback_AssertCrash(char const * functionName, char const * messageFormat, ...) {
	va_list messageArguments;
	va_start(messageArguments, messageFormat);
	abFeedback_CrashV(true, functionName, messageFormat, messageArguments);
	va_end(messageArguments);
}
