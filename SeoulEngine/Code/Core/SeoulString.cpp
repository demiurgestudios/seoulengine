/**
 * \file SeoulString.cpp
 * \brief Implementation of a string class in SeoulEngine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CaseMappingData.h"
#include "HashFunctions.h"
#include "MemoryManager.h"
#include "SeoulHString.h"
#include "SeoulMath.h"
#include "SeoulString.h"
#include "StringUtil.h"
#include "Logger.h"
#include "Vector.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

namespace Seoul
{

/**
 * Constant used to indicate an index which does not exist in a String.  It is
 * returned by the various Find*() methods to indicate that a value was not
 * found in the String.
 */
const UInt String::NPos = UIntMax;

/**
 * Default constructor
 *
 * Default constructor.  Initializes the String to the empty String.
 */
String::String()
	: m_zSize(0),
	  m_zCapacity(kSmallBufferSize)
{
	m_buffer.aSmallBuffer[0] = 0;
}

/**
 * HString string constructor
 */
String::String(HString hstring)
	: m_zSize(0),
	  m_zCapacity(kSmallBufferSize)
{
	m_buffer.aSmallBuffer[0] = 0;

	const Byte* sStr = hstring.CStr();
	if (sStr)
	{
		Assign(sStr, hstring.GetSizeInBytes());
	}
}

/**
 * C string constructor
 *
 * C string constructor.  Initializes the String to a copy of the given
 * nullptr-terminated C string.
 *
 * @param[in] sStr C string to make a copy of
 */
String::String(const Byte *sStr)
	: m_zSize(0),
	  m_zCapacity(kSmallBufferSize)
{
	m_buffer.aSmallBuffer[0] = 0;
	if (sStr)
	{
		Assign(sStr, (UInt)strlen(sStr));
	}
}

/**
 * C substring constructor
 *
 * C substring constructor.  Initializes the String to a copy of the C substring
 * beginning at sStr and consisting of at most zSize bytes, not including
 * the nullptr terminator.
 *
 * @param[in] sStr  C string to make a copy of
 * @param[in] zSize Maximum number of bytes to copy
 */
String::String(const Byte *sStr, UInt zSize)
	: m_zSize(0),
	  m_zCapacity(kSmallBufferSize)
{
	m_buffer.aSmallBuffer[0] = 0;
	if (sStr)
	{
		Assign(sStr, zSize);
	}
}

/**
 * Unicode character constructor
 *
 * Unicode character constructor.  Initializes the String to the string
 * consisting of nCount copies of the character chChar.
 *
 * @param[in] chChar Character to copy
 * @param[in] nCount Number of copies of chChar to copy
 */
String::String(UniChar chChar, UInt nCount /* = 1 */)
	: m_zSize(0),
	  m_zCapacity(kSmallBufferSize)
{
	m_buffer.aSmallBuffer[0] = 0;
	Assign(chChar, nCount);
}

/**
 * Copy constructor
 *
 * Copy constructor.  Deep copies the string.
 *
 * @param[in] sStr String to copy
 */
String::String(const String & sStr)
	: m_zSize(0),
	  m_zCapacity(kSmallBufferSize)
{
	m_buffer.aSmallBuffer[0] = 0;
	Assign(sStr);
}

/**
 * Move constructor.
 *
 * Move constructor. Avoids string copy on assignment.
 *
 * @param[in] rsStr String to move.
 */
String::String(String&& rsStr)
	: m_buffer(rsStr.m_buffer)
	, m_zSize(rsStr.m_zSize)
	, m_zCapacity(rsStr.m_zCapacity)
{
	// This implementation is only valid as
	// long as the internal buffer is the same size as the pointer.
	SEOUL_STATIC_ASSERT(sizeof(m_buffer) == sizeof(Byte*));

	rsStr.m_zSize = 0;
	rsStr.m_zCapacity = kSmallBufferSize;
	rsStr.m_buffer.aSmallBuffer[0] = 0;
}

/**
 * Destructor
 */
String::~String()
{
	if (!IsUsingInternalBuffer())
	{
		MemoryManager::Deallocate(m_buffer.pBuffer);
	}
}

/**
 * Assigns this String to a C string
 *
 * Assigns this String to a copy of the given nullptr-terminated C string.
 *
 * @param[in] sStr C string to make a copy of
 *
 * @return A reference to this String
 */
String & String::Assign(const Byte *sStr)
{
	if (sStr == nullptr)
	{
		sStr = "";
	}

	SEOUL_ASSERT_SLOW(IsValidUTF8String(sStr));

	UInt zSize = (UInt)strlen(sStr);

	Reserve(zSize + 1);

	memcpy(GetBuffer(), sStr, zSize + 1);

	m_zSize = zSize;

	return *this;
}

/**
 * Assigns this String to a substring of a C string
 *
 * Assigns this String to a copy of the C string beginning at sStr and
 * consisting of at most zSize bytes, not including the nullptr terminator.
 *
 * @param[in] sStr  C string to make a copy of
 * @param[in] zSize Maximum number of bytes to copy
 *
 * @return A reference to this String
 */
String & String::Assign(const Byte *sStr, UInt zInputSize)
{
	if (sStr == nullptr)
	{
		sStr = "";
	}

	SEOUL_ASSERT(zInputSize < UINT_MAX);

	// In case sStr is shorter than zInputSize, compute initially.
	// Don't use strlen() here, as this variation of Assign()
	// is safe for non-NULL terminated strings.
	UInt zActualSize = 0;
	while (zActualSize < zInputSize)
	{
		if ('\0' == sStr[zActualSize])
		{
			break;
		}

		++zActualSize;
	}

	SEOUL_ASSERT_SLOW(IsValidUTF8String(sStr, zActualSize));

	Reserve(zActualSize + 1);

	Byte *pBuffer = GetBuffer();
	memcpy(pBuffer, sStr, zActualSize);
	pBuffer[zActualSize] = '\0';

	m_zSize = zActualSize;

	return *this;
}

/**
 * Assigns this String to copies of a Unicode character
 *
 * Assigns this String to the string consisting of nCount copies of the
 * character chChar.
 *
 * @param[in] chChar Character to copy
 * @param[in] nCount Number of copies of chChar to copy
 *
 * @return A reference to this String
 */
String & String::Assign(UniChar chChar, UInt nCount /* = 1 */)
{
	SEOUL_ASSERT_SLOW(IsValidUnicodeChar(chChar));

	Byte sChar[4];
	UInt nBytesPerChar = UTF8EncodeChar(chChar, sChar);

	SEOUL_ASSERT(nCount < UINT_MAX / nBytesPerChar);
	UInt zSize = nBytesPerChar * nCount;

	Reserve(zSize + 1);

	m_zSize = zSize;

	Byte *pBuffer = GetBuffer();
	for (UInt i = 0; i < zSize; i += nBytesPerChar)
	{
		memcpy(pBuffer + i, sChar, nBytesPerChar);
	}

	pBuffer[zSize] = '\0';

	return *this;
}

/**
 * Assigns this String to a copy of another String.
 *
 * @param[in] sStr String to copy
 *
 * @return A reference to this String
 */
String & String::Assign(const String & sStr)
{
	if (&sStr == this)
	{
		return *this;
	}
	else
	{
		return Assign(sStr.CStr(), sStr.GetSize());
	}
}

/**
 * Assigns this String to a C string
 *
 * Assigns this String to a copy of the given nullptr-terminated C string.
 *
 * @param[in] sStr C string to make a copy of
 *
 * @return A reference to this String
 */
String & String::operator=(const Byte* sStr)
{
	return Assign(sStr);
}

/**
 * Assigns this String to a copy of another String.
 *
 * @param[in] sStr String to copy
 *
 * @return A reference to this String
 */
String & String::operator=(const String& sStr)
{
	if (this == &sStr)
		return *this;
	return Assign(sStr);
}

