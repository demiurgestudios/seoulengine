/**
 * \file SeoulString.h
 * \brief Implementation of a string class in SeoulEngine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_STRING_H
#define SEOUL_STRING_H

#include "ScopedArray.h"
#include "SeoulAssert.h"
#include "SeoulTypes.h"

#include <stdarg.h>  // for va_list
#include <stddef.h>  // for nullptr

#ifdef __OBJC__
#import <Foundation/NSString.h>
#endif

namespace Seoul
{

// Forward declarations
class HString;
class WString;
namespace CaseMappingInternal
{
	struct RootTable;
}

/**
 * Encodes the given character into UTF-8
 *
 * Encodes the given character into UTF-8, and places the encoded string in the
 * given buffer.  The buffer must be at least 4 bytes, i.e. large enough to hold
 * the encoded form of any possible Unicode character.  The return value is the
 * number of bytes stored in the buffer.  The buffer is NOT null-terminated.
 *
 * @param[in] chChar Character to encode into UTF-8
 * @param[out] sBuffer Buffer to encode the character into
 *
 * @return The number of bytes written to sBuffer
 */
static inline Byte UTF8EncodeChar(UniChar chChar, Byte *sBuffer)
{
	if (chChar < 0x0080)
	{
		sBuffer[0] = (Byte)chChar;
		return 1;
	}
	else if (chChar < 0x0800)
	{
		sBuffer[0] = (Byte)(0xC0 | ((chChar >> 6) & 0x1F));
		sBuffer[1] = (Byte)(0x80 | (chChar & 0x3F));
		return 2;
	}
	else if (chChar < 0x10000)
	{
		sBuffer[0] = (Byte)(0xE0 | ((chChar >> 12) & 0x0F));
		sBuffer[1] = (Byte)(0x80 | ((chChar >> 6) & 0x3F));
		sBuffer[2] = (Byte)(0x80 | (chChar & 0x3F));
		return 3;
	}
	else
	{
		sBuffer[0] = (Byte)(0xF0 | ((chChar >> 18) & 0x07));
		sBuffer[1] = (Byte)(0x80 | ((chChar >> 12) & 0x3F));
		sBuffer[2] = (Byte)(0x80 | ((chChar >> 6) & 0x3F));
		sBuffer[3] = (Byte)(0x80 | (chChar & 0x3F));
		return 4;
	}
}

/**
 * Decodes the given UTF-8 byte sequence into a character
 *
 * Decodes the given UTF-8 byte sequence into a character.  If the byte sequence
 * is not a valid UTF-8 byte sequence, the results are undefined.  Only one
 * character is decoded.
 *
 * @param[in] sBuffer UTF-8 byte sequence to decode
 *
 * @return Unicode character corresponding to the byte sequence represented by
 *         sBuffer
 */
static inline UniChar UTF8DecodeChar(const Byte *sBuffer)
{
	Byte cByte = sBuffer[0];

	if ((cByte & 0x80) == 0x00)  // 1-byte character (0xxxxxxx)
	{
		return (UniChar)cByte;
	}
	else if ((cByte & 0xE0) == 0xC0)  // 2-byte character (110xxxxx 10xxxxxx)
	{
		return (UniChar)((((UInt)cByte      & 0x1F) << 6) |
			             ( (UInt)sBuffer[1] & 0x3F));
	}
	else if ((cByte & 0xF0) == 0xE0)  // 3-byte character (1110xxxx 10xxxxxx 10xxxxxx)
	{
		return (UniChar)((((UInt)cByte & 0x0F)      << 12) |
		                 (((UInt)sBuffer[1] & 0x3F) <<  6) |
						 ( (UInt)sBuffer[2] & 0x3F));
	}
	else  // 4-byte character (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
	{
		return (UniChar)((((UInt)cByte & 0x07)      << 18) |
		                 (((UInt)sBuffer[1] & 0x3F) << 12) |
						 (((UInt)sBuffer[2] & 0x3F) <<  6) |
						 ( (UInt)sBuffer[3] & 0x3F));
	}
}

