#include "abFeedback.h"
#include "../abData/abText.h"

#if defined(abPlatform_OS_Windows)
/***************************
 * Windows feedback output
 ***************************/
#include <stdlib.h>
#include <Windows.h>

void abFeedback_DebugMessageForceV(char const * format, va_list arguments) {
	char message[1024u];
	abTextA_FormatV(message, abArrayLength(message), format, arguments);
	size_t messageLength = abTextA_Length(message);
	messageLength = abMin(abArrayLength(message) - 2u, messageLength);
	message[messageLength] = '\n';
	message[messageLength + 1u] = '\0';
	OutputDebugStringA(message);
}

#if defined(abPlatform_OS_WindowsDesktop)
void abFeedback_CrashV(abBool isAssert, char const * functionName, char const * messageFormat, va_list messageArguments) {
	char message[1024u];
	size_t written = abTextA_Copy(message, abArrayLength(message), functionName);
	written += abTextA_CopyInto(message, abArrayLength(message), written, isAssert ? " (assertion): " : ": ");
	abTextA_FormatV(message + written, abArrayLength(message) - written, messageFormat, messageArguments);
	abFeedback_DebugMessageForce("Fatal error: %s", message);
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
	abFeedback_CrashV(abFalse, functionName, messageFormat, messageArguments);
	va_end(messageArguments);
}

void abFeedback_AssertCrash(char const * functionName, char const * messageFormat, ...) {
	va_list messageArguments;
	va_start(messageArguments, messageFormat);
	abFeedback_CrashV(abTrue, functionName, messageFormat, messageArguments);
	va_end(messageArguments);
}