/**
* Assigns this String as a move op from rsStr - rsStr is invalidated.
*
* @param[in] rsStr String to move
*
* @return A reference to this String
*/
String & String::operator=(String&& rsStr)
{
	// This implementation is only valid as
	// long as the internal buffer is the same size as the pointer.
	SEOUL_STATIC_ASSERT(sizeof(m_buffer) == sizeof(Byte*));

	if (this == &rsStr)
	{
		return *this;
	}

	// Deallocate current storage if needed.
	if (!IsUsingInternalBuffer())
	{
		MemoryManager::Deallocate(m_buffer.pBuffer);
	}

	m_buffer = rsStr.m_buffer;
	m_zSize = rsStr.m_zSize;
	m_zCapacity = rsStr.m_zCapacity;

	rsStr.m_zSize = 0;
	rsStr.m_zCapacity = kSmallBufferSize;
	rsStr.m_buffer.aSmallBuffer[0] = 0;

	return *this;
}

/**
 * Appends a C string to this String
 *
 * Appends the given nullptr-terminated C string to this String.
 *
 * @param[in] sStr C string to append
 *
 * @return A reference to this String
 */
String & String::Append(const Byte *sStr)
{
	return Append(sStr, (UInt)strlen(sStr));
}

/**
 * Appends a substring of a C string to this String
 *
 * Appends the string beginning at sStr and consisting of at most zSize
 * bytes to this String.
 *
 * @param[in] sStr  C string to append
 * @param[in] zSize Maximum number of bytes to append, not including the nullptr
 *            terminator
 *
 * @return A reference to this String
 */
String & String::Append(const Byte *sStr, UInt zSize)
{
	SEOUL_ASSERT_SLOW(IsValidUTF8String(sStr, zSize));

	if (zSize == 0)
	{
		return *this;
	}

	// Watch out for overflow (although if this happens, I guarantee you there's
	// a bug somewhere...)
	if (m_zSize + zSize + 1 < m_zSize)
	{
		zSize = UINT_MAX - m_zSize - 1;
	}

	if (m_zSize + zSize >= m_zCapacity)
	{
		Reserve(Max(2 * m_zCapacity, m_zSize + zSize + 1));
	}

	Byte *pBuffer = GetBuffer();
	memcpy(pBuffer + m_zSize, sStr, zSize);
	pBuffer[m_zSize + zSize] = '\0';

	m_zSize += zSize;

	return *this;
}

/**
 * Appends copies of a Unicode character to this String
 *
 * Appends nCount copies of the character chChar to this String.
 *
 * @param[in] chChar Character to append
 * @param[in] nCount Number of copies of chChar to copy
 *
 * @return A reference to this String
 */
String & String::Append(UniChar chChar, UInt nCount /* = 1 */)
{
	SEOUL_ASSERT_SLOW(IsValidUnicodeChar(chChar));

	if (nCount == 0)
	{
		return *this;
	}

	Byte sChar[4];
	UInt nBytesPerChar = UTF8EncodeChar(chChar, sChar);

	SEOUL_ASSERT(nCount < UINT_MAX / nBytesPerChar);
	UInt zSize = nBytesPerChar * nCount;

	if (m_zSize + zSize >= m_zCapacity)
	{
		Reserve(Max(2 * m_zCapacity, m_zSize + zSize + 1));
	}

	Byte *pBuffer = GetBuffer();
	for (UInt i = 0; i < zSize; i += nBytesPerChar)
	{
		memcpy(pBuffer + m_zSize + i, sChar, nBytesPerChar);
	}

	m_zSize += zSize;

	pBuffer[m_zSize] = '\0';

	return *this;
}

/**
 * Appends a String to this String
 *
 * Appends the given String to this String.
 *
 * @param[in] sStr String to append
 *
 * @return A reference to this String
 */
String & String::Append(const String & sStr)
{
	if (sStr.m_zSize == 0)
	{
		return *this;
	}

	if (m_zSize + sStr.m_zSize >= m_zCapacity)
	{
		Reserve(Max(2 * m_zCapacity, m_zSize + sStr.m_zSize + 1));
	}

	memcpy(GetBuffer() + m_zSize, sStr.GetBuffer(), sStr.m_zSize + 1);

	m_zSize += sStr.m_zSize;

	return *this;
}

/**
 * Appends a C string to this String
 *
 * Appends the given nullptr-terminated C string to this String.
 *
 * @param[in] sStr C string to append
 *
 * @return A reference to this String
 */
String & String::operator += (const Byte *sStr)
{
	return Append(sStr);
}

/**
 * Appends a Unicode character to this String
 *
 * Appends the character chChar to this String.
 *
 * @param[in] chChar Character to append
 *
 * @return A reference to this String
 */
String & String::operator += (UniChar chChar)
{
	return Append(chChar, 1);
}

/**
 * Appends a String to this String
 *
 * Appends the given String to this String.
 *
 * @param[in] sStr String to append
 *
 * @return A reference to this String
 */
String & String::operator += (const String & sStr)
{
	return Append(sStr);
}

/**
 * String concatenation operator
 *
 * Returns the concatenation of the two given Strings.  Neither String is
 * modified.
 *
 * @param[in] sStr1 First String to concatenate
 * @param[in] sStr2 Second String to concatenate
 *
 * @return The concatenation of sStr1 and sStr2
 */
String operator + (const String & sStr1, const String & sStr2)
{
	return String(sStr1).Append(sStr2);
}

/**
 * String concatenation operator
 *
 * Returns the concatenation of the two given Strings.  Neither String is
 * modified.
 *
 * @param[in] sStr1 First String to concatenate
 * @param[in] sStr2 Second String to concatenate
 *
 * @return The concatenation of sStr1 and sStr2
 */
String operator + (const String & sStr1, const Byte *sStr2)
{
	return String(sStr1).Append(sStr2);
}

/**
 * String concatenation operator
 *
 * Returns the concatenation of the two given Strings.  Neither String is
 * modified.
 *
 * @param[in] sStr1 First String to concatenate
 * @param[in] sStr2 Second String to concatenate
 *
 * @return The concatenation of sStr1 and sStr2
 */
String operator + (const Byte *sStr1, const String & sStr2)
{
	return String(sStr1).Append(sStr2);
}

/**
 * Compares two Strings lexicographically
 *
 * Compares two Strings lexicographically.  If this String is lexicographically
 * less than sStr, a negative integer is returned.  If this String is
 * lexicographically greater than sStr, a positive integer is returned.  If
 * this String equals sStr, zero is returned.
 *
 * @param[in] sStr String to compare to this String
 *
 * @return A negative integer if this String is lexicographically less than
 *         sStr, a positive integer if this String is lexicographically
 *         greater than sStr, or zero if this String equals sStr.
 */
Int String::Compare(const String & sStr) const
{
	return strcmp(GetBuffer(), sStr.GetBuffer());
}

/**
 * Compares two Strings lexicographically
 *
 * Compares two Strings lexicographically.  If this String is lexicographically
 * less than sStr, a negative integer is returned.  If this String is
 * lexicographically greater than sStr, a positive integer is returned.  If
 * this String equals sStr, zero is returned.
 *
 * @param[in] sStr String to compare to this String
 *
 * @return A negative integer if this String is lexicographically less than
 *         sStr, a positive integer if this String is lexicographically
 *         greater than sStr, or zero if this String equals sStr.
 */
Int String::Compare(const Byte *sStr) const
{
	return strcmp(GetBuffer(), (sStr ? sStr : ""));
}

/**
 * Compares two Strings lexicographically and case-insensitively
 *
 * Compares two Strings lexicographically and case-insensitively.  The
 * comparison is performed as if both strings were converted to lowercase and
 * then compared.  If this String is lexicographically less than sStr, a
 * negative integer is returned.  If this String is lexicographically greater
 * than sStr, a positive integer is returned.  If this String equals sStr,
 * zero is returned.
 *
 * @param[in] sStr String to compare to this String
 *
 * @return A negative integer if this String is lexicographically less than
 *         sStr, a positive integer if this String is lexicographically
 *         greater than sStr, or zero if this String equals sStr.
 */
Int String::CompareASCIICaseInsensitive(const String & sStr) const
{
	return STRCMP_CASE_INSENSITIVE(GetBuffer(), sStr.GetBuffer());
}

/**
 * Compares two Strings lexicographically and case-insensitively
 *
 * Compares two Strings lexicographically and case-insensitively.  The
 * comparison is performed as if both strings were converted to lowercase and
 * then compared.  If this String is lexicographically less than sStr, a
 * negative integer is returned.  If this String is lexicographically greater
 * than sStr, a positive integer is returned.  If this String equals sStr,
 * zero is returned.
 *
 * @param[in] sStr String to compare to this String
 *
 * @return A negative integer if this String is lexicographically less than
 *         sStr, a positive integer if this String is lexicographically
 *         greater than sStr, or zero if this String equals sStr.
 */