class StringIterator SEOUL_SEALED
{
public:
	typedef Int32 DifferenceType;
	typedef UInt32 IndexType;
	typedef UInt32 SizeType;

	StringIterator()
		: m_s("")
		, m_zIndexInBytes(0)
	{
	}

	explicit StringIterator(const char* s, SizeType zStartIndexInBytes = 0)
		: m_s(s)
		, m_zIndexInBytes(zStartIndexInBytes)
	{
	}

	StringIterator& operator=(const StringIterator& i)
	{
		m_s = i.m_s;
		m_zIndexInBytes = i.m_zIndexInBytes;
		return *this;
	}

	bool operator==(const StringIterator& b) const
	{
		return (m_s == b.m_s && m_zIndexInBytes == b.m_zIndexInBytes);
	}

	bool operator!=(const StringIterator& b) const
	{
		return !(*this == b);
	}

	UniChar operator*() const
	{
		return UTF8DecodeChar(m_s + m_zIndexInBytes);
	}

	StringIterator operator+(DifferenceType iOffset) const
	{
		StringIterator ret(*this);

		if (iOffset >= 0)
		{
			for (DifferenceType i = 0; i < iOffset; ++i)
			{
				++ret;
			}
		}
		else
		{
			for (DifferenceType i = 0; i < -iOffset; ++i)
			{
				--ret;
			}
		}

		return ret;
	}

	StringIterator& operator+=(DifferenceType iOffset)
	{
		if (iOffset >= 0)
		{
			for (DifferenceType i = 0; i < iOffset; ++i)
			{
				++(*this);
			}
		}
		else
		{
			for (DifferenceType i = 0; i < -iOffset; ++i)
			{
				--(*this);
			}
		}

		return *this;
	}

	StringIterator operator-(DifferenceType iOffset) const
	{
		StringIterator ret(*this);

		if (iOffset >= 0)
		{
			for (DifferenceType i = 0; i < iOffset; ++i)
			{
				--ret;
			}
		}
		else
		{
			for (DifferenceType i = 0; i < -iOffset; ++i)
			{
				++ret;
			}
		}

		return ret;
	}

	StringIterator& operator-=(DifferenceType iOffset)
	{
		if (iOffset >= 0)
		{
			for (DifferenceType i = 0; i < iOffset; ++i)
			{
				--(*this);
			}
		}
		else
		{
			for (DifferenceType i = 0; i < -iOffset; ++i)
			{
				++(*this);
			}
		}

		return *this;
	}

	StringIterator& operator++()
	{
		do
		{
			++m_zIndexInBytes;
		} while (0x80 == (0xC0 & m_s[m_zIndexInBytes])) ;

		return *this;
	}

	StringIterator operator++(int)
	{
		StringIterator ret(*this);

		do
		{
			++m_zIndexInBytes;
		} while (0x80 == (0xC0 & m_s[m_zIndexInBytes])) ;

		return ret;
	}

	StringIterator& operator--()
	{
		do
		{
			SEOUL_ASSERT(m_zIndexInBytes > 0);
			--m_zIndexInBytes;
		} while (0x80 == (0xC0 & m_s[m_zIndexInBytes])) ;

		return *this;
	}

	StringIterator operator--(int)
	{
		StringIterator ret(*this);

		do
		{
			SEOUL_ASSERT(m_zIndexInBytes > 0);
			--m_zIndexInBytes;
		} while (0x80 == (0xC0 & m_s[m_zIndexInBytes])) ;

		return ret;
	}

	IndexType GetIndexInBytes() const
	{
		return m_zIndexInBytes;
	}

	const char* GetPtr() const
	{
		return (m_s + m_zIndexInBytes);
	}

	void SetIndexInBytes(IndexType zIndexInBytes)
	{
		m_zIndexInBytes = zIndexInBytes;
	}

	void SetPtr(const char* s)
	{
		m_s = s;
	}

private:
	const char* m_s;
	IndexType m_zIndexInBytes;
}; // class StringIterator

