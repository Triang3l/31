#include "abText.h"
#include <stdio.h>

size_t abTextA_Copy(char const * source, char * target, size_t targetSize) {
	char * originalTarget = target;
	if (targetSize != 0u) {
		while (--targetSize != 0u && *source != '\0') {
			*(target++) = *(source++);
		}
		*target = '\0';
	}
	return (size_t) (target - originalTarget);
}

size_t abTextA_FormatV(char * target, size_t targetSize, char const * format, va_list arguments) {
	if (target == abNull || targetSize == 0u) { // Normalize this.
		target = abNull;
		targetSize = 0u;
	}
	int length = vsnprintf(target, targetSize, format, arguments);
	if (length < 0) {
		if (targetSize != 0u) {
			target[0u] = '\0';
		}
		return 0u;
	}
	if (targetSize != 0u) {
		target[targetSize - 1u] = '\0';
	}
	return (size_t) length;
}

size_t abTextA_Format(char * target, size_t targetSize, char const * format, ...) {
	va_list arguments;
	va_start(arguments, format);
	size_t length = abTextA_FormatV(target, targetSize, format, arguments);
	va_end(arguments);
	return length;
}