Int String::CompareASCIICaseInsensitive(const Byte *sStr) const
{
	return STRCMP_CASE_INSENSITIVE(GetBuffer(), (sStr ? sStr : ""));
}

/**
 * Tests two Strings for equality
 *
 * Tests two Strings for equality.
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 and sStr2 are equal, or false otherwise
 */
Bool operator == (const String & sStr1, const String & sStr2)
{
	return (sStr1.GetSize() == sStr2.GetSize() && sStr1.Compare(sStr2) == 0);
}

/**
 * Tests a String and a C string for equality
 *
 * Tests a String and a C string for equality.
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 and sStr2 are equal, or false otherwise
 */
Bool operator == (const String & sStr1, const Byte *sStr2)
{
	return sStr1.Compare(sStr2) == 0;
}

/**
 * Tests a C string and a String for equality
 *
 * Tests a C string and a String for equality.
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 and sStr2 are equal, or false otherwise
 */
Bool operator == (const Byte *sStr1, const String & sStr2)
{
	return sStr2.Compare(sStr1) == 0;
}

/**
 * Tests two Strings for inequality
 *
 * Tests two Strings for inequality.
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 and sStr2 are unequal, or false otherwise
 */
Bool operator != (const String & sStr1, const String & sStr2)
{
	return (sStr1.GetSize() != sStr2.GetSize() || sStr1.Compare(sStr2) != 0);
}

/**
 * Tests a String and a C string for inequality
 *
 * Tests a String and a C string for inequality.
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 and sStr2 are unequal, or false otherwise
 */
Bool operator != (const String & sStr1, const Byte *sStr2)
{
	return sStr1.Compare(sStr2) != 0;
}

/**
 * Tests a C string and a String for inequality
 *
 * Tests a C string and a String for inequality.
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 and sStr2 are unequal, or false otherwise
 */
Bool operator != (const Byte *sStr1, const String & sStr2)
{
	return sStr2.Compare(sStr1) != 0;
}

/**
 * Tests if one String is lexicographically less than another
 *
 * Tests if one String is lexicographically less than another
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically less than sStr2, or false
 *         otherwise.
 */
Bool operator < (const String & sStr1, const String & sStr2)
{
	return sStr1.Compare(sStr2) < 0;
}

/**
 * Tests if a String is lexicographically less than a C string
 *
 * Tests if a String is lexicographically less than a C string
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically less than sStr2, or false
 *         otherwise.
 */
Bool operator < (const String & sStr1, const Byte *sStr2)
{
	return sStr1.Compare(sStr2) < 0;
}

/**
 * Tests if a C string is lexicographically less than a String
 *
 * Tests if a C string is lexicographically less than a String
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically less than sStr2, or false
 *         otherwise.
 */
Bool operator < (const Byte *sStr1, const String & sStr2)
{
	return sStr2.Compare(sStr1) > 0;
}

/**
 * Tests if one String is lexicographically less than or equal to another
 *
 * Tests if one String is lexicographically less than or equal to another
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically less than or equal to sStr2,
 *         or false otherwise.
 */
Bool operator <= (const String & sStr1, const String & sStr2)
{
	return sStr1.Compare(sStr2) <= 0;
}

/**
 * Tests if a String is lexicographically less than or equal to a C
 * string
 *
 * Tests if a String is lexicographically less than or equal to a C string
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically less than or equal to sStr2,
 *         or false otherwise.
 */
Bool operator <= (const String & sStr1, const Byte *sStr2)
{
	return sStr1.Compare(sStr2) <= 0;
}

/**
 * Tests if a C string is lexicographically less than or equal to a
 * String
 *
 * Tests if a C string is lexicographically less than or equal to a String
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically less than or equal to sStr2,
 *         or false otherwise.
 */
Bool operator <= (const Byte *sStr1, const String & sStr2)
{
	return sStr2.Compare(sStr1) >= 0;
}

/**
 * Tests if one String is lexicographically greater than another
 *
 * Tests if one String is lexicographically greater than another
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically greater than sStr2, or false
 *         otherwise.
 */
Bool operator > (const String & sStr1, const String & sStr2)
{
	return sStr1.Compare(sStr2) > 0;
}

/**
 * Tests if a String is lexicographically greater than a C string
 *
 * Tests if a String is lexicographically greater than a C string
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically greater than sStr2, or false
 *         otherwise.
 */
Bool operator > (const String & sStr1, const Byte *sStr2)
{
	return sStr1.Compare(sStr2) > 0;
}

/**
 * Tests if a C string is lexicographically greater than a String
 *
 * Tests if a C string is lexicographically greater than a String
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically greater than sStr2, or false
 *         otherwise.
 */
Bool operator > (const Byte *sStr1, const String & sStr2)
{
	return sStr2.Compare(sStr1) < 0;
}

/**
 * Tests if one String is lexicographically greater than or equal to
 * another
 *
 * Tests if one String is lexicographically greater than or equal to another
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically greater than or equal to
 *         sStr2, or false otherwise.
 */
Bool operator >= (const String & sStr1, const String & sStr2)
{
	return sStr1.Compare(sStr2) >= 0;
}

/**
 * Tests if a String is lexicographically greater than or equal to a C
 * string
 *
 * Tests if a String is lexicographically greater than or equal to a C string
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically greater than or equal to
 *         sStr2, or false otherwise.
 */
Bool operator >= (const String & sStr1, const Byte *sStr2)
{
	return sStr1.Compare(sStr2) >= 0;
}

/**
 * Tests if a C string is lexicographically greater than or equal to a
 * String
 *
 * Tests if a C string is lexicographically greater than or equal to a String
 *
 * @param[in] sStr1 First String to compare
 * @param[in] sStr2 Second String to compare
 *
 * @return True if sStr1 is lexicographically greater than or equal to
 *         sStr2, or false otherwise.
 */
Bool operator >= (const Byte *sStr1, const String & sStr2)
{
	return sStr2.Compare(sStr1) <= 0;
}

/**
 * Clears out this String to the empty string
 */
void String::Clear()
{
	if (!IsUsingInternalBuffer())
	{
		MemoryManager::Deallocate(m_buffer.pBuffer);
	}

	m_zSize = 0;
	m_zCapacity = kSmallBufferSize;
	m_buffer.aSmallBuffer[0] = 0;
}

/**
 * Shortens the string in-place
 *
 * Shortens the string to zSize bytes, if zSize is less than the current size;
 * otherwise, the string is not modified.  If offset zSize is is the middle of
 * a multibyte character, the behavior is undefined.
 */
void String::ShortenTo(UInt zSize)
{
	if (zSize < m_zSize)
	{
		GetBuffer()[zSize] = 0;
		m_zSize = zSize;
	}
}

/**
 * Reserves space in the internal buffer
 *
 * Reserves at least zCapacity bytes in the internal buffer.  This is used to
 * ensure that a series of Append() operations does not result in a large series
 * of reallocations, which hurts performance.  If it is known in advance even
 * approximately how much space will be needed for a series of Append()
 * operations, Reserve() should be used to speed that up.
 *
 * @param[in] zCapacity Minimum number of bytes to reserve in the internal
 *                      buffer
 */
void String::Reserve(UInt zCapacity)
{
	if (zCapacity > m_zCapacity)
	{
		SEOUL_ASSERT(zCapacity > kSmallBufferSize);
		if (IsUsingInternalBuffer())
		{
			Byte *pNewBuffer = (Byte*)MemoryManager::Allocate(zCapacity, MemoryBudgets::Strings);
			memcpy(pNewBuffer, m_buffer.aSmallBuffer, m_zSize + 1);
			m_buffer.pBuffer = pNewBuffer;
		}
		else
		{
			m_buffer.pBuffer = (Byte*)MemoryManager::Reallocate(
				m_buffer.pBuffer,
				zCapacity,
				MemoryBudgets::Strings);
		}

		m_zCapacity = zCapacity;
	}
}