class StringReverseIterator SEOUL_SEALED
{
public:
	typedef Int32 DifferenceType;
	typedef Int32 IndexType;
	typedef UInt32 SizeType;

	StringReverseIterator()
		: m_s("")
		, m_iIndexInBytes(0)
	{
	}

	explicit StringReverseIterator(const char* s, DifferenceType iStartIndexInBytes)
		: m_s(s)
		, m_iIndexInBytes(iStartIndexInBytes)
	{
	}

	StringReverseIterator& operator=(const StringReverseIterator& i)
	{
		m_s = i.m_s;
		m_iIndexInBytes = i.m_iIndexInBytes;
		return *this;
	}

	bool operator==(const StringReverseIterator& b) const
	{
		return (m_s == b.m_s && m_iIndexInBytes == b.m_iIndexInBytes);
	}

	bool operator!=(const StringReverseIterator& b) const
	{
		return !(*this == b);
	}

	UniChar operator*() const
	{
		return UTF8DecodeChar(m_s + m_iIndexInBytes);
	}

	StringReverseIterator operator+(DifferenceType iOffset) const
	{
		StringReverseIterator ret(*this);

		if (iOffset >= 0)
		{
			for (DifferenceType i = 0; i < iOffset; ++i)
			{
				++ret;
			}
		}
		else
		{
			for (DifferenceType i = 0; i < -iOffset; ++i)
			{
				--ret;
			}
		}

		return ret;
	}

	StringReverseIterator& operator+=(DifferenceType iOffset)
	{
		if (iOffset >= 0)
		{
			for (DifferenceType i = 0; i < iOffset; ++i)
			{
				++(*this);
			}
		}
		else
		{
			for (DifferenceType i = 0; i < -iOffset; ++i)
			{
				--(*this);
			}
		}

		return *this;
	}

	StringReverseIterator operator-(DifferenceType iOffset) const
	{
		StringReverseIterator ret(*this);

		if (iOffset >= 0)
		{
			for (DifferenceType i = 0; i < iOffset; ++i)
			{
				--ret;
			}
		}
		else
		{
			for (DifferenceType i = 0; i < -iOffset; ++i)
			{
				++ret;
			}
		}

		return ret;
	}

	StringReverseIterator& operator-=(DifferenceType iOffset)
	{
		if (iOffset >= 0)
		{
			for (DifferenceType i = 0; i < iOffset; ++i)
			{
				--(*this);
			}
		}
		else
		{
			for (DifferenceType i = 0; i < -iOffset; ++i)
			{
				++(*this);
			}
		}

		return *this;
	}

	StringReverseIterator& operator++()
	{
		do
		{
			--m_iIndexInBytes;
		} while (m_iIndexInBytes >= 0 && 0x80 == (0xC0 & m_s[m_iIndexInBytes])) ;

		return *this;
	}

	StringReverseIterator operator++(int)
	{
		StringReverseIterator ret(*this);

		do
		{
			--m_iIndexInBytes;
		} while (m_iIndexInBytes >= 0 && 0x80 == (0xC0 & m_s[m_iIndexInBytes])) ;

		return ret;
	}

	StringReverseIterator& operator--()
	{
		do
		{
			++m_iIndexInBytes;
		} while (0x80 == (0xC0 & m_s[m_iIndexInBytes])) ;

		return *this;
	}

	StringReverseIterator operator--(int)
	{
		StringReverseIterator ret(*this);

		do
		{
			++m_iIndexInBytes;
		} while (0x80 == (0xC0 & m_s[m_iIndexInBytes])) ;

		return ret;
	}

	IndexType GetIndexInBytes() const
	{
		return m_iIndexInBytes;
	}

	const char* GetPtr() const
	{
		return (m_s + m_iIndexInBytes);
	}

private:
	const char* m_s;
	IndexType m_iIndexInBytes;
}; // class StringReverseIterator

