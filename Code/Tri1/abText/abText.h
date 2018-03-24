#ifndef abInclude_abText
#define abInclude_abText
#include "../abCommon.h"

/*
 * Zero-terminated string manipulation functions.
 *
 * "Storage unit" refers to a single char, abTextU8, abTextU16.
 * "Character" means a whole code point (including combined UTF-16 surrogate pairs).
 *
 * In the function prototypes, "size" includes the terminator, and "length" doesn't.
 * Both are in storage units, so for fixed-length buffers, abArrayLength should be used to pass the size.
 * Return values are generally lengths, not sizes.
 *
 * Copy functions return the length in storage units actually written.
 *
 * Format functions return the length (in storage units) that would be written if target was large enough.
 * If the format is invalid, an empty string is created, and 0 is returned (unlike snprintf, which returns a negative value).
 * With null target and 0 target size, required buffer size minus 1 is returned.
 */

typedef char abTextU8;
typedef uint16_t abTextU16;

/****************
 * ASCII strings
 ****************/

#define abTextA_Length strlen
#define abTextA_Compare strcmp
#define abTextA_ComparePart strncmp
#ifdef abPlatform_OS_Windows
#define abTextA_CompareCaseless _stricmp
#define abTextA_ComparePartCaseless _strnicmp
#else
#define abTextA_CompareCaseless strcasecmp
#define abTextA_ComparePartCaseless strncasecmp
#endif
size_t abTextA_Copy(char const * source, char * target, size_t targetSize);
inline size_t abTextA_CopyInto(char const * source, char * target, size_t targetSize, size_t targetOffset) {
	return (targetOffset < targetSize ? abTextA_Copy(source, target + targetOffset, targetSize - targetOffset) : 0);
}
size_t abTextA_FormatV(char * target, size_t targetSize, char const * format, va_list arguments);
size_t abTextA_Format(char * target, size_t targetSize, char const * format, ...);

#endif