/**
 * Trims the internal buffer to reduce memory usage
 *
 * Trims the internal buffer to reduce memory usage.  If the internal buffer
 * capacity is currently more than the size of the string, the buffer is
 * reallocated to be just equal to the size of the string to reduce memory
 * usage.
 */
void String::Trim()
{
	if (m_zCapacity > m_zSize + 1 && !IsUsingInternalBuffer())
	{
		if (m_zSize >= kSmallBufferSize)
		{
			m_buffer.pBuffer = (Byte*)MemoryManager::Reallocate(
				m_buffer.pBuffer,
				m_zSize + 1u,
				MemoryBudgets::Strings);

			m_zCapacity = m_zSize + 1;
		}
		else
		{
			// Don't remove this local variable, we need a sequence point
			// between the read of pBuffer and the write of aSmallBuffer
			// because of the union.
			Byte *pBuffer = m_buffer.pBuffer;
			memcpy(m_buffer.aSmallBuffer, pBuffer, m_zSize + 1);
			m_zCapacity = kSmallBufferSize;
		}
	}
}

/**
 * Finds the first occurrence of a Unicode character in this String
 *
 * Finds the first occurrence of the given Unicode character in this String.  If
 * the character is not found, NPos is returned.
 *
 * @param[in] chChar Character to find
 * @param[in] nStartIndex (Optional) Byte index to start the search at
 *
 * @return Byte index of the first occurrence of chChar in this String, or
 *         NPos if chChar does not occur
 */
UInt String::Find(UniChar chChar, UInt nStartIndex /* = 0 */) const
{
	if (nStartIndex >= m_zSize)
	{
		return NPos;
	}

	const Byte *pBuffer = GetBuffer();
	const Byte *pChar;
	if (chChar < 0x80)
	{
		// Handle ASCII with the more efficient strchr()
		pChar = strchr(pBuffer + nStartIndex, chChar);
	}
	else
	{
		Byte sChar[5];
		UInt nBytes = UTF8EncodeChar(chChar, sChar);
		sChar[nBytes] = '\0';

		pChar = strstr(pBuffer + nStartIndex, sChar);
	}

	if (pChar != nullptr)
	{
		return (UInt)(pChar - pBuffer);
	}
	else
	{
		return NPos;
	}
}

/**
 * Finds the first occurrence of a substring in this String
 *
 * Finds the first occurrence of the given String in this String.  If the
 * string is not found, NPos is returned.
 *
 * @param[in] sString Substring to find
 * @param[in] nStartIndex (Optional) Byte index to start the search at
 *
 * @return Byte index of the first occurrence of sString in this String, or
 *         NPos if sString does not occur as a substring
 */
UInt String::Find(const String & sString, UInt nStartIndex /* = 0 */) const
{
	if (nStartIndex >= m_zSize)
	{
		return NPos;
	}

	const Byte *pBuffer = GetBuffer();
	const Byte *pSubstr = strstr(pBuffer + nStartIndex, sString.GetBuffer());

	if (pSubstr == nullptr)
	{
		return NPos;
	}

	return (UInt)(pSubstr - pBuffer);
}

/**
 * Finds the last occurrence of a Unicode character in this String
 *
 * Finds the last occurrence of the given Unicode character in this String.  If
 * the character is not found, NPos is returned.
 *
 * @param[in] chChar Character to find
 * @param[in] nStartIndex (Optional) Byte index to start the search at
 *
 * @return Byte index of the larst occurrence of chChar in this String, or
 *         NPos if chChar does not occur
 */
UInt String::FindLast(UniChar chChar, UInt nStartIndex /* = NPos */) const
{
	if (m_zSize == 0)
	{
		return NPos;
	}

	if (nStartIndex >= m_zSize)
	{
		nStartIndex = m_zSize - 1;
	}

	const Byte *pBuffer = GetBuffer();
	Int nIndex = nStartIndex;
	if (chChar < 0x80)
	{
		while(nIndex >= 0 && pBuffer[nIndex] != (Byte)chChar)
		{
			nIndex--;
		}
	}
	else
	{
		Byte sChar[5];
		UInt nBytes = UTF8EncodeChar(chChar, sChar);
		sChar[nBytes] = '\0';

		nIndex -= nBytes - 1;
		while(nIndex >= 0 && memcmp(pBuffer + nIndex, sChar, nBytes) != 0)
		{
			nIndex--;
		}
	}

	return (nIndex >= 0) ? nIndex : NPos;
}

/**
 * Finds the last occurrence of a substring in this String
 *
 * Finds the last occurrence of the given String in this String.  If the
 * string is not found, NPos is returned.
 *
 * @param[in] sString Substring to find
 * @param[in] nStartIndex (Optional) Byte index to start the search at
 *
 * @return Byte index of the last occurrence of sString in this String, or
 *         NPos if sString does not occur as a substring
 */
UInt String::FindLast(const String & sString, UInt nStartIndex /* = NPos */) const
{
	if (m_zSize == 0)
	{
		return NPos;
	}

	if (nStartIndex >= m_zSize)
	{
		nStartIndex = m_zSize - 1;
	}

	// If sString is empty, this will set nIndex to m_zSize, but that's ok
	Int nIndex = nStartIndex - (sString.GetSize() - 1);

	const Byte *pBuffer = GetBuffer();
	const Byte *pOtherBuffer = sString.GetBuffer();
	while(nIndex >= 0 && memcmp(pBuffer + nIndex, pOtherBuffer, sString.m_zSize) != 0)
	{
		nIndex--;
	}

	return (nIndex >= 0) ? nIndex : NPos;
}

/**
 * Finds the first occurrence of any character in a character set in this
 *        String
 *
 * Finds the first occurrence of any character in sCharSet in this String.
 * If none of the characters in sCharSet are found, NPos is returned.
 *
 * @param[in] sCharSet Character set to search for
 * @param[in] nStartIndex (Optional) Byte index to start the search at
 *
 * @return Byte index of the first occurrence of any of the characters in
 *         sCharSet, or NPos if none of the characters occur
 */
UInt String::FindFirstOf(const String & sCharSet, UInt nStartIndex /* = 0 */) const
{
	if (nStartIndex >= m_zSize)  // Automatically handles the m_zSize==0 case
	{
		return NPos;
	}

	// Optimize for the common case, where the character set is ASCII
	if (sCharSet.IsASCII())
	{
		// strcspn returns the number of initial characters not in the string
		// defined by the second parameter
		UInt zIndex = (UInt)strcspn(GetBuffer() + nStartIndex, sCharSet.GetBuffer());
		zIndex += nStartIndex;
		return ((zIndex < m_zSize) ? zIndex : NPos);
	}
	else
	{
		// For non-ASCII character sets, just do the stupid slow way
		Iterator iter = Iterator(CStr(), nStartIndex), end = End();
		Iterator charSetEnd = sCharSet.End();

		for ( ; iter != end; ++iter)
		{
			UniChar chChar = *iter;

			for (Iterator charSetIter = sCharSet.Begin(); charSetIter != charSetEnd; ++charSetIter)
			{
				if (chChar == *charSetIter)
				{
					return iter.GetIndexInBytes();
				}
			}
		}

		return NPos;
	}
}

/**
 * Finds the first occurrence of any character not in a character set in
 *        this String
 *
 * Finds the first occurrence of any character not in sCharSet in this
 * String.  If every character in this String is a member of sCharSet, then
 * NPos is returned.
 *
 * @param[in] sCharSet Complement of character set to search for
 * @param[in] nStartIndex (Optional) Byte index to start the search at
 *
 * @return Byte index of the first occurrence of a character not in sCharSet,
 *         or NPos if all characters are in sCharSet.
 */
UInt String::FindFirstNotOf(const String & sCharSet, UInt nStartIndex /* = 0 */) const
{
	if (nStartIndex >= m_zSize)  // Automatically handles the m_zSize==0 case
	{
		return NPos;
	}

	// Optimize for the common case, where the character set is ASCII
	if (sCharSet.IsASCII())
	{
		// strspn returns the number of initial characters in the string
		// defined by the second parameter
		UInt zIndex = (UInt)strspn(GetBuffer() + nStartIndex, sCharSet.GetBuffer());
		zIndex += nStartIndex;
		return ((zIndex < m_zSize) ? zIndex : NPos);
	}
	else
	{
		// For non-ASCII character sets, just do the stupid slow way
		Iterator iter = Iterator(CStr(), nStartIndex), end = End();
		Iterator charSetEnd = sCharSet.End();

		for ( ; iter != end; ++iter)
		{
			UniChar chChar = *iter;
			Bool bFound = false;

			for (Iterator charSetIter = sCharSet.Begin(); charSetIter != charSetEnd; ++charSetIter)
			{
				if (chChar == *charSetIter)
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				return iter.GetIndexInBytes();
			}
		}

		return NPos;
	}
}

