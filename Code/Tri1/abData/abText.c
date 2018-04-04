#include "abText.h"
#include <stdio.h>

/********
 * ASCII
 ********/

size_t abTextA_Copy(char * target, size_t targetSize, char const * source) {
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

/********
 * UTF-8
 ********/

abTextU32 abTextU8_NextCP(abTextU8 const * * textCursor) {
	abTextU8 const * text = *textCursor, first = *text;
	if (first == '\0') {
		return '\0';
	}
	if (first <= 127u) {
		++(*textCursor);
		return first;
	}
	// Doing && sequences in order is safe due to early exit.
	if ((first >> 5u) == 6u) {
		if ((text[1u] >> 6u) == 2u) {
			*textCursor += 2u;
			return ((abTextU32) (first & 31u) << 6u) | (text[1u] & 63u);
		}
	}
	if ((first >> 4u) == 14u) {
		if ((text[1u] >> 6u) == 2u && (text[2u] >> 6u) == 2u) {
			*textCursor += 3u;
			abTextU32 cp = ((abTextU32) (first & 15u) << 12u) | ((abTextU32) (text[1u] & 63u) << 6u) | (text[2u] & 63u);
			return ((cp >> 1u) != 0x7fffu && (cp & ~((abTextU32) 0x7ffu)) != 0xd800u) ? cp : abText_InvalidSubstitute;
		}
	}
	if ((first >> 3u) == 30u) {
		if ((text[1u] >> 6u) == 2u && (text[2u] >> 6u) == 2u && (text[3u] >> 6u) == 2u) {
			*textCursor += 4u;
			abTextU32 cp = ((abTextU32) (first & 7u) << 18u) | ((abTextU32) (text[1u] & 63u) << 12u) |
					((abTextU32) (text[2u] & 63u) << 6u) | (text[3u] & 63u);
			return (cp <= 0x10ffffu && (cp & 0xfffeu) != 0xfffeu) ? cp : abText_InvalidSubstitute;
		}
	}
	++(*textCursor);
	return abText_InvalidSubstitute;
}

/*********
 * UTF-16
 *********/

size_t abTextU16_FromU8(abTextU16 * target, size_t targetSize, abTextU8 const * source) {
	abTextU16 * originalTarget = target;
	if (targetSize != 0u) {
		--targetSize;
		abTextU32 cp;
		while (targetSize != 0u && (cp = abTextU8_NextCP(&source)) != '\0') {
			if ((cp >> 16u) != 0u) {
				if (targetSize < 2u) {
					break;
				}
				*(target++) = 0xd800u | ((cp >> 10u) - 0x40u);
				*(target++) = 0xdc00u | (cp & 0x3ffu);
				targetSize -= 2u;
			} else {
				*(target++) = cp;
				--targetSize;
			}
		}
		*target = '\0';
	}
	return (size_t) (target - originalTarget);
}