/**
 * String class
 *
 * String class.  All Strings are encoded using UTF-8.  Internally, the String
 * is represented as an array of Bytes.  The size of the String is the length
 * of the buffer in bytes, whereas the length of the String is the number of
 * characters encoded into the buffer.  Since UTF-8 is a multi-byte encoding,
 * the length will not necessarily equal the size.
 */
class String SEOUL_SEALED
{
public:
	typedef StringIterator ConstIterator;
	typedef StringIterator Iterator;
	typedef StringReverseIterator ReverseConstIterator;
	typedef StringReverseIterator ReverseIterator;

	// Default constructor - creates empty string
	String();

	// HString string constructor
	explicit String(HString hstring);

#ifdef __OBJC__
	// NSString constructor
	explicit String(NSString *sString)
		: m_zSize(0),
		  m_zCapacity(kSmallBufferSize)
	{
		m_buffer.aSmallBuffer[0] = 0;

		Assign([sString UTF8String]);
	}
#endif

	// C string constructor
	String(const Byte *sStr);

	// C substring constructor
	String(const Byte *sStr, UInt zSize);

	// Character constructor
	explicit String(UniChar chChar, UInt nCount = 1);

	// Copy constructor
	String(const String & sStr);

	// Move constructor.
	String(String&& rsStr);

	~String();

	// Assign something else to this string
	String & Assign(const Byte *sStr);
	String & Assign(const Byte *sStr, UInt zSize);
	String & Assign(UniChar chChar, UInt nCount = 1);
	String & Assign(const String & sStr);

	String & operator = (const Byte *sStr);
	String & operator = (const String & sStr);
	String & operator = (String&& rsStr);

	// Append something to this string
	String & Append(const Byte *sStr);
	String & Append(const Byte *sStr, UInt zSize);
	String & Append(UniChar chChar, UInt nCount = 1);
	String & Append(const String & sStr);

	String & operator += (const Byte *sStr);
	String & operator += (UniChar chChar);
	String & operator += (const String & sStr);

	// Compare this string to another
	Int Compare(const String & sStr) const;
	Int Compare(const Byte *sStr) const;

	// Compare this string to another (case-insensitive)
	Int CompareASCIICaseInsensitive(const String & sStr) const;
	Int CompareASCIICaseInsensitive(const Byte *sStr) const;

	// Clear this string out to an empty string and free up memory
	void Clear();

	// Shortens the string in-place; will not lengthen the string
	void ShortenTo(UInt zSize);

	// Reserve at least this much space in internal buffer
	void Reserve(UInt zCapacity);

	// Reallocates to use only as much memory as necessary
	void Trim();

	// Find characters or substrings
	UInt Find(UniChar chChar, UInt nStartIndex = 0) const;
	UInt Find(const String & sString, UInt nStartIndex = 0) const;
	UInt FindLast(UniChar chChar, UInt nStartIndex = NPos) const;
	UInt FindLast(const String & sString, UInt nStartIndex = NPos) const;
	UInt FindFirstOf(const String & sCharSet, UInt nStartIndex = 0) const;
	UInt FindFirstNotOf(const String & sCharSet, UInt nStartIndex = 0) const;
	UInt FindLastOf(const String & sCharSet, UInt nStartIndex = NPos) const;
	UInt FindLastNotOf(const String & sCharSet, UInt nStartIndex = NPos) const;

	// Does this string start / end with another?
	Bool StartsWith(const String & sStr) const;
	Bool StartsWithASCIICaseInsensitive(const String & sStr) const;
	Bool EndsWith(const String & sStr) const;

	// Create a substring of this
	String Substring(UInt nIndex) const;
	String Substring(UInt nIndex, UInt zSize) const;

	// Output the string's data to rpData. A new buffer will be allocated
	// if this String is using internal storage.
	void RelinquishBuffer(void*& rpData, UInt32& ruData);

	// Replace matching substrings with other strings; does not modify this
	// string
	String ReplaceAll(const String & sPattern, const String & sReplacement) const;

	// Returns the reversal of this string
	String Reverse() const;