/**
 * Finds the last occurrence of any character in a character set in this
 *        String
 *
 * Finds the last occurrence of any character in sCharSet in this String.  If
 * none of the characters in sCharSet are found, NPos is returned.
 *
 * @param[in] sCharSet Character set to search for
 * @param[in] nStartIndex (Optional) Index to start the search at
 *
 * @return Byte index of the last occurrence of any of the characters in
 *         sCharSet, or NPos if none of the characters occur
 */
UInt String::FindLastOf(const String & sCharSet, UInt nStartIndex /* = NPos */) const
{
	if (m_zSize == 0)
	{
		return NPos;
	}

	if (nStartIndex >= m_zSize)
	{
		nStartIndex = m_zSize - 1;
	}

	// Optimize for the common case, where the character set is ASCII
	if (sCharSet.IsASCII())
	{
		// Too bad there's no strrcspn()
		const Byte *pBuffer = GetBuffer();
		const Byte *pCharSetBuffer = sCharSet.GetBuffer();
		for (Int nIndex = nStartIndex; nIndex >= 0; nIndex--)
		{
			if ((pBuffer[nIndex] & 0xC0) != 0x80 &&
			   strchr(pCharSetBuffer, pBuffer[nIndex]))
			{
				return nIndex;
			}
		}

		return NPos;
	}
	else
	{
		// For non-ASCII character sets, just do the stupid slow way
		Iterator iter = Iterator(CStr(), nStartIndex);
		Iterator begin = Begin();
		Iterator charSetEnd = sCharSet.End();

		while(1)
		{
			UniChar chChar = *iter;

			for (Iterator charSetIter = sCharSet.Begin(); charSetIter != charSetEnd; ++charSetIter)
			{
				if (chChar == *charSetIter)
				{
					return iter.GetIndexInBytes();
				}
			}

			if (iter != begin)
			{
				--iter;
			}
			else
			{
				break;
			}
		}

		return NPos;
	}
}

/**
 * Finds the last occurrence of any character not in a character set in
 *        this String
 *
 * Finds the last occurrence of any character not in sCharSet in this String.
 * If every character in this String is a member of sCharSet, then NPos is
 * returned.
 *
 * @param[in] sCharSet Complement of character set to search for
 * @param[in] nStartIndex (Optional) Index to start the search at
 *
 * @return Byte index of the last occurrence of a character not in sCharSet,
 *         or NPos if all characters are in sCharSet.
 */
UInt String::FindLastNotOf(const String & sCharSet, UInt nStartIndex /* = NPos */) const
{
	if (m_zSize == 0)
	{
		return NPos;
	}

	if (nStartIndex >= m_zSize)
	{
		nStartIndex = m_zSize - 1;
	}

	// Optimize for the common case, where the character set is ASCII
	if (sCharSet.IsASCII())
	{
		// Too bad there's no strrcspn()
		const Byte *pBuffer = GetBuffer();
		const Byte *pCharSetBuffer = sCharSet.GetBuffer();
		for (Int nIndex = nStartIndex; nIndex >= 0; nIndex--)
		{
			if ((pBuffer[nIndex] & 0xC0) != 0x80 &&
			   !strchr(pCharSetBuffer, pBuffer[nIndex]))
			{
				return nIndex;
			}
		}

		return NPos;
	}
	else
	{
		// For non-ASCII character sets, just do the stupid slow way
		Iterator iter = Iterator(CStr(), nStartIndex);
		Iterator begin = Begin();
		Iterator charSetEnd = sCharSet.End();

		while(1)
		{
			UniChar chChar = *iter;
			Bool bFound = false;

			for (Iterator charSetIter = sCharSet.Begin(); charSetIter != charSetEnd; ++charSetIter)
			{
				if (chChar == *charSetIter)
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				return iter.GetIndexInBytes();
			}

			if (iter != begin)
			{
				--iter;
			}
			else
			{
				break;
			}
		}

		return NPos;
	}
}

/**
 * Returns true if the given String is a prefix of this String.
 *
 * @param[in] sStr The prefix to test.
 * @return True if the first sStr.GetUnicodeLength() characters of this String match sStr.
 */

Bool String::StartsWith(const String & sStr) const
{
	return (m_zSize >= sStr.m_zSize &&
			memcmp(GetBuffer(), sStr.GetBuffer(), sStr.m_zSize) == 0);
}

/**
 * Returns true if the given String is a prefix of this String. This
 * comparison is not case sensitive.
 *
 * @param[in] sStr The prefix to test.
 * @return True if the first sStr.GetSize() characters of this String match sStr.
 */
Bool String::StartsWithASCIICaseInsensitive(const String & sStr) const
{
	return (m_zSize >= sStr.m_zSize &&
			STRNCMP_CASE_INSENSITIVE(GetBuffer(), sStr.GetBuffer(), sStr.m_zSize) == 0);
}

/**
 * Returns true if the given String is a suffix of this String.
 *
 * @param[in] sStr The suffix to test.
 * @return True if the last sStr.GetUnicodeLength() characters of this String match sStr.
 */
Bool String::EndsWith(const String & sStr) const
{
	// Substring accepts byte indices, not character indices, so use GetSize():
	return (m_zSize >= sStr.m_zSize &&
			memcmp(GetBuffer() + m_zSize - sStr.m_zSize, sStr.GetBuffer(), sStr.m_zSize) == 0);
}

/**
 * Returns a substring of this String
 *
 * Returns the substring of this String beginning at the byte index nIndex
 * and continuing to the end of the string.  If nIndex is an index of a byte
 * which is not the first byte of a character, the results are undefined.  If
 * nIndex is greater than the size of this String, the result is the empty
 * string.
 *
 * @return The substring of this String beginning at byte index nIndex and
 *         continuing to the end of the string
 */
String String::Substring(UInt nIndex) const
{
	if (nIndex >= m_zSize)
	{
		return String();
	}
	else
	{
		return String(GetBuffer() + nIndex, m_zSize - nIndex);
	}
}

/**
 * Returns a substring of this String
 *
 * Returns the substring of this String beginning at the byte index nIndex
 * and consisting of at most zSize bytes.  If nIndex is an index of a byte
 * which is not the first byte of a character, or if the last byte specified is
 * not the last byte of a character, the results are undefined.  If nIndex is
 * greater than the size of this String, the result is the empty string.
 *
 * @return The substring of this String beginning at byte index nIndex and
 *         continuing for at most zSize bytes
 */
String String::Substring(UInt nIndex, UInt zSize) const
{
	if (nIndex >= m_zSize)
	{
		return String();
	}

	if (zSize > m_zSize - nIndex)
	{
		zSize = m_zSize - nIndex;
	}

	return String(GetBuffer() + nIndex, zSize);
}

/**
 * Output the string's data to rpData. A new buffer will be allocated
 * if this String is using internal storage.
 */
void String::RelinquishBuffer(void*& rpData, UInt32& ruData)
{
	if (IsUsingInternalBuffer())
	{
		rpData = MemoryManager::Allocate(m_zSize+1u, MemoryBudgets::Strings);
		memcpy(rpData, GetBuffer(), m_zSize+1u);
	}
	else
	{
		rpData = MemoryManager::Reallocate(m_buffer.pBuffer, m_zSize+1u, MemoryBudgets::Strings);
	}
	ruData = m_zSize;

	// Clear
	m_zSize = 0;
	m_zCapacity = kSmallBufferSize;
	m_buffer.aSmallBuffer[0] = 0;
}

/**
 * Replaces all occurrences of a substring with another string
 *
 * Replaces all occurrences of the substring sPattern with the string
 * sReplacement.  The resulting string is returned, and this string is not
 * modified.  If the replacement string contains the pattern string or ends with
 * a prefix thereof, the replacement is NOT replaced recursively.
 *
 * @param[in] sPattern     Pattern string to replace
 * @param[in] sReplacement Replacement string to insert
 *
 * @return The resulting string, with all occurrences of sPattern replaced
 *         with sReplacement
 */
