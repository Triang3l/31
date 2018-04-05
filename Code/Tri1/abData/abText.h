#ifndef abInclude_abData_abText
#define abInclude_abData_abText
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
 *
 * abText_InvalidSubstitute replaces corrupt code points when reading UTF-8 and UTF-16 strings.
 * In all encodings, invalid code points are those above 0x10ffff, surrogate pair indicators (0xd800 to 0xdfff) and the last 2 in each plane.
 * Zero terminator is considered valid.
 */

typedef char abTextU8; // So ASCII and UTF-8 literals can be written the same way.
typedef uint16_t abTextU16;
typedef uint32_t abTextU32; // Whole code point.

#define abText_InvalidSubstitute '?' // Must be a 7-bit ASCII character!

/********
 * ASCII
 ********/

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
size_t abTextA_Copy(char * target, size_t targetSize, char const * source);
inline size_t abTextA_CopyInto(char * target, size_t targetSize, size_t targetOffset, char const * source) {
	return (targetOffset < targetSize ? abTextA_Copy(target + targetOffset, targetSize - targetOffset, source) : 0u);
}
size_t abTextA_FormatV(char * target, size_t targetSize, char const * format, va_list arguments);
size_t abTextA_Format(char * target, size_t targetSize, char const * format, ...);

/**********************
 * Unicode code points
 **********************/

inline abBool abTextU32_IsCPValid(abTextU32 cp) {
	return (cp <= 0x10ffffu) && ((cp & 0xfffeu) != 0xfffeu) && ((cp & ~((abTextU32) 0x7ffu)) != 0xd800u);
}
abForceInline abTextU32 abTextU32_ValidateCP(abTextU32 cp) { return abTextU32_IsCPValid(cp) ? cp : abText_InvalidSubstitute; }

/********
 * UTF-8
 ********/

inline unsigned int abTextU8_CPLength_Valid(abTextU32 cp) { return (cp > 0u) + (cp > 0x7fu) + (cp > 0x7ffu) + (cp > 0xffffu); }
inline unsigned int abTextU8_CPLength(abTextU32 cp) { return abTextU32_IsCPValid(cp) ? abTextU8_CPLength_Valid(cp) : 1u; }

abTextU32 abTextU8_NextCP(abTextU8 const * * textCursor); // 0 if nothing to read anymore. Advances the cursor.

inline size_t abTextU8_LengthInCPs(abTextU8 const * text) {
	size_t length = 0u;
	while (abTextU8_NextCP(&text) != '\0') { ++length; }
	return length;
}
inline size_t abTextU8_LengthInU16(abTextU8 const * text) {
	size_t length = 0u;
	abTextU32 cp;
	while ((cp = abTextU8_NextCP(&text)) != '\0') { length += 1u + ((cp >> 16u) != 0u); }
	return length;
}

/*********
 * UTF-16
 *********/

abTextU32 abTextU16_NextCP(abTextU16 const * * textCursor); // 0 if nothing to read anymore. Advances the cursor.

inline size_t abTextU16_LengthInUnits(abTextU16 const * text) { // No validation - returns real data size (for appending)!
	abTextU16 const * originalText = text;
	while (*(text++) != '\0') {}
	return (size_t) (text - originalText);
}
inline size_t abTextU16_LengthInCPs(abTextU16 const * text) {
	size_t length = 0u;
	while (abTextU16_NextCP(&text) != '\0') { ++length; }
	return length;
}

unsigned int abTextU16_WriteCP_Valid(abTextU16 * target, size_t targetSize, abTextU32 cp);
size_t abTextU16_Copy(abTextU16 * target, size_t targetSize, abTextU16 const * source); // Does validation - length may change!
size_t abTextU16_FromU8(abTextU16 * target, size_t targetSize, abTextU8 const * source);

#endif