	// Returns an all uppercase version of this string.  Does not modify this
	// string.  Nearly full support for Unicode.  The locale should be a
	// lowercase two-letter ISO 639-1 language code (e.g. "en").
	String ToUpper(const String& sLocaleID) const;

	// Returns an all lowercase version of this string.  Does not modify this
	// string.  Nearly full support for Unicode.  The locale should be a
	// lowercase two-letter ISO 639-1 language code (e.g. "en").
	String ToLower(const String& sLocaleID) const;

	// Returns an all uppercase version of this string.  Does not modify this
	// string.  Only converts ASCII characters.
	String ToUpperASCII() const;

	// Returns an all lowercase version of this string.  Does not modify this
	// string.  Only converts ASCII characters.
	String ToLowerASCII() const;

	// Tests if the String contains only ASCII characters; not constant time
	Bool IsASCII() const;

	// Check for empty string
	Bool IsEmpty() const { return (m_zSize == 0); }

	// Convert to a C string
	const Byte *CStr() const
	{
		return GetBuffer();
	}

	// Convert to a wide-character C string (e.g. for Win32 API functions)
	WString WStr() const;

	// Size in BYTES of the string
	UInt GetSize() const
	{
		return m_zSize;
	}

	// Length in UNICODE CHARACTERS of the string; not constant time, and not
	// usually what you want unless you're doing font rendering or something
	UInt GetUnicodeLength() const;

	// Gets the capacity of the string in bytes (maximum number of bytes, plus
	// 1, that can be stored without requiring a reallocation)
	UInt GetCapacity() const
	{
		return m_zCapacity;
	}

	// Byte indexing -- NOT character indexing (be careful!)
	Byte operator [] (UInt nIndex) const
	{
		SEOUL_ASSERT(nIndex <= m_zSize);  // Allow the dereference of the terminating nullptr
		return GetBuffer()[nIndex];
	}

	// Byte indexing -- NOT character indexing (be careful!)
	Byte& operator [] (UInt nIndex)
	{
		SEOUL_ASSERT(nIndex < m_zSize);  // Don't allow the dereference of the terminating nullptr in the mutation case.
		return GetBuffer()[nIndex];
	}

	// Returns the Unicode character starting at the given *byte* index
	UniChar CharAtByteIndex(UInt nIndex) const;

	// Swaps this String with another String
	void Swap(String& sOther);

#ifdef __OBJC__
	/**
	 * Converts this String to an NSString.  The returned string is
	 * autoreleased.
	 */
	NSString *ToNSString() const SEOUL_RETURNS_AUTORELEASED
	{
		return [NSString stringWithUTF8String:CStr()];
	}
#endif

	// Iterator pointing to beginning of the String
	Iterator Begin() const;

	// Iterator pointing just past end of the String
	Iterator End() const;

	// Remove the last element of this String. SEOUL_FAIL() if the String IsEmpty().
	void PopBack();

	/** Swap the internal buffer of String with a raw byte buffer allocated with MemoryManager::Allocate. */
	void TakeOwnership(void*& rpData, UInt32& ruSizeInBytes);