String String::ReplaceAll(const String & sPattern, const String & sReplacement) const
{
	UInt zPatternSize = sPattern.GetSize();

	String sResult;
	UInt uCurrentIndex = 0u;
	UInt uIndex = 0u;
	while((uIndex = Find(sPattern, uCurrentIndex)) != NPos)
	{
		SEOUL_ASSERT(uIndex >= uCurrentIndex);

		sResult.Append(Substring(uCurrentIndex, uIndex - uCurrentIndex));
		sResult.Append(sReplacement);
		uCurrentIndex = uIndex + zPatternSize;
	}

	return sResult.Append(Substring(uCurrentIndex));
}

/**
 * Returns the reversal of this string
 *
 * @return The reversal of this string
 */
String String::Reverse() const
{
	String sResult;
	sResult.Reserve(m_zSize + 1);
	sResult.m_zSize = m_zSize;

	// Special case for empty string
	if (IsEmpty())
	{
		return sResult;
	}

	const Byte *pBufferPtr = GetBuffer();
	const Byte *pBufferEnd = pBufferPtr + m_zSize;
	Byte *pOutBuffer = sResult.GetBuffer();
	Byte *pOutBufferPtr = pOutBuffer + m_zSize;

	*pOutBufferPtr = 0;

	while (pBufferPtr < pBufferEnd)
	{
		// Fast case for ASCII
		Byte cLeadByte = *pBufferPtr;
		if ((cLeadByte & 0x80) == 0x00)
		{
			*--pOutBufferPtr = *pBufferPtr++;
		}
		else if ((cLeadByte & 0xE0) == 0xC0)
		{
			// 2-byte character
			pOutBufferPtr -= 2;
			memcpy(pOutBufferPtr, pBufferPtr, 2);
			pBufferPtr += 2;
		}
		else if ((cLeadByte & 0xF0) == 0xE0)
		{
			// 3-byte character
			pOutBufferPtr -= 3;
			memcpy(pOutBufferPtr, pBufferPtr, 3);
			pBufferPtr += 3;
		}
		else
		{
			// Assume 4-byte character
			SEOUL_ASSERT_DEBUG((cLeadByte & 0xF8) == 0xF0);

			pOutBufferPtr -= 4;
			memcpy(pOutBufferPtr, pBufferPtr, 4);
			pBufferPtr += 4;
		}
	}

	SEOUL_ASSERT(pBufferPtr == pBufferEnd);
	SEOUL_ASSERT(pOutBufferPtr == pOutBuffer);

	return sResult;
}

/**
 * Returns an all uppercase version of this string.  Does not modify this
 * string.  Nearly full support for Unicode.
 *
 * @param[in] sLocaleID Lowercase two-letter ISO 639-1 language code of the
 *              locale to perform the case conversion in.  In most cases, this
 *              doesn't make a difference, but it sometimes does.  For example,
 *              in the "en" locale, "i" maps to "I" (U+0049, LATIN CAPITAL
 *              LETTER I), but in the "tr" or "az" locales, "i" maps to U+0130,
 *              LATIN CAPITAL LETTER I WITH DOT ABOVE.
 *
 * @return The uppercase version of this string
 */
String String::ToUpper(const String& sLocaleID) const
{
	return InternalCaseMap(sLocaleID, CaseMappingInternal::g_UppercaseTable);
}

/**
 * Returns an all lowercase version of this string.  Does not modify this
 * string.  Nearly full support for Unicode.
 *
 * @param[in] sLocaleID Lowercase two-letter ISO 639-1 language code of the
 *              locale to perform the case conversion in.  In most cases, this
 *              doesn't make a difference, but it sometimes does.  For example,
 *              in the "en" locale, "I" maps to "i" (U+0069, LATIN SMALL LETTER
 *              I), but in the "tr" or "az" locales, "I" maps to U+0131, LATIN
 *              SMALL LETTER DOTLESS I.
 *
 * @return The lowercase version of this string
 */
String String::ToLower(const String& sLocaleID) const
{
	return InternalCaseMap(sLocaleID, CaseMappingInternal::g_LowercaseTable);
}

/**
 * Converts this string to uppercase or to lowercase according to a case
 * mapping table.
 *
 * See the comments in SeoulStringInternal.h for an overview of the data
 * structures used and how this works
 */
String String::InternalCaseMap(const String& sLocaleID, const CaseMappingInternal::RootTable& rCaseTable) const
{
	using namespace CaseMappingInternal;

	// Reserve enough space for a string of the same length.  Note that some
	// case mappings can change the length of the string, but that's ok since
	// this is just a memory optimization.  If the length gets longer than
	// this, we'll just do an ordinary reallocation.
	String sResult;
	sResult.Reserve(m_zSize + 1);

	const Byte *pBufferStart = GetBuffer();
	const Byte *pBuffer = pBufferStart;
	const Byte *pBufferEnd = pBuffer + m_zSize;

	// Check the locale.  We only have special handling for 3 locales at the
	// moment.
	Bool bIsLithuanian = (sLocaleID == "lt");
	Bool bIsTurkishOrAzeri = (!bIsLithuanian && (sLocaleID == "tr" || sLocaleID == "az"));

	// Walk the string and convert each character
	while (pBuffer < pBufferEnd)
	{
		UByte uLeadByte = *pBuffer;
		UInt uCharLength;
		const CharEntry *pEntry = nullptr;

		// If this character is a 1-byte character, get the leaf entry directly
		// from the root table
		if ((uLeadByte & 0x80) == 0x00)  // 1-byte character (0xxxxxxx)
		{
			uCharLength = 1;
			pEntry = rCaseTable.m_apBaseEntries[uLeadByte];
		}
		else
		{
			// Otherwise, look up this character in the table.  Its depth in
			// the table equals the length of its UTF-8 encoding.
			if ((uLeadByte & 0xE0) == 0xC0)  // 2-byte character (110xxxxx 10xxxxxx)
			{
				uCharLength = 2;
			}
			else if ((uLeadByte & 0xF0) == 0xE0)  // 3-byte character (1110xxxx 10xxxxxx 10xxxxxx)
			{
				uCharLength = 3;
			}
			else if ((uLeadByte & 0xF8) == 0xF0)  // 4-byte character (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
			{
				uCharLength = 4;
			}
			else
			{
				SEOUL_LOG("InternalCaseMap: Invalid lead byte 0x%02x", uLeadByte);
				break;
			}

			// Search uCharLength levels down into the table.  If at some point
			// we don't find a valid node, this character has no case mapping.
			const SubTable *pSubTable = rCaseTable.m_apSubTables[uLeadByte - 0xC0];
			if (pSubTable == nullptr)
			{
				goto NoCaseMap;
			}

			for (UInt i = 1; i < uCharLength - 1; i++)
			{
				UShort uSubTableIndex = pSubTable->m_auChildIndices[((UByte)pBuffer[i]) & 0x3F];
				if (uSubTableIndex != USHRT_MAX)
				{
					pSubTable = &rCaseTable.m_pAllSubTables[uSubTableIndex];
				}
				else
				{
					goto NoCaseMap;
				}
			}

			UShort uEntryIndex = pSubTable->m_auChildIndices[((UByte)pBuffer[uCharLength - 1]) & 0x3F];
			if (uEntryIndex != USHRT_MAX)
			{
				pEntry = &rCaseTable.m_pAllEntries[uEntryIndex];
			}
		}

		if (pEntry != nullptr)
		{
			// If we found a valid case mapping for this character, try to
			// aterate through all of the possible mappings until we find a
			// matching entry, given the current conditions.
			Bool bFoundMatch = false;
			while (1)
			{
				// Common case: no flags
				UShort uFlags = pEntry->m_uFlags;
				if (uFlags == 0)
				{
					bFoundMatch = true;
					break;
				}

				// Check language flags
				if (((uFlags & kFlagLithuanian) && !bIsLithuanian) ||
					((uFlags & kFlagTurkishAzeri) && !bIsTurkishOrAzeri))
				{
					goto NextEntry;
				}

				// The following context definitions are from Table 3-14
				// "Context Specification for Casing" from Chapter 3.13
				// "Default Case Algorithms" of version 6.3.0 of the Unicode
				// standard
				// (http://www.unicode.org/versions/Unicode6.2.0/ch03.pdf).
				// Note that the core specification was unchanged between
				// versions 6.2.0 and 6.3.0, except for definition D136, whose
				// textual change can be found here:
				// http://www.unicode.org/versions/Unicode6.3.0/#Character_Additions

				// Check final sigma flag
				if (uFlags & kFlagFinalSigma)
				{
					// TODO: Check if we're currently at the end of a word
					SEOUL_LOG("InternalCaseMap: Unimplemented flag: kFlagFinalSigma");

					// Final_Sigma spec: "C is preceded by a sequence
					// consisting of a cased letter and then zero or more
					// case-ignorable characters, and C is not followed by a
					// sequence consisting of zero or more case-ignorable
					// characters and then a cased letter."
					goto NextEntry;
				}

				// Check other flags
				if (uFlags & kFlagAfterSoftDotted)
				{
					// TODO: Check if previous character is soft dotted
					SEOUL_LOG("InternalCaseMap: Unimplemented flag: kFlagAfterSoftDotted");

					// After_Soft_Dotted spec: "There is a Soft_Dotted
					// character before C, with no intervening character of
					// combining class 0 or 230 (Above)."
					goto NextEntry;
				}

				if (uFlags & kFlagAfterI)
				{
					// TODO: Check for intervening combining characters
					SEOUL_LOG("InternalCaseMap: kFlagAfterI: Need to check for intervening combining characters");

					// After_I spec: "There is an uppercase I before C, and
					// there is no intervening combining character class 230
					// (Above) or 0."

					// Check if previous character was "I" (U+0049)
					if (!(pBuffer > pBufferStart && pBuffer[-1] == 'I'))
					{
						goto NextEntry;
					}
				}

				if (uFlags & kFlagMoreAbove)
				{
					// TODO: Check if previous character is a combining
					// character above
					SEOUL_LOG("InternalCaseMap: Unimplemented flag: kFlagMoreAbove");

					// More_Above spec: "C is followed by a character of
					// combining class 230 (Above) with no intervening
					// character of combining class 0 or 230 (Above)."
					goto NextEntry;
				}

				if (uFlags & kFlagNotBeforeDot)
				{
					// TODO: Check for intervening combining characters
					SEOUL_LOG("InternalCaseMap: kFlagNotBeforeDot: Need to check for intervening combining characters");

					// Before_Dot spec: "C is followed by combining dot above
					// (U+0307). Any sequence of characters with a combining
					// class that is neither 0 nor 230 may intervene between
					// the current character and the combining dot above."

					// Check if the next character is a combining dot above
					// (U+0307)
					if (pBuffer + 2 < pBufferEnd &&
						(UByte)pBuffer[1] == 0xcc &&
						(UByte)pBuffer[2] == 0x87)
					{
						goto NextEntry;
					}
				}

				// If we got here, all conditions passed
				bFoundMatch = true;
				break;

			NextEntry:
				// If we jumped here, the flags didn't match for some reason.
				// Try the next entry for this character, if there is one.
				if (uFlags & kFlagMoreEntries)
				{
					pEntry++;
				}
				else
				{
					break;
				}
			}

			// If we found a valid, matching case mapping entry, append the
			// string data to the result
			if (bFoundMatch)
			{
				const Byte *pStr = rCaseTable.m_pStringPool + pEntry->m_uStrOffset;
				sResult.Append(pStr, pEntry->m_Length);
			}
			else
			{
				// If no case mappings matched, append the current character
				// unmodified
				sResult.Append(pBuffer, uCharLength);
			}
		}
		else
		{
		NoCaseMap:
			// If we didn't find a case mapping for this character, append the
			// current character unmodified
			sResult.Append(pBuffer, uCharLength);
		}

		// Move on to the next character in the input
		pBuffer += uCharLength;
	}

	return sResult;
}

/**
 * Returns an all uppercase version of this string.  Does not modify this
 * string.  Only converts ASCII characters.
 *
 * @return The uppercase version of this string
 */
String String::ToUpperASCII() const
{
	String sResult;
	sResult.Reserve(m_zSize + 1);
	sResult.m_zSize = m_zSize;

	const Byte *pBuffer = GetBuffer();
	const Byte *pBufferEnd = pBuffer + m_zSize;
	Byte *pOutBuffer = sResult.GetBuffer();

	while (pBuffer < pBufferEnd)
	{
		Byte c = *pBuffer;
		if (c >= 'a' && c <= 'z')
		{
			*pOutBuffer = c + ('A' - 'a');
		}
		else
		{
			*pOutBuffer = c;
		}

		pBuffer++;
		pOutBuffer++;
	}

	*pOutBuffer = 0;
	SEOUL_ASSERT(pOutBuffer < pOutBuffer + sResult.m_zCapacity);

	return sResult;
}

/**
 * Returns an all uppercase version of this string.  Does not modify this
 * string.  Only converts ASCII characters.
 *
 * @return The uppercase version of this string
 */
String String::ToLowerASCII() const
{
	String sResult;
	sResult.Reserve(m_zSize + 1);
	sResult.m_zSize = m_zSize;

	const Byte *pBuffer = GetBuffer();
	const Byte *pBufferEnd = pBuffer + m_zSize;
	Byte *pOutBuffer = sResult.GetBuffer();

	while (pBuffer < pBufferEnd)
	{
		Byte c = *pBuffer;
		if (c >= 'A' && c <= 'Z')
		{
			*pOutBuffer = c + ('a' - 'A');
		}
		else
		{
			*pOutBuffer = c;
		}

		pBuffer++;
		pOutBuffer++;
	}

	*pOutBuffer = 0;
	SEOUL_ASSERT(pOutBuffer < pOutBuffer + sResult.m_zCapacity);

	return sResult;
}

/**
 * Tests if the String contains only ASCII characters
 *
 * Tests if the String contains only ASCII characters.  An ASCII character is
 * a character with a code point between U+0000 and U+007F inclusive, i.e.
 * between 0 and 127.  If the String contains only ASCII characters, it can
 * safely be passed to standard library functions expecting ASCII strings.
 *
 * @return True if the String contains only ASCII characters (or is empty), or
 *         false if the String contains at least one non-ASCII character
 */
Bool String::IsASCII() const
{
	const Byte *pBuffer = GetBuffer();
	for (UInt32 i = 0; i < m_zSize; i++)
	{
		if ((pBuffer[i] & 0x80) != 0)
		{
			return false;
		}
	}

	return true;
}

/**
 * Converts the string to a wide-character C string (e.g. for Win32 API
 * functions).  The returned WString object automatically manages the memory
 * for the returned string.
 */
WString String::WStr() const
{
	return WString(*this);
}

/**
 * Returns the length of the string in Unicode characters
 *
 * For a string containing non-ASCII characters, this will be different
 * than the string's size in bytes
 *
 * This should rarely be used in string handling code; this should
 * probably only be used for things such as character rendering etc.  In most
 * cases, you want GetSize.
 *
 * \sa String::GetSize
 */
UInt String::GetUnicodeLength() const
{
	return UTF8Strlen(GetBuffer(), GetSize());
}


/** Returns the Unicode character starting at the given *byte* index */
UniChar String::CharAtByteIndex(UInt nIndex) const
{
	return UTF8DecodeChar(GetBuffer() + nIndex);
}

/**
 * Swaps this String with another String
 *
 * @param[in,out] sOther String to swap with this String
 */