	// Create a formatted String printf-style
	static String Printf(const Byte *sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(1, 2);
	static String VPrintf(const Byte *sFormat, va_list argList) SEOUL_PRINTF_FORMAT_ATTRIBUTE(1, 0);

	// Create a formatted String printf-style, using wide character arguments.
	static String WPrintf(wchar_t const* sFormat, ...);
	static String WVPrintf(wchar_t const* sFormat, va_list argList);

private:

	// Helper function for performing ToUpper()/ToLower()
	String InternalCaseMap(const String& sLocale, const CaseMappingInternal::RootTable& rCaseTable) const;

	/**
	 * Size of the internal stack buffer used for the small string
	 * optimization
	 */
	enum { kSmallBufferSize = sizeof(Byte*) };

	// Tests if we are using the small string optimization
	Bool IsUsingInternalBuffer() const
	{
		return (m_zCapacity <= kSmallBufferSize);
	}

	// Returns pointer to the actual buffer used (see comment below).  Always
	// returns a non-nullptr pointer.
	Byte *GetBuffer()
	{
		return IsUsingInternalBuffer() ? m_buffer.aSmallBuffer : m_buffer.pBuffer;
	}

	// Const version of the above
	const Byte *GetBuffer() const
	{
		return IsUsingInternalBuffer() ? m_buffer.aSmallBuffer : m_buffer.pBuffer;
	}

	/**
	 * The m_pBuffer pointer is itself used a string storage as long as the
	 * characters (plus the null terminator) fit into the space of a pointer
	 * (4 bytes on 32-bit, 8 bytes on 64-bit).
	 */
	union
	{
		/** Pointer to heap buffer for large strings */
		Byte *pBuffer;

		/** Internal buffer for small strings */
		Byte aSmallBuffer[kSmallBufferSize];
	} m_buffer;

	/** Length (in bytes) of string */
	UInt m_zSize;

	/** Capacity (in bytes) of buffer */
	UInt m_zCapacity;

public:
	// Constant indicating that something was not found in a String
	static const UInt NPos;
};
SEOUL_STATIC_ASSERT(sizeof(String) == sizeof(UInt) + sizeof(UInt) + sizeof(void*));

// String concatenation operators
String operator + (const String & sStr1, const String & sStr2);
String operator + (const String & sStr1, const Byte *sStr2);
String operator + (const Byte *sStr1, const String & sStr2);

// String comparison operators
Bool operator == (const String & sStr1, const String & sStr2);
Bool operator == (const String & sStr1, const Byte *sStr2);
Bool operator == (const Byte *sStr1, const String & sStr2);

Bool operator != (const String & sStr1, const String & sStr2);
Bool operator != (const String & sStr1, const Byte *sStr2);
Bool operator != (const Byte *sStr1, const String & sStr2);

Bool operator < (const String & sStr1, const String & sStr2);
Bool operator < (const String & sStr1, const Byte *sStr2);
Bool operator < (const Byte *sStr1, const String & sStr2);

Bool operator <= (const String & sStr1, const String & sStr2);
Bool operator <= (const String & sStr1, const Byte *sStr2);
Bool operator <= (const Byte *sStr1, const String & sStr2);

Bool operator > (const String & sStr1, const String & sStr2);
Bool operator > (const String & sStr1, const Byte *sStr2);
Bool operator > (const Byte *sStr1, const String & sStr2);

Bool operator >= (const String & sStr1, const String & sStr2);
Bool operator >= (const String & sStr1, const Byte *sStr2);
Bool operator >= (const Byte *sStr1, const String & sStr2);

template <>
struct DefaultHashTableKeyTraits<String>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static String GetNullKey()
	{
		return String();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

/**
 * A WString encapsulates a wide-character string, suitable for interacting
 * with the various Win32 Unicode APIs.  It automatically handles the memory
 * management, so that a String can be easily converted to a const wchar_t*.
 */
class WString
{
public:
	// Default constructor
	WString();

	// Destructor
	~WString();

	// UTF-8 string constructor
	WString(const String& sStr);

	// Copy constructor
	WString(const WString& wStr);

	// Assignment operator
	const WString& operator = (const WString& wStr);

	// Conversion operator
	operator const wchar_t* () const { return m_pwBuffer.IsValid() ? m_pwBuffer.Get() : L""; }

	/**
	 * Swap the contents of rpwBuffer for the internal buffer of this WString.
	 */
	void Swap(ScopedArray<wchar_t>& rpwBuffer)
	{
		m_pwBuffer.Swap(rpwBuffer);
	}

	/**
	 * Gets the length in wide characters (NOT bytes) of the string
	 */
	UInt GetLengthInChars() const { return m_zLength; }

private:
	/** Pointer to the wide-character data */
	ScopedArray<wchar_t> m_pwBuffer;

	/** Length in wide characters (NOT bytes) of the string */
	UInt m_zLength;
}; // class String

} // namespace Seoul

#endif // include guard