void String::Swap(String& sOther)
{
	// First, swap the buffers, which breaks into 4 cases depending on whether
	// we're using the internal buffer and whether the other string is using
	// the internal buffer
	if (IsUsingInternalBuffer())
	{
		char sTemp[kSmallBufferSize];
		memcpy(sTemp, m_buffer.aSmallBuffer, m_zSize + 1);

		if (sOther.IsUsingInternalBuffer())
		{
			memcpy(m_buffer.aSmallBuffer, sOther.m_buffer.aSmallBuffer, sOther.m_zSize + 1);
		}
		else
		{
			m_buffer.pBuffer = sOther.m_buffer.pBuffer;
		}

		memcpy(sOther.m_buffer.aSmallBuffer, sTemp, m_zSize + 1);
	}
	else
	{
		char *psTemp = m_buffer.pBuffer;

		if (sOther.IsUsingInternalBuffer())
		{
			memcpy(m_buffer.aSmallBuffer, sOther.m_buffer.aSmallBuffer, sOther.m_zSize + 1);
		}
		else
		{
			m_buffer.pBuffer = sOther.m_buffer.pBuffer;
		}

		sOther.m_buffer.pBuffer = psTemp;
	}

	// Now swap the other relevant data members
	Seoul::Swap(m_zSize, sOther.m_zSize);
	Seoul::Swap(m_zCapacity, sOther.m_zCapacity);
}

/**
 * Returns an iterator pointing to the beginning of this String
 *
 * Returns an iterator pointing to the beginning of this String.
 *
 * @return An iterator pointing to the beginning of this String
 */
String::Iterator String::Begin() const
{
	return Iterator(CStr(), 0);
}

/**
 * Returns an iterator pointing to the end of this String
 *
 * Returns an iterator pointing to the end of this String.
 *
 * @return An iterator pointing to the end of this String
 */
String::Iterator String::End() const
{
	return Iterator(CStr(), m_zSize);
}

/**
 * Remove the last element of this String. SEOUL_FAIL() if the String IsEmpty().
 */
void String::PopBack()
{
	// String must have at least 1 character.
	SEOUL_ASSERT(!IsEmpty());

	// Back up to a maximum of 4 bytes (4-byte Unicode).
	UInt32 uBackup = 1u;

	// Find the beginning of the last character.
	while (
		uBackup <= 4u &&
		uBackup <= GetSize() &&
		!IsValidUTF8String(CStr() + GetSize() - uBackup))
	{
		++uBackup;
	}

	// Shorten the string to the beginning of the last character.
	ShortenTo(GetSize() - uBackup);
}

/** Swap the internal buffer of String with a raw byte buffer allocated with MemoryManager::Allocate. */
void String::TakeOwnership(void*& rpData, UInt32& ruSizeInBytes)
{
	auto const pData = rpData;
	rpData = nullptr;
	auto const uSizeInBytes = ruSizeInBytes;
	ruSizeInBytes = 0u;

	if (!IsUsingInternalBuffer())
	{
		MemoryManager::Deallocate(m_buffer.pBuffer);
	}

	m_zSize = uSizeInBytes;
	m_zCapacity = Max(uSizeInBytes + 1, (UInt32)kSmallBufferSize);
	if (IsUsingInternalBuffer())
	{
		memcpy(m_buffer.aSmallBuffer, pData, uSizeInBytes);
		m_buffer.aSmallBuffer[uSizeInBytes] = '\0';
		MemoryManager::Deallocate(pData);
	}
	else
	{
		// Make sure there is enough room for the null terminator. We don't
		// require this (the input may be a string without null termination).
		m_buffer.pBuffer = (Byte*)MemoryManager::Reallocate(pData, uSizeInBytes + 1u, MemoryBudgets::Strings);
		m_buffer.pBuffer[uSizeInBytes] = '\0';
	}
}

/**
 * Creates a formatted String printf-style
 *
 * Creates a formatted String printf-style.  The size of the buffer needed is
 * automatically calculated.
 *
 * @param[in] sFormat Format string
 * @param[in] ...     Arguments
 *
 * @return A formatted String
 */
String String::Printf(const Byte *sFormat, ...)
{
	va_list argList;
	va_start(argList, sFormat);

	String sResult = String::VPrintf(sFormat, argList);

	va_end(argList);

	return sResult;
}

/**
 * Creates a formatted String printf-style
 *
 * Creates a formatted String printf-style.  The size of the buffer needed is
 * automatically calculated.
 *
 * @param[in] sFormat Format string
 * @param[in] argList Argument list; must be produced from a variadic function
 *
 * @return A formatted String
 */
String String::VPrintf(const Byte *sFormat, va_list argList)
{
	// Calculate size of formatted string
	va_list argListCopy;
	VA_COPY(argListCopy, argList);
	Int zRequiredSize = VSCPRINTF(sFormat, argListCopy);
	va_end(argListCopy);

	SEOUL_ASSERT(zRequiredSize >= 0);

	// Reserve memory
	String sResult;
	sResult.Reserve((UInt)zRequiredSize + 1);
	sResult.m_zSize = (UInt)zRequiredSize;

	// Print straight into the String's buffer
	SEOUL_VERIFY(zRequiredSize == VSNPRINTF(sResult.GetBuffer(), sResult.m_zCapacity, sFormat, argList));

	return sResult;
}

/**
 * Creates a formatted String printf-style, with wide character arguments.
 *
 * Creates a formatted String printf-style.  The size of the buffer needed is
 * automatically calculated.
 *
 * @param[in] sFormat Format string
 * @param[in] ...     Arguments
 *
 * @return A formatted String
 */
String String::WPrintf(wchar_t const* sFormat, ...)
{
	va_list argList;
	va_start(argList, sFormat);

	String sResult = String::WVPrintf(sFormat, argList);

	va_end(argList);

	return sResult;
}

/**
 * Creates a formatted String printf-style, with wide character arguments.
 *
 * Creates a formatted String printf-style.  The size of the buffer needed is
 * automatically calculated.
 *
 * @param[in] sFormat Format string
 * @param[in] argList Argument list; must be produced from a variadic function
 *
 * @return A formatted String
 */
String String::WVPrintf(wchar_t const* sFormat, va_list argList)
{
	// Calculate size of formatted string
	va_list argListCopy;
	VA_COPY(argListCopy, argList);
	Int zRequiredSize = VSCWPRINTF(sFormat, argListCopy);
	va_end(argListCopy);

	SEOUL_ASSERT(zRequiredSize >= 0);

	// Create a buffer with enough space for the entire formatted string.
	Vector<wchar_t, MemoryBudgets::Strings> vBuffer(zRequiredSize + 1);

	// Print into the temporary buffer.
	SEOUL_VERIFY(zRequiredSize == VSNWPRINTF(vBuffer.Get(0), vBuffer.GetSize(), sFormat, argList));

	// Assign to the return string.
	return WCharTToUTF8(vBuffer.Get(0));
}

/** Constructs an empty WString */
WString::WString()
	: m_zLength(0)
{
}

/** WString Destructor */
WString::~WString()
{
	// ScopedArray destructor frees the memory for us
}

/** Converts a UTF-8 String to a wide-character string */
WString::WString(const String& sStr)
{
	if (sStr.IsEmpty())
	{
		m_pwBuffer.Reset();
		m_zLength = 0u;
	}
	else
	{
		UInt zLength = sStr.GetUnicodeLength();
		SEOUL_ASSERT(zLength < UINT_MAX / 2);

		// Each Unicode character can occupy up to two wchar_t's (if it's above
		// U+FFFF), and we add one for the null terminator
		UInt zMaxChars = zLength * 2 + 1;

		// Allocate the buffer and do the string conversion
		m_pwBuffer.Reset(SEOUL_NEW(MemoryBudgets::Strings) wchar_t[zMaxChars]);
		m_zLength = UTF8ToWCharT(sStr.CStr(), m_pwBuffer.Get(), zMaxChars) - 1;
		SEOUL_ASSERT(m_zLength < zMaxChars);
	}
}

/** Copy constructor */
WString::WString(const WString& wStr)
{
	if (!wStr.m_pwBuffer.IsValid() || wStr.m_zLength == 0)
	{
		// Empty.
		m_pwBuffer.Reset();
		m_zLength = 0u;
	}
	else
	{
		// Copy the char buffer
		m_zLength = wStr.m_zLength;
		m_pwBuffer.Reset(SEOUL_NEW(MemoryBudgets::Strings) wchar_t[m_zLength + 1]);
		memcpy(m_pwBuffer.Get(), wStr.m_pwBuffer.Get(), (m_zLength + 1) * sizeof(wchar_t));
	}
}

/** Assignment operator */
const WString& WString::operator=(const WString& wStr)
{
	WString tmp(wStr);
	tmp.m_pwBuffer.Swap(m_pwBuffer);
	Seoul::Swap(tmp.m_zLength, m_zLength);
	return *this;
}

} // namespace Seoul
